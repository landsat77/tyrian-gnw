/*
 * Sound of OpenTyrian on the Game & Watch (replaces src/ot/loudness.c).
 *
 *  - Music: the Loudness songs (AdLib, music.mus) are pre-rendered on the PC
 *    (ty_render.c) to "musicNN.mus" pieces of the pack, played back in place
 *    (ty_music.c), looping or once, with the fade of the game.
 *  - Sound effects and voices: the 8 bit 11025 Hz samples of tyrian.snd /
 *    voices.snd, used in place from the pack (nortsong.c GNW), resampled
 *    here with a fixed point step.
 *
 * The volumes follow loudness.c (30 dB range, the music twice as loud).
 */
#include "loudness.h"

#include "lds_play.h"
#include "logging.h"
#include "opentyr.h"

#include <stdio.h>
#include <string.h>

#include "ty_platform.h"
#include "ty_music.h"

int audioSampleRate = 0;

bool music_stopped = true;
unsigned int song_playing = 0;

bool audio_disabled = false, music_disabled = false, samples_disabled = false;

#ifdef TY_DEVICE
/* lds_play.c is not built on the console */
bool playing, songlooped;
#endif

static Uint8 musicVolume = 255;
static Uint8 sampleVolume = 255;

// Fixed point Q20.12 (loudness.c)
static Sint32 volumeFactorTable[256];
#define TO_FIXED(x) ((Sint32)((x) * (1 << 12)))
#define FIXED_TO_INT(x) ((Sint32)((x) >> 12))

/* the fade of the Loudness player: the volume goes down by one 255th at
 * each of its updates (69.5 Hz), down to 1 */
static const int ldsUpdate2Rate = 139;  // 69.5 * 2
static int fadeLevel;    // 255 = full, 0 = no fade
static int fadeSpeed;
static int samplesUntilFade;

static ty_mus_t *song;
static Uint32 songPlayed, songLength;

#define CHANNEL_COUNT 8
#define CHANNEL_VOLUME_LEVELS 8
static const Sint8 *channelSamples[CHANNEL_COUNT];
static Uint32 channelLength[CHANNEL_COUNT];   // samples
static Uint32 channelPos[CHANNEL_COUNT];      // 16.16
static Uint8 channelVolume[CHANNEL_COUNT];
static Uint32 sampleStep;                     // 16.16, 11025 Hz -> output

static void audioCallback(void *userdata, Uint8 *stream, int size);

bool init_audio(void)
{
	if (audio_disabled)
		return false;

	SDL_AudioSpec ask, got;
	memset(&ask, 0, sizeof(ask));
	ask.freq = 22050;
	ask.format = AUDIO_S16SYS;
	ask.channels = 1;
	ask.samples = 512;
	ask.callback = audioCallback;

	if (SDL_OpenAudioDevice(NULL, 0, &ask, &got, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE) == 0)
	{
		audio_disabled = true;
		return false;
	}

	audioSampleRate = got.freq;
	sampleStep = (Uint32)(((Uint64)11025 << 16) / (Uint32)audioSampleRate);

	// 10^((255 - i) * -30 / (20 * 255)), without powf
	const double step = 0.986543;  // 10^(-30 / (20 * 255))
	double f = 1.0;
	volumeFactorTable[0] = 0;
	for (int i = 255; i >= 1; --i)
	{
		volumeFactorTable[i] = TO_FIXED(f);
		f *= step;
	}

	SDL_PauseAudioDevice(1, 0);

	return true;
}

static void mixMusic(Sint16 *samples, int count)
{
	memset(samples, 0, (size_t)count * sizeof(Sint16));
	if (song == NULL || music_stopped || !playing)
		return;

	int n = count;
	if (ty_mus_ends(song))
	{
		Uint32 left = songLength > songPlayed ? songLength - songPlayed : 0;
		if ((Uint32)n > left)
			n = (int)left;
	}
	if (n > 0)
		ty_mus_mix(song, samples, n, 1, !music_disabled, audioSampleRate);
	songPlayed += (Uint32)n;
	if (ty_mus_ends(song) && songPlayed >= songLength)
		playing = false;

	if (fadeSpeed != 0)
	{
		// one fade step per Loudness update
		for (int i = 0; i < count; )
		{
			if (samplesUntilFade <= 0)
			{
				samplesUntilFade += 2 * audioSampleRate / ldsUpdate2Rate;
				fadeLevel = fadeLevel > fadeSpeed + 1 ? fadeLevel - fadeSpeed : 1;
			}
			int c = count - i < samplesUntilFade ? count - i : samplesUntilFade;
			for (int k = 0; k < c; ++k)
				samples[i + k] = (Sint16)(samples[i + k] * fadeLevel / 255);
			i += c;
			samplesUntilFade -= c;
		}
	}
}

