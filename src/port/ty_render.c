/*
 * Harness only (option -R): renders the music of Tyrian (the Loudness
 * songs of music.mus, AdLib) with the Loudness player and the OPL emulator
 * of OpenTyrian, for tools/ty_pack.py.
 *
 * Each song becomes <dir>/musicNN.mus in the GWMU format of ty_music.c
 * (version 2, with its loop start), mono, 16 bit PCM or IMA ADPCM, at 22050
 * Hz or 11025 Hz (rendered at 22050 Hz, low-pass filtered and decimated).
 *
 * The songs are rendered like loudness.c plays them (one Loudness update
 * at 69.5 Hz, the OPL output in between). A song that jumps back (it
 * loops) is the first pass followed by the first TAIL_SECONDS of the second
 * one, in which the notes held at the end still sound; its loop goes back
 * to TAIL_SECONDS after the start of the position it jumps to. A song that
 * stops (Loudness command 0xFC) is played once, with a short release, and
 * flagged so (codec bit 15): the game waits for its end.
 */
#ifndef TY_DEVICE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "file.h"
#include "lds_play.h"
#include "loudness.h"
#include "opl.h"
#include "ty_platform.h"

unsigned int lds_position(void);

#define RENDER_RATE 22050
#define MAX_SECONDS 600
#define TAIL_SECONDS 2
#define RELEASE_MS 500

static const int ldsUpdate2Rate = 139;  // 69.5 * 2 (loudness.c)

