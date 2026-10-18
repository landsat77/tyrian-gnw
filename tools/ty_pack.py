#!/usr/bin/env python3
"""Build the game data pack of the Tyrian homebrew.

    python3 ty_pack.py Tyrian.ty /path/to/tyrian21

The folder holds the files of Tyrian 2.1 (the freeware version, tyrian21.zip):
tyrian1.lvl, tyrian.shp, palette.dat, music.mus... They are stored as they
are (names in lower case), with the music rendered to samples
(musicNN.mus): the console is too small to synthesize the AdLib music while
the game runs. The music is made by running the game's music player on this
computer: ty_render (ty_render/ next to this script, or the harness of the
repository). Pack format: src/port/ty_pack.c.

Only Python 3 is needed (no extra module).
"""
import argparse
import os
import platform
import shutil
import struct
import subprocess
import sys
import tempfile

MAGIC = b"GWTY01\0\0"
ENTRY_NAME_LEN = 24
REQUIRED = ["tyrian1.lvl", "tyrian.shp", "palette.dat", "tyrian.hdt", "music.mus"]
SKIP_EXT = (".exe", ".com", ".bat", ".doc", ".txt", ".ico", ".pif", ".diz", ".ovl", ".int",
            ".box", ".ini", ".pcx", ".tfp", ".awe", ".bin", ".cfg", ".sav")


def collect(folder):
    files = {}
    for entry in sorted(os.listdir(folder)):
        path = os.path.join(folder, entry)
        if not os.path.isfile(path):
            continue
        name = entry.lower()
        if name.endswith(SKIP_EXT) or len(name) >= ENTRY_NAME_LEN:
            continue
        with open(path, "rb") as fh:
            files[name] = fh.read()
    return files


def write_pack(out_path, files, verbose=True):
    header_size = 16 + len(files) * (ENTRY_NAME_LEN + 8)
    offset = (header_size + 3) & ~3
    entries = []
    for name, blob in files:
        entries.append((name, offset, len(blob)))
        offset = (offset + len(blob) + 3) & ~3
    total = offset
    out = bytearray(total)
    out[0:8] = MAGIC
    struct.pack_into("<II", out, 8, len(files), total)
    for i, (name, off, size) in enumerate(entries):
        nb = name.encode("ascii")[:ENTRY_NAME_LEN - 1]
        struct.pack_into("<%dsII" % ENTRY_NAME_LEN, out, 16 + i * (ENTRY_NAME_LEN + 8), nb, off, size)
    for (name, blob), (_, off, size) in zip(files, entries):
        out[off:off + size] = blob
    with open(out_path, "wb") as fh:
        fh.write(out)
    if verbose:
        print("wrote %s: %d files, %d bytes" % (out_path, len(files), total))
    return total


def default_renderer():
    """ty_render-<os>-<arch> in ty_render/ next to this script (release zip)
    or in tools/bin/ (tools/build_renderers.sh), else the harness built with
    'make -C harness'."""
    here = os.path.dirname(os.path.abspath(__file__))
    system = {"Linux": "linux", "Windows": "windows", "Darwin": "macos"}.get(platform.system())
    machine = platform.machine().lower()
    arch = "arm64" if machine in ("arm64", "aarch64") else "x86_64"
    candidates = []
    if system:
        names = ["ty_render-%s-%s%s" % (system, arch, ".exe" if system == "windows" else "")]
        if system == "windows" and arch == "arm64":  # x86_64 emulation
            names.append("ty_render-windows-x86_64.exe")
        for n in names:
            candidates += [os.path.join(here, "ty_render", n), os.path.join(here, "bin", n),
                           os.path.join(here, n)]
    candidates += [os.path.join(here, "..", "harness", "build", "ty_host")]
    for p in candidates:
        if os.path.isfile(p):
            prepare_executable(p)
            return p
    return None


def prepare_executable(path):
    """Make a renderer from a zip file runnable (exec bit, macOS quarantine)."""
    if os.name != "nt" and not os.access(path, os.X_OK):
        try:
            os.chmod(path, os.stat(path).st_mode | 0o111)
        except OSError:
            pass
    if platform.system() == "Darwin":
        subprocess.run(["xattr", "-d", "com.apple.quarantine", path],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def run(renderer, args, what):
    r = subprocess.run([renderer] + args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       universal_newlines=True)
    if r.returncode != 0:
        sys.stderr.write(r.stdout[-4000:])
        sys.exit("error: %s failed (%s)" % (what, renderer))
    return r.stdout


def render_music(items, renderer, codec, rate, verbose=True):
    """Run the game's music player on this computer: musicNN.mus files."""
    tmp = tempfile.mkdtemp(prefix="ty_pack_")
    try:
        base = os.path.join(tmp, "base.ty")
        write_pack(base, items, verbose=False)
        mdir = os.path.join(tmp, "music")
        os.mkdir(mdir)
        args = [base, "-R", mdir, "-M", str(rate)] + (["-A"] if codec == "adpcm" else [])
        run(renderer, args, "rendering the music")
        result = []
        size = 0
        for name in sorted(os.listdir(mdir)):
            if name.endswith(".mus"):
                with open(os.path.join(mdir, name), "rb") as fh:
                    data = fh.read()
                result.append((name, data))
                size += len(data)
        if verbose:
            print("music: %d songs, %s %d Hz, %d bytes" % (len(result), codec.upper(), rate, size))
        return result
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("output", help="pack to write (same name as the .bin, extension .ty)")
    ap.add_argument("folder", help="folder of Tyrian 2.1 (unzipped tyrian21.zip)")
    ap.add_argument("--music-rate", type=int, choices=(11025, 22050), default=11025,
                    help="rate of the pre-rendered music (default 11025)")
    ap.add_argument("--music-codec", choices=("pcm", "adpcm"), default="adpcm",
                    help="adpcm (default, 4 times smaller) or pcm (16 bit, no hiss)")
    ap.add_argument("--no-music", action="store_true", help="no music (smaller pack)")
    ap.add_argument("--renderer", help="ty_render program (ty_render/ next to this script by default)")
    ap.add_argument("-q", "--quiet", action="store_true")
    args = ap.parse_args()
    if not os.path.isdir(args.folder):
        sys.exit("error: %s is not a folder" % args.folder)
    files = collect(args.folder)
    if "tyrian1.lvl" not in files:
        # the zip holds a "tyrian21" folder
        sub = os.path.join(args.folder, "tyrian21")
        if os.path.isdir(sub):
            files = collect(sub)
    missing = [n for n in REQUIRED if n not in files]
    if missing:
        sys.exit("error: missing game data: %s (is this the folder of Tyrian 2.1?)" % ", ".join(missing))
    items = sorted(files.items())
    extra = []
    if not args.no_music:
        renderer = args.renderer or default_renderer()
        if not renderer or not os.path.isfile(renderer):
            sys.exit("error: ty_render not found (ty_render/ next to this script, 'make -C harness' or --renderer)")
        extra = render_music(items, renderer, args.music_codec, args.music_rate, verbose=not args.quiet)
    # the console plays the rendered music, not the AdLib songs
    items = [(n, b) for n, b in items if n != "music.mus"] + extra
    write_pack(args.output, sorted(items), verbose=not args.quiet)


if __name__ == "__main__":
    main()