static void audioCallback(void *userdata, Uint8 *stream, int size)
{
	(void)userdata;

	Sint16 *const samples = (Sint16 *)stream;
	const int samplesCount = size / (int)sizeof(Sint16);

	mixMusic(samples, samplesCount);

	Sint32 musicVolumeFactor = music_disabled ? 0 : volumeFactorTable[musicVolume];
	musicVolumeFactor *= 2;  // OPL emulator is too quiet

	Sint32 sampleVolumeFactor = volumeFactorTable[sampleVolume];
	Sint32 sampleVolumeFactors[CHANNEL_VOLUME_LEVELS];
	for (int i = 0; i < CHANNEL_VOLUME_LEVELS; ++i)
		sampleVolumeFactors[i] = sampleVolumeFactor * (i + 1) / CHANNEL_VOLUME_LEVELS;

	bool anySample = false;
	if (!samples_disabled)
		for (int i = 0; i < CHANNEL_COUNT; ++i)
			anySample |= channelLength[i] != 0;

	for (int s = 0; s < samplesCount; ++s)
	{
		Sint32 sample = samples[s] * musicVolumeFactor;

		if (anySample)
		{
			for (int i = 0; i < CHANNEL_COUNT; ++i)
			{
				if (channelLength[i] == 0)
					continue;
				Uint32 idx = channelPos[i] >> 16;
				if (idx >= channelLength[i])
				{
					channelLength[i] = 0;
					continue;
				}
				// 8 bit -> 16 bit, like SDL_ConvertAudio()
				sample += (channelSamples[i][idx] * 256) * sampleVolumeFactors[channelVolume[i]];
				channelPos[i] += sampleStep;
			}
		}

		sample = FIXED_TO_INT(sample);
		samples[s] = (Sint16)(sample < INT16_MIN ? INT16_MIN : sample > INT16_MAX ? INT16_MAX : sample);
	}
}

void deinit_audio(void)
{
	memset(channelLength, 0, sizeof channelLength);
	if (song != NULL)
	{
		ty_mus_close(song);
		song = NULL;
	}
}

static void load_song(unsigned int song_num)  // FKA NortSong.loadSong
{
	if (song != NULL)
	{
		ty_mus_close(song);
		song = NULL;
	}

	char name[16];
	snprintf(name, sizeof(name), "music%02u.mus", song_num);
	const Uint8 *data;
	Uint32 size;
	if (!ty_plat_find_file(name, &data, &size))
	{
		logWarn("No pre-rendered music %s", name);
		return;
	}
	song = ty_mus_open(data, (int)size);
	songLength = ty_mus_length(song, audioSampleRate);
}

static void rewind_song(void)
{
	ty_mus_rewind(song);
	songPlayed = 0;
	playing = song != NULL;
	songlooped = false;
	fadeLevel = 255;
	fadeSpeed = 0;
	samplesUntilFade = 0;
}

/* ty_state.c: the next play_song() starts its song again */
void fade_song_reset(void)
{
	song_playing = ~0u;
}

void play_song(unsigned int song_num)  // FKA NortSong.playSong
{
	if (audio_disabled)
		return;

	if (song_num != song_playing || song == NULL)
	{
		music_stopped = true;
		load_song(song_num);
		rewind_song();
		song_playing = song_num;
	}

	music_stopped = false;
}

void restart_song(void)  // FKA Player.selectSong(1)
{
	if (audio_disabled)
		return;

	rewind_song();
	music_stopped = false;
}

void stop_song(void)  // FKA Player.selectSong(0)
{
	if (audio_disabled)
		return;

	music_stopped = true;
}

void fade_song(void)  // FKA Player.selectSong($C001)
{
	if (audio_disabled)
		return;

	fadeSpeed = 1;
}

void set_volume(Uint8 musicVolume_, Uint8 sampleVolume_)  // FKA NortSong.setVol and Player.setVol
{
	if (audio_disabled)
		return;

	musicVolume = musicVolume_;
	sampleVolume = sampleVolume_;
}

/* `samples`: 8 bit samples of the pack (nortsong.c GNW), `sampleCount` of them */
void multiSamplePlay(const Sint16 *samples, size_t sampleCount, Uint8 chan, Uint8 vol)  // FKA Player.multiSamplePlay
{
	if (chan >= CHANNEL_COUNT || vol >= CHANNEL_VOLUME_LEVELS)
		return;

	if (audio_disabled || samples_disabled || samples == NULL)
		return;

	channelSamples[chan] = (const Sint8 *)samples;
	channelPos[chan] = 0;
	channelVolume[chan] = vol;
	channelLength[chan] = (Uint32)sampleCount;
}
