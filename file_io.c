/***************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 ***************************************************************************/

/***************************************************************************
 *
 * file_io.c
 *
 * Offset-based binary file input and output.
 *
 ***************************************************************************/

#include "file_io.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


/***************************************************************************
 *
 * Private types
 *
 ***************************************************************************/

typedef struct fio_Control fio_Control;

struct fio_Control {
	int	fd;
	int	writable;
};


/*=======================================================================
 *
 *	int fio_OffsetValid(uint64_t offset, size_t length)
 *
 *	Description:	Checks whether a byte range fits in an off_t
 *
 *	Input:		offset, starting file offset; length, byte count
 *
 *	Output:		Non-zero when the complete range is representable
 *
 *	Notes:		The build requires 64-bit file offsets.
 *
 *======================================================================*/
static int
fio_OffsetValid(uint64_t offset, size_t length)
{
	uint64_t last;

	if(offset > (uint64_t)INT64_MAX) {
		return 0;
	}

	if(length == 0) {
		return 1;
	}

	last = offset + (uint64_t)length - 1;
	return last >= offset && last <= (uint64_t)INT64_MAX;
}


/*=======================================================================
 *
 *	fio_Status fio_AllocateHandle(int fd, int writable, fio_Handle *hnd)
 *
 *	Description:	Allocates a file control structure
 *
 *	Input:		fd, open descriptor; writable, non-zero for writable files
 *
 *	Output:		hnd, allocated handle
 *			A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		The descriptor is closed if allocation fails.
 *
 *======================================================================*/
static fio_Status
fio_AllocateHandle(int fd, int writable, fio_Handle *hnd)
{
	fio_Control *ctrl;

	ctrl = malloc(sizeof(*ctrl));

	if(ctrl == NULL) {
		close(fd);
		return FIO_STATUS_NO_MEMORY;
	}

	ctrl->fd = fd;
	ctrl->writable = writable;
	*hnd = ctrl;
	return FIO_STATUS_OK;
}


/*=======================================================================
 *
 *	fio_Status fio_OpenFile(const char *filename, fio_Handle *hnd)
 *
 *	Description:	Opens an existing file for reading
 *
 *	Input:		filename, path of file to open
 *
 *	Output:		hnd, handle used for subsequent operations, or NULL
 *			if the operation fails
 *			A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		The returned handle must be released using fio_CloseFile().
 *
 *======================================================================*/
fio_Status
fio_OpenFile(const char *filename, fio_Handle *hnd)
{
	int fd;

	if(hnd != NULL) {
		*hnd = NULL;
	}

	if(filename == NULL || hnd == NULL) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	fd = open(filename, O_RDONLY);

	if(fd < 0) {
		return FIO_STATUS_OPEN_FAILED;
	}

	return fio_AllocateHandle(fd, 0, hnd);
}


/*=======================================================================
 *
 *	fio_Status fio_CreateFile(const char *filename, fio_Handle *hnd)
 *
 *	Description:	Creates a new file for reading and writing
 *
 *	Input:		filename, path of file to create
 *
 *	Output:		hnd, handle used for subsequent operations, or NULL
 *			if the operation fails
 *			A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		An existing file is not replaced.
 *
 *======================================================================*/
fio_Status
fio_CreateFile(const char *filename, fio_Handle *hnd)
{
	int fd;

	if(hnd != NULL) {
		*hnd = NULL;
	}

	if(filename == NULL || hnd == NULL) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0666);

	if(fd < 0) {
		return errno == EEXIST ? FIO_STATUS_FILE_EXISTS : FIO_STATUS_CREATE_FAILED;
	}

	return fio_AllocateHandle(fd, 1, hnd);
}


/*=======================================================================
 *
 *	fio_Status fio_GetFileLength(fio_Handle hnd, uint64_t *length)
 *
 *	Description:	Returns the current file length
 *
 *	Input:		hnd, handle returned by fio_OpenFile() or fio_CreateFile()
 *
 *	Output:		length, number of bytes in the file
 *			A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		None
 *
 *======================================================================*/
fio_Status
fio_GetFileLength(fio_Handle hnd, uint64_t *length)
{
	struct stat file_status;

	if(hnd == NULL || length == NULL) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	if(fstat(hnd->fd, &file_status) != 0 || file_status.st_size < 0) {
		return FIO_STATUS_FILE_STATUS_FAILED;
	}

	*length = (uint64_t)file_status.st_size;
	return FIO_STATUS_OK;
}


