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
 * wav_file.c
 *
 * RIFF/WAVE parsing and validation.
 *
 ***************************************************************************/

#include "wav_file.h"

#include "file_io.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>


/***************************************************************************
 *
 * Private constants
 *
 ***************************************************************************/

#define WAV_FORMAT_PCM			0x0001
#define WAV_FORMAT_ALAW			0x0006
#define WAV_FORMAT_MULAW			0x0007
#define WAV_FORMAT_IEEE_FLOAT	0x0003
#define WAV_FORMAT_EXTENSIBLE	0xfffe


/***************************************************************************
 *
 * Private types
 *
 ***************************************************************************/

typedef struct wav_Control wav_Control;

struct wav_Control {
	fio_Handle		file_hnd;
	uint64_t		file_length;
	uint64_t		data_offset;
	uint64_t		data_length;
	wav_Properties	properties;
};


/*=======================================================================
 *
 *	uint16_t wav_DecodeU16(const uint8_t *data)
 *
 *	Description:	Decodes an unsigned little-endian 16-bit value
 *
 *	Input:		data, address of encoded value
 *
 *	Output:		Decoded value
 *
 *	Notes:		The caller verifies that two bytes are available.
 *
 *======================================================================*/
static uint16_t
wav_DecodeU16(const uint8_t *data)
{
	return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}


/*=======================================================================
 *
 *	uint32_t wav_DecodeU32(const uint8_t *data)
 *
 *	Description:	Decodes an unsigned little-endian 32-bit value
 *
 *	Input:		data, address of encoded value
 *
 *	Output:		Decoded value
 *
 *	Notes:		The caller verifies that four bytes are available.
 *
 *======================================================================*/