/* ---- IMA ADPCM (ty_music.c) ---- */
static const int16_t ima_step[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
	253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
	1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
	3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
	11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
	32767
};
static const int8_t ima_index[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
#define ADPCM_BLOCK 1024

typedef struct { uint8_t *p; size_t n, cap; } buf_t;

static void put(buf_t *b, uint8_t v)
{
	if (b->n == b->cap) {
		b->cap = b->cap ? b->cap * 2 : 65536;
		b->p = realloc(b->p, b->cap);
	}
	b->p[b->n++] = v;
}
static void put16(buf_t *b, uint32_t x) { put(b, x & 0xFF); put(b, (x >> 8) & 0xFF); }
static void put32(buf_t *b, uint32_t x) { put16(b, x & 0xFFFF); put16(b, x >> 16); }

static void adpcm_encode(const int16_t *s, long n, buf_t *out)
{
	for (long b = 0; b < n; b += ADPCM_BLOCK) {
		long len = n - b < ADPCM_BLOCK ? n - b : ADPCM_BLOCK;
		const int16_t *blk = s + b;
		int pred = blk[0];
		int idx = 0;
		if (len > 1) {
			int d = blk[1] - blk[0];
			if (d < 0) d = -d;
			while (idx < 88 && ima_step[idx] < d) idx++;
		}
		put(out, (uint8_t)(pred & 0xFF));
		put(out, (uint8_t)((pred >> 8) & 0xFF));
		put(out, (uint8_t)idx);
		put(out, 0);
		uint8_t byte = 0;
		for (long i = 0; i < len; i++) {
			int step = ima_step[idx];
			int diff = blk[i] - pred;
			int code = 0;
			if (diff < 0) { code = 8; diff = -diff; }
			int dq = step >> 3;
			if (diff >= step) { code |= 4; diff -= step; dq += step; }
			step >>= 1;
			if (diff >= step) { code |= 2; diff -= step; dq += step; }
			step >>= 1;
			if (diff >= step) { code |= 1; dq += step; }
			pred = (code & 8) ? pred - dq : pred + dq;
			if (pred > 32767) pred = 32767;
			if (pred < -32768) pred = -32768;
			idx += ima_index[code & 7];
			if (idx < 0) idx = 0;
			if (idx > 88) idx = 88;
			if (i & 1)
				put(out, (uint8_t)(byte | (code << 4)));
			else
				byte = (uint8_t)code;
		}
		if (len & 1)
			put(out, byte);
	}
}

/* ---- decimation by 2 (22050 -> 11025 Hz): windowed sinc low-pass, 63
 * taps, delay compensated (the pop-gnw renderer) ---- */
#define DECIM_TAPS 63
#define DECIM_DELAY ((DECIM_TAPS - 1) / 2)

static long decimate(const int16_t *in, long n, int16_t *out)
{
	static double h[DECIM_TAPS];
	static bool ready;
	if (!ready) {
		const double pi = 3.14159265358979323846;
		const double fc = 0.45 * 0.5;
		double sum = 0;
		for (int k = 0; k < DECIM_TAPS; k++) {
			double m = k - DECIM_DELAY;
			double sinc = m == 0 ? 2 * fc : sin(2 * pi * fc * m) / (pi * m);
			double w = 0.42 - 0.5 * cos(2 * pi * k / (DECIM_TAPS - 1)) + 0.08 * cos(4 * pi * k / (DECIM_TAPS - 1));
			h[k] = sinc * w;
			sum += h[k];
		}
		for (int k = 0; k < DECIM_TAPS; k++)
			h[k] /= sum;
		ready = true;
	}
	long m = 0;
	for (long c = 0; c < n; c += 2, m++) {
		double acc = 0;
		for (int k = 0; k < DECIM_TAPS; k++) {
			long i = c + DECIM_DELAY - k;
			if (i >= 0 && i < n)
				acc += h[k] * in[i];
		}
		long v = lround(acc);
		out[m] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
	}
	return m;
}

/* ---- one song ---- */
typedef struct {
	int16_t *s;
	long n, cap;
} pcm_t;

static void pcm_grow(pcm_t *p, long more)
{
	if (p->n + more > p->cap) {
		while (p->n + more > p->cap)
			p->cap = p->cap ? p->cap * 2 : RENDER_RATE * 60;
		p->s = realloc(p->s, (size_t)p->cap * sizeof(int16_t));
	}
}

static bool render_song(const uint8_t *data, size_t size, unsigned int num, const char *dir,
                        int rate, bool adpcm)
{
	audioSampleRate = RENDER_RATE;
	opl_init();
	if (!lds_load(data, size)) {
		printf("render: song %u: invalid\n", num);
		return false;
	}

	const int samplesPerLdsUpdate = 2 * (RENDER_RATE / ldsUpdate2Rate);
	const int samplesPerLdsUpdateFrac = 2 * (RENDER_RATE % ldsUpdate2Rate);
	int untilUpdate = 0, untilUpdateFrac = 0;

	long first_at[256];
	for (int i = 0; i < 256; i++)
		first_at[i] = -1;

	pcm_t pcm = { 0 };
	long loop_from = -1, loop_at = -1;   // pass 1 position time, its repeat
	long end_at = -1;                     // the song stopped
	long stop_at = (long)RENDER_RATE * MAX_SECONDS;
	bool was_looped = false;

	while (pcm.n < stop_at) {
		if (untilUpdate == 0) {
			bool was_playing = playing;
			lds_update();
			unsigned int pos = lds_position() & 255;
			if (songlooped && !was_looped) {
				// jumped back: the second pass starts here
				was_looped = true;
				if (first_at[pos] >= 0) {
					loop_from = first_at[pos];
					loop_at = pcm.n;
					stop_at = pcm.n + (long)RENDER_RATE * (TAIL_SECONDS + 2);
				}
			}
			if (first_at[pos] < 0)
				first_at[pos] = pcm.n;
			if (was_playing && !playing && end_at < 0) {
				end_at = pcm.n;
				stop_at = pcm.n + (long)RENDER_RATE * RELEASE_MS / 1000;
			}
			untilUpdate += samplesPerLdsUpdate;
			untilUpdateFrac += samplesPerLdsUpdateFrac;
			if (untilUpdateFrac >= ldsUpdate2Rate) {
				untilUpdate += 1;
				untilUpdateFrac -= ldsUpdate2Rate;
			}
		}
		int count = untilUpdate < 256 ? untilUpdate : 256;
		pcm_grow(&pcm, count);
		opl_update(pcm.s + pcm.n, count);
		pcm.n += count;
		untilUpdate -= count;
	}

	long total, loop_start;
	bool ends = false;
	double err = 0;
	const char *how;
	if (loop_at > 0) {
		long tail = (long)RENDER_RATE * TAIL_SECONDS;
		if (tail > loop_at - loop_from)
			tail = loop_at - loop_from;
		loop_start = loop_from + tail;
		/* the second pass starts about where the jump was seen (the new
		 * pattern starts at the next row of the player): the repeat of the
		 * loop start that matches best */
		const long win = RENDER_RATE / 2, search = RENDER_RATE / 2;
		double best = 1e30;
		long best_at = loop_at;
		for (long c = loop_at - search; c <= loop_at + search; c++) {
			if (c + tail + win > pcm.n || c - loop_from < tail)
				continue;
			double e = 0;
			for (long k = 0; k < win && e < best * win; k += 4) {
				double d = pcm.s[c + tail + k] - pcm.s[loop_start + k];
				e += d * d;
			}
			e /= win / 4;
			if (e < best) {
				best = e;
				best_at = c;
			}
		}
		err = sqrt(best);
		total = best_at + tail;
		printf("render: song %u: repeat found %+ld samples from the jump\n", num, best_at - loop_at);
		how = "loops";
	} else if (end_at >= 0) {
		total = pcm.n;
		loop_start = 0;
		ends = true;
		how = "ends";
	} else {
		total = pcm.n;
		loop_start = 0;
		how = "cut";
	}

	int16_t *out = pcm.s;
	long out_n = total, out_loop = loop_start;
	if (rate == RENDER_RATE / 2) {
		out = malloc((size_t)(total / 2 + 1) * sizeof(int16_t));
		out_n = decimate(pcm.s, total, out);
		out_loop = loop_start / 2;
		if (out_loop >= out_n)
			out_loop = 0;
	} else {
		rate = RENDER_RATE;
	}

	buf_t mus = { 0 };
	put(&mus, 'G'); put(&mus, 'W'); put(&mus, 'M'); put(&mus, 'U');
	put16(&mus, 2);
	put16(&mus, (adpcm ? 1 : 0) | (ends ? 0x8000 : 0));
	put32(&mus, (uint32_t)rate);
	put32(&mus, (uint32_t)out_n);
	put32(&mus, (uint32_t)out_loop);
	if (adpcm) {
		adpcm_encode(out, out_n, &mus);
	} else {
		for (long i = 0; i < out_n; i++)
			put16(&mus, (uint16_t)out[i]);
	}

	char path[600];
	snprintf(path, sizeof(path), "%s/music%02u.mus", dir, num);
	FILE *f = fopen(path, "wb");
	bool ok = f != NULL;
	if (f) {
		ok = fwrite(mus.p, 1, mus.n, f) == mus.n;
		fclose(f);
	}
	printf("render: song %u: %.1f s, %s at %.1f s (rms diff %.1f)\n", num, total / (double)RENDER_RATE,
	       how, loop_start / (double)RENDER_RATE, err);
	if (out != pcm.s)
		free(out);
	free(pcm.s);
	free(mus.p);
	return ok;
}

int ty_render_music(const char *dir, int rate, bool adpcm)
{
	File file = dataFileOpen("music.mus", "rb");
	if (file.error) {
		printf("render: no music.mus\n");
		return 0;
	}
	const uint8_t *base = fileMapped(&file);
	long length = fileGetLength(&file);
	unsigned int count = fileReadU16(&file);
	long *pos = calloc(count + 1, sizeof(long));
	for (unsigned int i = 0; i < count; i++)
		pos[i] = (long)fileReadU32(&file);
	pos[count] = length;

	int done = 0;
	const char *only = getenv("TY_RENDER_ONLY");  // debugging: one song
	for (unsigned int i = 0; i < count; i++) {
		if (pos[i + 1] <= pos[i] || pos[i] >= length)
			continue;
		if (only && (unsigned int)atoi(only) != i)
			continue;
		if (render_song(base + pos[i], (size_t)(pos[i + 1] - pos[i]), i, dir, rate, adpcm))
			done++;
	}
	free(pos);
	fileClose(&file);
	return done;
}
#endif