/*=======================================================================
 *
 *	fio_Status fio_SetFileLength(fio_Handle hnd, uint64_t length)
 *
 *	Description:	Changes the length of a writable file
 *
 *	Input:		hnd, handle returned by fio_CreateFile()
 *			length, requested file length in bytes
 *
 *	Output:		A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		Extending a file fills the new region with zero bytes.
 *
 *======================================================================*/
fio_Status
fio_SetFileLength(fio_Handle hnd, uint64_t length)
{
	if(hnd == NULL || !hnd->writable) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	if(length > (uint64_t)INT64_MAX) {
		return FIO_STATUS_OFFSET_TOO_LARGE;
	}

	if(ftruncate(hnd->fd, (off_t)length) != 0) {
		return FIO_STATUS_SET_LENGTH_FAILED;
	}

	return FIO_STATUS_OK;
}


/*=======================================================================
 *
 *	fio_Status fio_ReadBytes(fio_Handle hnd, uint64_t offset, void *data, size_t length)
 *
 *	Description:	Reads an exact number of bytes at a file offset
 *
 *	Input:		hnd, handle returned by fio_OpenFile() or fio_CreateFile()
 *			offset, zero-based file offset
 *			length, number of bytes to read
 *
 *	Output:		data, destination buffer
 *			A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		The file position is not shared between operations.
 *
 *======================================================================*/
fio_Status
fio_ReadBytes(fio_Handle hnd, uint64_t offset, void *data, size_t length)
{
	uint8_t	*destination;
	ssize_t	count;
	size_t	request_length;
	size_t	remaining;

	if(hnd == NULL || (data == NULL && length != 0)) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	if(!fio_OffsetValid(offset, length)) {
		return FIO_STATUS_OFFSET_TOO_LARGE;
	}

	destination = data;
	remaining = length;

	while(remaining != 0) {
		request_length = remaining > (size_t)SSIZE_MAX ? (size_t)SSIZE_MAX : remaining;
		count = pread(hnd->fd, destination, request_length, (off_t)offset);

		if(count > 0) {
			destination += (size_t)count;
			offset += (uint64_t)count;
			remaining -= (size_t)count;
		}
		else if(count == 0) {
			return FIO_STATUS_END_OF_FILE;
		}
		else if(errno != EINTR) {
			return FIO_STATUS_READ_FAILED;
		}
	}

	return FIO_STATUS_OK;
}


/*=======================================================================
 *
 *	fio_Status fio_WriteBytes(fio_Handle hnd, uint64_t offset, const void *data, size_t length)
 *
 *	Description:	Writes an exact number of bytes at a file offset
 *
 *	Input:		hnd, handle returned by fio_CreateFile()
 *			offset, zero-based file offset
 *			data, source buffer
 *			length, number of bytes to write
 *
 *	Output:		A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		The file position is not shared between operations.
 *
 *======================================================================*/
fio_Status
fio_WriteBytes(fio_Handle hnd, uint64_t offset, const void *data, size_t length)
{
	const uint8_t	*source;
	ssize_t			count;
	size_t			request_length;
	size_t			remaining;

	if(hnd == NULL || !hnd->writable || (data == NULL && length != 0)) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	if(!fio_OffsetValid(offset, length)) {
		return FIO_STATUS_OFFSET_TOO_LARGE;
	}

	source = data;
	remaining = length;

	while(remaining != 0) {
		request_length = remaining > (size_t)SSIZE_MAX ? (size_t)SSIZE_MAX : remaining;
		count = pwrite(hnd->fd, source, request_length, (off_t)offset);

		if(count > 0) {
			source += (size_t)count;
			offset += (uint64_t)count;
			remaining -= (size_t)count;
		}
		else if(count == 0) {
			return FIO_STATUS_WRITE_FAILED;
		}
		else if(errno != EINTR) {
			return FIO_STATUS_WRITE_FAILED;
		}
	}

	return FIO_STATUS_OK;
}


/*=======================================================================
 *
 *	uint64_t fio_DecodeLe(const uint8_t *data, size_t length)
 *
 *	Description:	Decodes an unsigned little-endian value
 *
 *	Input:		data, encoded bytes; length, number of bytes
 *
 *	Output:		Decoded value
 *
 *	Notes:		Private callers supply a length no greater than eight.
 *
 *======================================================================*/
