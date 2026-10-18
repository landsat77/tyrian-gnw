/*
 * Files of OpenTyrian on the Game & Watch (replaces src/ot/file.c).
 *
 * Data files are read in place from the asset pack (external flash,
 * ty_plat_find_file()); user files (configuration, saved games) are real
 * files in ty_plat_save_dir().
 */
#include "file.h"

#include "opentyr.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ty_platform.h"

const char *customDataDirPath = NULL;

enum
{
	ERRNUM_EOF = -1,
	ERRNUM_NOT_FOUND = ENOENT,
};

bool findDataFiles(void)
{
	return dataFileExists("tyrian1.lvl");
}

bool dataFileExists(const char *filename)
{
	const Uint8 *data;
	Uint32 size;
	return ty_plat_find_file(filename, &data, &size);
}

bool userFileExists(const char *filename)
{
	File file = userFileOpen(filename, "rb");
	bool result = !file.error;
	fileClose(&file);
	return result;
}

File dataFileOpen(const char *filename, const char *mode)
{
	File file;
	memset(&file, 0, sizeof(file));
	(void)mode;
	if (!ty_plat_find_file(filename, &file.mem, &file.size)) {
		file.mem = NULL;
		file.errnum = ERRNUM_NOT_FOUND;
		file.error = true;
	}
	return file;
}

File userFileOpen(const char *filename, const char *mode)
{
	File file;
	memset(&file, 0, sizeof(file));
	char path[256];
	snprintf(path, sizeof(path), "%s/%s", ty_plat_save_dir(), filename);
	file.f = fopen(path, mode);
	if (file.f == NULL) {
		file.errnum = ERRNUM_NOT_FOUND;
		file.error = true;
	}
	return file;
}

const Uint8 *fileMapped(File *file)
{
	if (file->error || file->mem == NULL)
		return NULL;
	return file->mem + file->pos;
}

void fileSetPosition(File *file, long position)
{
	if (file->error)
		return;
	if (file->mem) {
		if (position < 0 || (Uint32)position > file->size) {
			file->errnum = ERRNUM_EOF;
			file->error = true;
			return;
		}
		file->pos = (Uint32)position;
		return;
	}
	if (fseek(file->f, position, SEEK_SET) != 0) {
		file->errnum = EIO;
		file->error = true;
	}
}

long fileGetPosition(File *file)
{
	if (file->error)
		return 0;
	if (file->mem)
		return (long)file->pos;
	long position = ftell(file->f);
	if (position >= 0)
		return position;
	file->errnum = EIO;
	file->error = true;
	return 0;
}

long fileGetLength(File *file)
{
	if (file->error)
		return 0;
	if (file->mem)
		return (long)file->size;
	long position = ftell(file->f);
	if (position >= 0 && fseek(file->f, 0, SEEK_END) == 0) {
		long length = ftell(file->f);
		if (length >= 0 && fseek(file->f, position, SEEK_SET) == 0)
			return length;
	}
	file->errnum = EIO;
	file->error = true;
	return 0;
}

size_t fileReadAtMost(File *file, void *data, size_t size)
{
	if (file->error)
		return 0;
	if (file->mem) {
		size_t n = file->size - file->pos;
		if (n > size)
			n = size;
		memcpy(data, file->mem + file->pos, n);
		file->pos += (Uint32)n;
		return n;
	}
	return fread(data, 1, size, file->f);
}

void fileReadExactly(File *file, void *data, size_t size)
{
	if (file->error) {
		memset(data, 0, size);
		return;
	}
	size_t read = fileReadAtMost(file, data, size);
	if (read == size)
		return;
	file->errnum = ERRNUM_EOF;
	file->error = true;
	memset((Uint8 *)data + read, 0, size - read);
}

void fileWrite(File *file, const void *data, size_t size)
{
	if (file->error)
		return;
	if (file->mem || fwrite(data, 1, size, file->f) != size) {
		file->errnum = EIO;
		file->error = true;
	}
}

void fileFlush(File *file)
{
	(void)file;  // written when closed
}

void fileClose(File *file)
{
	file->mem = NULL;
	if (file->f == NULL)
		return;
	int result = fclose(file->f);
	file->f = NULL;
	if (result != 0) {
		file->errnum = EIO;
		file->error = true;
	}
}

const char *fileGetError(File *file)
{
	switch (file->errnum) {
	case ERRNUM_EOF:
		return "Unexpected end of file";
	case ERRNUM_NOT_FOUND:
		return "Not found";
	default:
		return file->error ? "I/O error" : "No error";
	}
}
