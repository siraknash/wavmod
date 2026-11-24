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
 * file_io.h
 *
 * Offset-based binary file input and output.
 *
 ***************************************************************************/

#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h>
#include <stdint.h>


/***************************************************************************
 *
 * Public types
 *
 ***************************************************************************/

typedef struct fio_Control	*fio_Handle;

typedef enum {
	FIO_STATUS_OK = 0,
	FIO_STATUS_INVALID_ARGUMENT,
	FIO_STATUS_NO_MEMORY,
	FIO_STATUS_OPEN_FAILED,
	FIO_STATUS_CREATE_FAILED,
	FIO_STATUS_FILE_EXISTS,
	FIO_STATUS_FILE_STATUS_FAILED,
	FIO_STATUS_OFFSET_TOO_LARGE,
	FIO_STATUS_END_OF_FILE,
	FIO_STATUS_READ_FAILED,
	FIO_STATUS_WRITE_FAILED,
	FIO_STATUS_SET_LENGTH_FAILED,
	FIO_STATUS_FLUSH_FAILED,
	FIO_STATUS_CLOSE_FAILED,
	FIO_STATUS_REMOVE_FAILED
} fio_Status;


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
extern fio_Status
fio_OpenFile(const char *filename, fio_Handle *hnd);


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
extern fio_Status
fio_CreateFile(const char *filename, fio_Handle *hnd);


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
extern fio_Status
fio_GetFileLength(fio_Handle hnd, uint64_t *length);


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
extern fio_Status
fio_SetFileLength(fio_Handle hnd, uint64_t length);


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
extern fio_Status
fio_ReadBytes(fio_Handle hnd, uint64_t offset, void *data, size_t length);


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
extern fio_Status
fio_WriteBytes(fio_Handle hnd, uint64_t offset, const void *data, size_t length);


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
extern fio_Status
fio_ReadU16Le(fio_Handle hnd, uint64_t offset, uint16_t *value);


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
extern fio_Status
fio_ReadU32Le(fio_Handle hnd, uint64_t offset, uint32_t *value);


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
extern fio_Status
fio_ReadU64Le(fio_Handle hnd, uint64_t offset, uint64_t *value);


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
extern fio_Status
fio_WriteU16Le(fio_Handle hnd, uint64_t offset, uint16_t value);


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
extern fio_Status
fio_WriteU32Le(fio_Handle hnd, uint64_t offset, uint32_t value);


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
extern fio_Status
fio_WriteU64Le(fio_Handle hnd, uint64_t offset, uint64_t value);


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
extern fio_Status
fio_FlushFile(fio_Handle hnd);


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
extern fio_Status
fio_CloseFile(fio_Handle hnd);


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
extern fio_Status
fio_RemoveFile(const char *filename);


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
extern const char *
fio_StatusText(fio_Status status);


#endif