static uint64_t
fio_DecodeLe(const uint8_t *data, size_t length)
{
	uint64_t	value;
	size_t		index;

	value = 0;

	for(index = 0; index < length; ++index) {
		value |= (uint64_t)data[index] << (index * 8);
	}

	return value;
}


/*=======================================================================
 *
 *	void fio_EncodeLe(uint64_t value, uint8_t *data, size_t length)
 *
 *	Description:	Encodes an unsigned little-endian value
 *
 *	Input:		value, value to encode; length, number of bytes
 *
 *	Output:		data, encoded bytes
 *
 *	Notes:		Private callers supply a length no greater than eight.
 *
 *======================================================================*/
static void
fio_EncodeLe(uint64_t value, uint8_t *data, size_t length)
{
	size_t index;

	for(index = 0; index < length; ++index) {
		data[index] = (uint8_t)(value >> (index * 8));
	}
}


/*=======================================================================
 *
 *	fio_Status fio_ReadU16Le(fio_Handle hnd, uint64_t offset, uint16_t *value)
 *
 *	Description:	Reads a little-endian unsigned 16-bit value
 *
 *	Input:		hnd, file handle; offset, zero-based file offset
 *
 *	Output:		value, decoded value
 *			A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		The output value is unchanged on failure.
 *
 *======================================================================*/
fio_Status
fio_ReadU16Le(fio_Handle hnd, uint64_t offset, uint16_t *value)
{
	uint8_t		data[2];
	fio_Status	status;

	if(value == NULL) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	status = fio_ReadBytes(hnd, offset, data, sizeof(data));

	if(status == FIO_STATUS_OK) {
		*value = (uint16_t)fio_DecodeLe(data, sizeof(data));
	}

	return status;
}


/*=======================================================================
 *
 *	fio_Status fio_ReadU32Le(fio_Handle hnd, uint64_t offset, uint32_t *value)
 *
 *	Description:	Reads a little-endian unsigned 32-bit value
 *
 *	Input:		hnd, file handle; offset, zero-based file offset
 *
 *	Output:		value, decoded value
 *			A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		The output value is unchanged on failure.
 *
 *======================================================================*/
fio_Status
fio_ReadU32Le(fio_Handle hnd, uint64_t offset, uint32_t *value)
{
	uint8_t		data[4];
	fio_Status	status;

	if(value == NULL) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	status = fio_ReadBytes(hnd, offset, data, sizeof(data));

	if(status == FIO_STATUS_OK) {
		*value = (uint32_t)fio_DecodeLe(data, sizeof(data));
	}

	return status;
}


/*=======================================================================
 *
 *	fio_Status fio_ReadU64Le(fio_Handle hnd, uint64_t offset, uint64_t *value)
 *
 *	Description:	Reads a little-endian unsigned 64-bit value
 *
 *	Input:		hnd, file handle; offset, zero-based file offset
 *
 *	Output:		value, decoded value
 *			A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		The output value is unchanged on failure.
 *
 *======================================================================*/
fio_Status
fio_ReadU64Le(fio_Handle hnd, uint64_t offset, uint64_t *value)
{
	uint8_t		data[8];
	fio_Status	status;

	if(value == NULL) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	status = fio_ReadBytes(hnd, offset, data, sizeof(data));

	if(status == FIO_STATUS_OK) {
		*value = fio_DecodeLe(data, sizeof(data));
	}

	return status;
}


/*=======================================================================
 *
 *	fio_Status fio_WriteU16Le(fio_Handle hnd, uint64_t offset, uint16_t value)
 *
 *	Description:	Writes a little-endian unsigned 16-bit value
 *
 *	Input:		hnd, writable file handle; offset, zero-based file offset
 *			value, value to write
 *
 *	Output:		A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		None
 *
 *======================================================================*/
fio_Status
fio_WriteU16Le(fio_Handle hnd, uint64_t offset, uint16_t value)
{
	uint8_t data[2];

	fio_EncodeLe(value, data, sizeof(data));
	return fio_WriteBytes(hnd, offset, data, sizeof(data));
}


/*=======================================================================
 *
 *	fio_Status fio_WriteU32Le(fio_Handle hnd, uint64_t offset, uint32_t value)
 *
 *	Description:	Writes a little-endian unsigned 32-bit value
 *
 *	Input:		hnd, writable file handle; offset, zero-based file offset
 *			value, value to write
 *
 *	Output:		A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		None
 *
 *======================================================================*/
