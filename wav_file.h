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
 * wav_file.h
 *
 * Interface for opening and examining RIFF/WAVE audio files.
 *
 ***************************************************************************/

#ifndef WAV_FILE_H
#define WAV_FILE_H

#include <stddef.h>
#include <stdint.h>


/***************************************************************************
 *
 * Public types
 *
 ***************************************************************************/

typedef struct wav_Control	*wav_Handle;

typedef enum {
	WAV_STATUS_OK = 0,
	WAV_STATUS_INVALID_ARGUMENT,
	WAV_STATUS_OPEN_FAILED,
	WAV_STATUS_NO_MEMORY,
	WAV_STATUS_INVALID_FILE,
	WAV_STATUS_RIFF_HEADER_NOT_FOUND,
	WAV_STATUS_WAVE_HEADER_NOT_FOUND,
	WAV_STATUS_FORMAT_CHUNK_NOT_FOUND,
	WAV_STATUS_DATA_CHUNK_NOT_FOUND,
	WAV_STATUS_INVALID_FORMAT_CHUNK,
	WAV_STATUS_INVALID_CHUNK_LENGTH,
	WAV_STATUS_TRUNCATED_FILE,
	WAV_STATUS_RIFX_NOT_SUPPORTED,
	WAV_STATUS_RF64_NOT_SUPPORTED,
	WAV_STATUS_ALAW_NOT_SUPPORTED,
	WAV_STATUS_MULAW_NOT_SUPPORTED,
	WAV_STATUS_UNSUPPORTED_ENCODING,
	WAV_STATUS_UNSUPPORTED_SAMPLE_FORMAT,
	WAV_STATUS_READ_FAILED
} wav_Status;

typedef enum {
	WAV_ENCODING_UNKNOWN = 0,
	WAV_ENCODING_PCM,
	WAV_ENCODING_IEEE_FLOAT,
	WAV_ENCODING_G711_ALAW,
	WAV_ENCODING_G711_MULAW
} wav_Encoding;

typedef struct {
	wav_Encoding	encoding;
	uint16_t		format_tag;
	uint16_t		channel_count;
	uint32_t		sample_rate;
	uint32_t		byte_rate;
	uint16_t		block_align;
	uint16_t		bits_per_sample;
	uint16_t		valid_bits_per_sample;
	uint32_t		channel_mask;
	uint64_t		frame_count;
	uint64_t		data_byte_count;
} wav_Properties;

typedef struct {
	uint64_t	file_byte_count;
	uint64_t	audio_byte_offset;
	uint64_t	audio_byte_count;
} wav_FileLayout;


/*=======================================================================
 *
 *	wav_Status wav_Open(const char *filename, wav_Handle *hnd)
 *
 *	Description:	Opens and validates a RIFF/WAVE file
 *
 *	Input:		filename, path of file to open
 *
 *	Output:		hnd, handle used for subsequent operations, or NULL
 *			if the operation fails
 *			A wav_Status value, WAV_STATUS_OK for success
 *
 *	Notes:		The returned handle must be released using wav_Close().
 *
 *======================================================================*/
extern wav_Status
wav_Open(const char *filename, wav_Handle *hnd);


/*=======================================================================
 *
 *	wav_Status wav_GetProperties(wav_Handle hnd, wav_Properties *properties)
 *
 *	Description:	Returns the audio properties found while opening a file
 *
 *	Input:		hnd, handle returned by wav_Open()
 *
 *	Output:		properties, structure receiving the audio properties
 *			A wav_Status value, WAV_STATUS_OK for success
 *
 *	Notes:		None
 *
 *======================================================================*/
extern wav_Status
wav_GetProperties(wav_Handle hnd, wav_Properties *properties);


/*=======================================================================
 *
 *	wav_Status wav_GetFileLayout(wav_Handle hnd, wav_FileLayout *layout)
 *
 *	Description:	Returns file and audio data locations
 *
 *	Input:		hnd, handle returned by wav_Open()
 *
 *	Output:		layout, structure receiving file layout information
 *			A wav_Status value, WAV_STATUS_OK for success
 *
 *	Notes:		Offsets and lengths are measured in bytes.
 *
 *======================================================================*/
extern wav_Status
wav_GetFileLayout(wav_Handle hnd, wav_FileLayout *layout);


/*=======================================================================
 *
 *	wav_Status wav_ReadFileBytes(wav_Handle hnd, uint64_t offset, void *data, size_t length)
 *
 *	Description:	Reads bytes from the source WAVE file
 *
 *	Input:		hnd, handle returned by wav_Open()
 *			offset, zero-based file offset
 *			length, number of bytes to read
 *
 *	Output:		data, destination buffer
 *			A wav_Status value, WAV_STATUS_OK for success
 *
 *	Notes:		The requested range must be inside the source file.
 *
 *======================================================================*/
extern wav_Status
wav_ReadFileBytes(wav_Handle hnd, uint64_t offset, void *data, size_t length);


/*=======================================================================
 *
 *	wav_Status wav_ReadAudioBytes(wav_Handle hnd, uint64_t offset, void *data, size_t length)
 *
 *	Description:	Reads bytes relative to the start of WAVE audio data
 *
 *	Input:		hnd, handle returned by wav_Open()
 *			offset, byte offset within the audio data
 *			length, number of bytes to read
 *
 *	Output:		data, destination buffer
 *			A wav_Status value, WAV_STATUS_OK for success
 *
 *	Notes:		The requested range must be inside the data chunk.
 *
 *======================================================================*/
extern wav_Status
wav_ReadAudioBytes(wav_Handle hnd, uint64_t offset, void *data, size_t length);


/*=======================================================================
 *
 *	const char *wav_StatusText(wav_Status status)
 *
 *	Description:	Returns a printable description of a status value
 *
 *	Input:		status, status value to describe
 *
 *	Output:		Address of a constant, null-terminated string
 *
 *	Notes:		The returned string must not be modified or freed.
 *
 *======================================================================*/
extern const char *
wav_StatusText(wav_Status status);


/*=======================================================================
 *
 *	void wav_Close(wav_Handle hnd)
 *
 *	Description:	Closes a WAVE file and releases its resources
 *
 *	Input:		hnd, handle of WAVE file to close
 *
 *	Output:		None
 *
 *	Notes:		A NULL handle is ignored.
 *
 *======================================================================*/
extern void
wav_Close(wav_Handle hnd);


#endif