static uint32_t
wav_DecodeU32(const uint8_t *data)
{
	return ((uint32_t)data[0] | ((uint32_t)data[1] << 8) |
		((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24));
}


/*=======================================================================
 *
 *	wav_Status wav_MapFileStatus(fio_Status status)
 *
 *	Description:	Converts a file API status into a WAVE status
 *
 *	Input:		status, file API status
 *
 *	Output:		Equivalent WAVE status
 *
 *	Notes:		None
 *
 *======================================================================*/
static wav_Status
wav_MapFileStatus(fio_Status status)
{
	if(status == FIO_STATUS_END_OF_FILE) {
		return WAV_STATUS_TRUNCATED_FILE;
	}

	if(status == FIO_STATUS_NO_MEMORY) {
		return WAV_STATUS_NO_MEMORY;
	}

	return WAV_STATUS_READ_FAILED;
}


/*=======================================================================
 *
 *	int wav_GetExtensibleFormat(const uint8_t *guid, uint16_t *format_tag)
 *
 *	Description:	Validates a WAVE extensible sub-format identifier
 *
 *	Input:		guid, address of 16-byte sub-format identifier
 *
 *	Output:		format_tag, underlying WAVE format tag
 *			Non-zero for a recognized WAVE sub-format identifier
 *
 *	Notes:		The caller verifies that 16 bytes are available.
 *
 *======================================================================*/
static int
wav_GetExtensibleFormat(const uint8_t *guid, uint16_t *format_tag)
{
	static const uint8_t guid_tail[14] = {
		0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80,
		0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
	};

	if(memcmp(guid + 2, guid_tail, sizeof(guid_tail)) != 0) {
		return 0;
	}

	*format_tag = wav_DecodeU16(guid);
	return 1;
}


/*=======================================================================
 *
 *	wav_Status wav_ParseFormat(wav_Control *ctrl, const uint8_t *data, size_t length)
 *
 *	Description:	Parses and validates a WAVE format chunk
 *
 *	Input:		ctrl, control structure receiving audio properties
 *			data, address of format chunk data
 *			length, number of available format bytes
 *
 *	Output:		A wav_Status value, WAV_STATUS_OK for success
 *
 *	Notes:		PCM, IEEE floating-point, and G.711 formats are supported.
 *
 *======================================================================*/
static wav_Status
wav_ParseFormat(wav_Control *ctrl, const uint8_t *data, size_t length)
{
	uint64_t	expected_byte_rate;
	uint32_t	expected_block_align;
	uint32_t	byte_rate;
	uint32_t	sample_rate;
	uint32_t	channel_mask;
	uint16_t	bits_per_sample;
	uint16_t	block_align;
	uint16_t	channel_count;
	uint16_t	extension_size;
	uint16_t	format_tag;
	uint16_t	effective_format;
	uint16_t	valid_bits;

	if(length < 16) {
		return WAV_STATUS_INVALID_FORMAT_CHUNK;
	}

	format_tag = wav_DecodeU16(data);
	channel_count = wav_DecodeU16(data + 2);
	sample_rate = wav_DecodeU32(data + 4);
	byte_rate = wav_DecodeU32(data + 8);
	block_align = wav_DecodeU16(data + 12);
	bits_per_sample = wav_DecodeU16(data + 14);
	channel_mask = 0;
	valid_bits = bits_per_sample;
	effective_format = format_tag;

	if(format_tag == WAV_FORMAT_EXTENSIBLE) {
		if(length < 40) {
			return WAV_STATUS_INVALID_FORMAT_CHUNK;
		}

		extension_size = wav_DecodeU16(data + 16);

		if(extension_size < 22 || extension_size > length - 18) {
			return WAV_STATUS_INVALID_FORMAT_CHUNK;
		}

		valid_bits = wav_DecodeU16(data + 18);
		channel_mask = wav_DecodeU32(data + 20);

		if(!wav_GetExtensibleFormat(data + 24, &effective_format)) {
			return WAV_STATUS_UNSUPPORTED_ENCODING;
		}
	}

	if(channel_count == 0 || sample_rate == 0 || block_align == 0 || bits_per_sample == 0 ||
			(bits_per_sample & 0x07) != 0 || valid_bits == 0 || valid_bits > bits_per_sample) {
		return WAV_STATUS_INVALID_FORMAT_CHUNK;
	}

	if(effective_format == WAV_FORMAT_PCM) {
		if(bits_per_sample != 8 && bits_per_sample != 16 && bits_per_sample != 24 &&
				bits_per_sample != 32) {
			return WAV_STATUS_UNSUPPORTED_SAMPLE_FORMAT;
		}

		ctrl->properties.encoding = WAV_ENCODING_PCM;
	}
	else if(effective_format == WAV_FORMAT_IEEE_FLOAT) {
		if((bits_per_sample != 32 && bits_per_sample != 64) || valid_bits != bits_per_sample) {
			return WAV_STATUS_UNSUPPORTED_SAMPLE_FORMAT;
		}

		ctrl->properties.encoding = WAV_ENCODING_IEEE_FLOAT;
	}
	else if(effective_format == WAV_FORMAT_ALAW) {
		if(bits_per_sample != 8 || valid_bits != bits_per_sample) {
			return WAV_STATUS_UNSUPPORTED_SAMPLE_FORMAT;
		}

		ctrl->properties.encoding = WAV_ENCODING_G711_ALAW;
	}
	else if(effective_format == WAV_FORMAT_MULAW) {
		if(bits_per_sample != 8 || valid_bits != bits_per_sample) {
			return WAV_STATUS_UNSUPPORTED_SAMPLE_FORMAT;
		}

		ctrl->properties.encoding = WAV_ENCODING_G711_MULAW;
	}
	else {
		return WAV_STATUS_UNSUPPORTED_ENCODING;
	}

	expected_block_align = (uint32_t)channel_count * (uint32_t)(bits_per_sample / 8);

	if(expected_block_align > UINT16_MAX || block_align != expected_block_align) {
		return WAV_STATUS_INVALID_FORMAT_CHUNK;
	}

	expected_byte_rate = (uint64_t)sample_rate * block_align;

	if(expected_byte_rate > UINT32_MAX || byte_rate != expected_byte_rate) {
		return WAV_STATUS_INVALID_FORMAT_CHUNK;
	}

	ctrl->properties.format_tag = format_tag;
	ctrl->properties.channel_count = channel_count;
	ctrl->properties.sample_rate = sample_rate;
	ctrl->properties.byte_rate = byte_rate;
	ctrl->properties.block_align = block_align;
	ctrl->properties.bits_per_sample = bits_per_sample;
	ctrl->properties.valid_bits_per_sample = valid_bits;
	ctrl->properties.channel_mask = channel_mask;
	return WAV_STATUS_OK;
}


/*=======================================================================
 *
 *	wav_Status wav_ValidateFile(wav_Control *ctrl)
 *
 *	Description:	Validates a RIFF/WAVE file and records its audio layout
 *
 *	Input:		ctrl, control structure containing an open file
 *
 *	Output:		A wav_Status value, WAV_STATUS_OK for success
 *
 *	Notes:		Unknown chunks are preserved by the writer and skipped here.
 *
 *======================================================================*/
static wav_Status
wav_ValidateFile(wav_Control *ctrl)
{
	uint8_t		format_data[40];
	uint8_t		header[12];
	uint8_t		chunk_header[8];
	uint64_t	chunk_data_offset;
	uint64_t	chunk_end;
	uint64_t	chunk_length;
	uint64_t	offset;
	uint64_t	riff_end;
	uint32_t	riff_length;
	fio_Status	file_status;
	wav_Status	status;
	int			data_found;
	int			format_found;

	if(ctrl->file_length < 4) {
		return WAV_STATUS_TRUNCATED_FILE;
	}

	file_status = fio_ReadBytes(ctrl->file_hnd, 0, header, ctrl->file_length < sizeof(header) ?
		(size_t)ctrl->file_length : sizeof(header));

	if(file_status != FIO_STATUS_OK) {
		return wav_MapFileStatus(file_status);
	}

	if(memcmp(header, "RIFX", 4) == 0) {
		return WAV_STATUS_RIFX_NOT_SUPPORTED;
	}

	if(memcmp(header, "RF64", 4) == 0) {
		return WAV_STATUS_RF64_NOT_SUPPORTED;
	}

	if(memcmp(header, "RIFF", 4) != 0) {
		return WAV_STATUS_RIFF_HEADER_NOT_FOUND;
	}

	if(ctrl->file_length < sizeof(header)) {
		return WAV_STATUS_TRUNCATED_FILE;
	}

	if(memcmp(header + 8, "WAVE", 4) != 0) {
		return WAV_STATUS_WAVE_HEADER_NOT_FOUND;
	}

	riff_length = wav_DecodeU32(header + 4);
	riff_end = (uint64_t)riff_length + 8;

	if(riff_end > ctrl->file_length) {
		return WAV_STATUS_TRUNCATED_FILE;
	}

	if(riff_end < 12) {
		return WAV_STATUS_INVALID_FILE;
	}

	offset = 12;
	data_found = 0;
	format_found = 0;

	while(offset < riff_end) {
		if(riff_end - offset < sizeof(chunk_header)) {
			return WAV_STATUS_INVALID_CHUNK_LENGTH;
		}

		file_status = fio_ReadBytes(ctrl->file_hnd, offset, chunk_header, sizeof(chunk_header));

		if(file_status != FIO_STATUS_OK) {
			return wav_MapFileStatus(file_status);
		}

		chunk_length = wav_DecodeU32(chunk_header + 4);
		chunk_data_offset = offset + sizeof(chunk_header);
		chunk_end = chunk_data_offset + chunk_length;

		if(chunk_end < chunk_data_offset || chunk_end > riff_end) {
			return WAV_STATUS_INVALID_CHUNK_LENGTH;
		}

		if(memcmp(chunk_header, "fmt ", 4) == 0) {
			if(format_found || chunk_length < 16) {
				return WAV_STATUS_INVALID_FORMAT_CHUNK;
			}

			file_status = fio_ReadBytes(ctrl->file_hnd, chunk_data_offset, format_data,
				chunk_length < sizeof(format_data) ? (size_t)chunk_length : sizeof(format_data));

			if(file_status != FIO_STATUS_OK) {
				return wav_MapFileStatus(file_status);
			}

			status = wav_ParseFormat(ctrl, format_data,
				chunk_length < sizeof(format_data) ? (size_t)chunk_length : sizeof(format_data));

			if(status != WAV_STATUS_OK) {
				return status;
			}

			format_found = 1;
		}
		else if(memcmp(chunk_header, "data", 4) == 0) {
			if(data_found) {
				return WAV_STATUS_INVALID_FILE;
			}

			ctrl->data_offset = chunk_data_offset;
			ctrl->data_length = chunk_length;
			data_found = 1;
		}

		offset = chunk_end + (chunk_length & 1);

		if(offset < chunk_end || offset > riff_end) {
			return WAV_STATUS_INVALID_CHUNK_LENGTH;
		}
	}

	if(!format_found) {
		return WAV_STATUS_FORMAT_CHUNK_NOT_FOUND;
	}

	if(!data_found) {
		return WAV_STATUS_DATA_CHUNK_NOT_FOUND;
	}

	if(ctrl->data_length % ctrl->properties.block_align != 0) {
		return WAV_STATUS_INVALID_CHUNK_LENGTH;
	}

	ctrl->properties.data_byte_count = ctrl->data_length;
	ctrl->properties.frame_count = ctrl->data_length / ctrl->properties.block_align;
	return WAV_STATUS_OK;
}


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
wav_Status
wav_Open(const char *filename, wav_Handle *hnd)
{
	wav_Control	*ctrl;
	fio_Status	file_status;
	wav_Status	status;

	if(hnd != NULL) {
		*hnd = NULL;
	}

	if(filename == NULL || hnd == NULL) {
		return WAV_STATUS_INVALID_ARGUMENT;
	}

	ctrl = calloc(1, sizeof(*ctrl));

	if(ctrl == NULL) {
		return WAV_STATUS_NO_MEMORY;
	}

	file_status = fio_OpenFile(filename, &ctrl->file_hnd);

	if(file_status != FIO_STATUS_OK) {
		free(ctrl);
		return file_status == FIO_STATUS_NO_MEMORY ? WAV_STATUS_NO_MEMORY : WAV_STATUS_OPEN_FAILED;
	}

	file_status = fio_GetFileLength(ctrl->file_hnd, &ctrl->file_length);

	if(file_status != FIO_STATUS_OK) {
		wav_Close(ctrl);
		return WAV_STATUS_READ_FAILED;
	}

	status = wav_ValidateFile(ctrl);

	if(status != WAV_STATUS_OK) {
		wav_Close(ctrl);
		return status;
	}

	*hnd = ctrl;
	return WAV_STATUS_OK;
}


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
wav_Status
wav_GetProperties(wav_Handle hnd, wav_Properties *properties)
{
	if(hnd == NULL || properties == NULL) {
		return WAV_STATUS_INVALID_ARGUMENT;
	}

	*properties = hnd->properties;
	return WAV_STATUS_OK;
}


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
wav_Status
wav_GetFileLayout(wav_Handle hnd, wav_FileLayout *layout)
{
	if(hnd == NULL || layout == NULL) {
		return WAV_STATUS_INVALID_ARGUMENT;
	}

	layout->file_byte_count = hnd->file_length;
	layout->audio_byte_offset = hnd->data_offset;
	layout->audio_byte_count = hnd->data_length;
	return WAV_STATUS_OK;
}


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
wav_Status
wav_ReadFileBytes(wav_Handle hnd, uint64_t offset, void *data, size_t length)
{
	fio_Status status;

	if(hnd == NULL || (data == NULL && length != 0) || offset > hnd->file_length ||
			(uint64_t)length > hnd->file_length - offset) {
		return WAV_STATUS_INVALID_ARGUMENT;
	}

	status = fio_ReadBytes(hnd->file_hnd, offset, data, length);
	return status == FIO_STATUS_OK ? WAV_STATUS_OK : wav_MapFileStatus(status);
}


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
wav_Status
wav_ReadAudioBytes(wav_Handle hnd, uint64_t offset, void *data, size_t length)
{
	if(hnd == NULL || (data == NULL && length != 0) || offset > hnd->data_length ||
			(uint64_t)length > hnd->data_length - offset) {
		return WAV_STATUS_INVALID_ARGUMENT;
	}

	return wav_ReadFileBytes(hnd, hnd->data_offset + offset, data, length);
}


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
const char *
wav_StatusText(wav_Status status)
{
	switch(status) {
	case WAV_STATUS_OK:
		return "Success";

	case WAV_STATUS_INVALID_ARGUMENT:
		return "Invalid argument";

	case WAV_STATUS_OPEN_FAILED:
		return "Unable to open file";

	case WAV_STATUS_NO_MEMORY:
		return "Insufficient memory";

	case WAV_STATUS_INVALID_FILE:
		return "Invalid WAVE file";

	case WAV_STATUS_RIFF_HEADER_NOT_FOUND:
		return "RIFF header not found";

	case WAV_STATUS_WAVE_HEADER_NOT_FOUND:
		return "WAVE header not found";

	case WAV_STATUS_FORMAT_CHUNK_NOT_FOUND:
		return "WAVE format chunk not found";

	case WAV_STATUS_DATA_CHUNK_NOT_FOUND:
		return "WAVE data chunk not found";

	case WAV_STATUS_INVALID_FORMAT_CHUNK:
		return "Invalid WAVE format chunk";

	case WAV_STATUS_INVALID_CHUNK_LENGTH:
		return "Invalid WAVE chunk length";

	case WAV_STATUS_TRUNCATED_FILE:
		return "Truncated WAVE file";

	case WAV_STATUS_RIFX_NOT_SUPPORTED:
		return "RIFX files are not supported";

	case WAV_STATUS_RF64_NOT_SUPPORTED:
		return "RF64 files are not supported";

	case WAV_STATUS_ALAW_NOT_SUPPORTED:
		return "G.711 A-law audio is not supported";

	case WAV_STATUS_MULAW_NOT_SUPPORTED:
		return "G.711 mu-law audio is not supported";

	case WAV_STATUS_UNSUPPORTED_ENCODING:
		return "Unsupported WAVE audio encoding";

	case WAV_STATUS_UNSUPPORTED_SAMPLE_FORMAT:
		return "Unsupported WAVE sample format";

	case WAV_STATUS_READ_FAILED:
		return "Unable to read WAVE file";

	default:
		return "Unknown WAVE status";
	}
}


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
void
wav_Close(wav_Handle hnd)
{
	if(hnd == NULL) {
		return;
	}

	fio_CloseFile(hnd->file_hnd);
	free(hnd);
}