fio_Status
fio_WriteU32Le(fio_Handle hnd, uint64_t offset, uint32_t value)
{
	uint8_t data[4];

	fio_EncodeLe(value, data, sizeof(data));
	return fio_WriteBytes(hnd, offset, data, sizeof(data));
}


/*=======================================================================
 *
 *	fio_Status fio_WriteU64Le(fio_Handle hnd, uint64_t offset, uint64_t value)
 *
 *	Description:	Writes a little-endian unsigned 64-bit value
 *
 *	Input:		hnd, writable file handle; offset, zero-based file offset
 *			value, value to write
 *
 *	Output:		A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		None
 *
 *======================================================================*/
fio_Status
fio_WriteU64Le(fio_Handle hnd, uint64_t offset, uint64_t value)
{
	uint8_t data[8];

	fio_EncodeLe(value, data, sizeof(data));
	return fio_WriteBytes(hnd, offset, data, sizeof(data));
}


/*=======================================================================
 *
 *	fio_Status fio_FlushFile(fio_Handle hnd)
 *
 *	Description:	Flushes pending file data to storage
 *
 *	Input:		hnd, writable file handle
 *
 *	Output:		A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		None
 *
 *======================================================================*/
fio_Status
fio_FlushFile(fio_Handle hnd)
{
	if(hnd == NULL || !hnd->writable) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	return fsync(hnd->fd) == 0 ? FIO_STATUS_OK : FIO_STATUS_FLUSH_FAILED;
}


/*=======================================================================
 *
 *	fio_Status fio_CloseFile(fio_Handle hnd)
 *
 *	Description:	Closes a file and releases its handle
 *
 *	Input:		hnd, file handle to close
 *
 *	Output:		A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		A NULL handle is accepted.
 *
 *======================================================================*/
fio_Status
fio_CloseFile(fio_Handle hnd)
{
	int result;

	if(hnd == NULL) {
		return FIO_STATUS_OK;
	}

	result = close(hnd->fd);
	free(hnd);
	return result == 0 ? FIO_STATUS_OK : FIO_STATUS_CLOSE_FAILED;
}


/*=======================================================================
 *
 *	fio_Status fio_RemoveFile(const char *filename)
 *
 *	Description:	Removes a file
 *
 *	Input:		filename, path of file to remove
 *
 *	Output:		A fio_Status value, FIO_STATUS_OK for success
 *
 *	Notes:		None
 *
 *======================================================================*/
fio_Status
fio_RemoveFile(const char *filename)
{
	if(filename == NULL) {
		return FIO_STATUS_INVALID_ARGUMENT;
	}

	return unlink(filename) == 0 ? FIO_STATUS_OK : FIO_STATUS_REMOVE_FAILED;
}


/*=======================================================================
 *
 *	const char *fio_StatusText(fio_Status status)
 *
 *	Description:	Returns a printable description of a file status
 *
 *	Input:		status, status value to describe
 *
 *	Output:		Address of a constant, null-terminated string
 *
 *	Notes:		The returned string must not be modified or freed.
 *
 *======================================================================*/
const char *
fio_StatusText(fio_Status status)
{
	switch(status) {
	case FIO_STATUS_OK:
		return "Success";

	case FIO_STATUS_INVALID_ARGUMENT:
		return "Invalid argument";

	case FIO_STATUS_NO_MEMORY:
		return "Insufficient memory";

	case FIO_STATUS_OPEN_FAILED:
		return "Unable to open file";

	case FIO_STATUS_CREATE_FAILED:
		return "Unable to create file";

	case FIO_STATUS_FILE_EXISTS:
		return "Output file already exists";

	case FIO_STATUS_FILE_STATUS_FAILED:
		return "Unable to obtain file status";

	case FIO_STATUS_OFFSET_TOO_LARGE:
		return "File offset is too large";

	case FIO_STATUS_END_OF_FILE:
		return "Unexpected end of file";

	case FIO_STATUS_READ_FAILED:
		return "File read failed";

	case FIO_STATUS_WRITE_FAILED:
		return "File write failed";

	case FIO_STATUS_SET_LENGTH_FAILED:
		return "Unable to set file length";

	case FIO_STATUS_FLUSH_FAILED:
		return "Unable to flush file";

	case FIO_STATUS_CLOSE_FAILED:
		return "Unable to close file";

	case FIO_STATUS_REMOVE_FAILED:
		return "Unable to remove file";

	default:
		return "Unknown file status";
	}
}
