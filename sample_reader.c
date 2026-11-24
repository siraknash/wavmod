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
 * sample_reader.c
 *
 * WAVE sample decoding into a temporary double store.
 *
 ***************************************************************************/

#include "sample_reader.h"

#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/***************************************************************************
 *
 * Private constants
 *
 ***************************************************************************/

#define SAMPLE_READER_BUFFER_SIZE	65536


/*=======================================================================
 *
 *	uint64_t sampleReader_DecodeUnsigned(const uint8_t *data, uint16_t byte_count)
 *
 *	Description:	Decodes an unsigned little-endian value
 *
 *	Input:		data, address of encoded bytes
 *			byte_count, number of bytes to decode
 *
 *	Output:		Decoded unsigned value
 *
 *	Notes:		The caller supplies no more than eight bytes.
 *
 *======================================================================*/
static uint64_t
sampleReader_DecodeUnsigned(const uint8_t *data, uint16_t byte_count)
{
	uint64_t	value;
	uint16_t	index;

	value = 0;

	for(index = 0; index < byte_count; ++index) {
		value |= (uint64_t)data[index] << (index * 8);
	}

	return value;
}


/*=======================================================================
 *
 *	double sampleReader_DecodePcm(const wav_Properties *properties, const uint8_t *data)
 *
 *	Description:	Decodes and normalizes an integer PCM sample
 *
 *	Input:		properties, WAVE sample properties
 *			data, address of encoded sample bytes
 *
 *	Output:		Normalized double sample value
 *
 *	Notes:		Eight-bit PCM is unsigned. Wider PCM is signed.
 *
 *======================================================================*/
static double
sampleReader_DecodePcm(const wav_Properties *properties, const uint8_t *data)
{
	uint64_t	denominator;
	uint64_t	raw_value;
	uint64_t	sign_range;
	uint16_t	sample_byte_count;
	uint16_t	shift_count;
	int64_t		signed_value;

	sample_byte_count = properties->bits_per_sample / 8;
	raw_value = sampleReader_DecodeUnsigned(data, sample_byte_count);
	shift_count = properties->bits_per_sample - properties->valid_bits_per_sample;
	raw_value >>= shift_count;
	denominator = UINT64_C(1) << (properties->valid_bits_per_sample - 1);

	if(properties->bits_per_sample == 8) {
		signed_value = (int64_t)raw_value - (int64_t)denominator;
	}
	else {
		sign_range = denominator << 1;
		signed_value = raw_value & denominator ? (int64_t)(raw_value - sign_range) :
			(int64_t)raw_value;
	}

	return (double)signed_value / (double)denominator;
}


/*=======================================================================
 *
 *	double sampleReader_DecodeALaw(uint8_t encoded)
 *
 *	Description:	Decodes a G.711 A-law sample
 *
 *	Input:		encoded, eight-bit A-law sample
 *
 *	Output:		Decoded double sample value
 *
 *	Notes:		The linear value uses the signed 16-bit PCM scale.
 *
 *======================================================================*/
static double
sampleReader_DecodeALaw(uint8_t encoded)
{
	int32_t	magnitude;
	uint8_t	exponent;
	uint8_t	value;

	value = encoded ^ 0x55;
	magnitude = (value & 0x0f) << 4;
	exponent = (value >> 4) & 0x07;

	if(exponent == 0) {
		magnitude += 8;
	}
	else if(exponent == 1) {
		magnitude += 0x108;
	}
	else {
		magnitude = (magnitude + 0x108) << (exponent - 1);
	}

	if(!(value & 0x80)) {
		magnitude = -magnitude;
	}

	return (double)magnitude / 32768.0;
}


/*=======================================================================
 *
 *	double sampleReader_DecodeMuLaw(uint8_t encoded)
 *
 *	Description:	Decodes a G.711 mu-law sample
 *
 *	Input:		encoded, eight-bit mu-law sample
 *
 *	Output:		Decoded double sample value
 *
 *	Notes:		The linear value uses the signed 16-bit PCM scale.
 *
 *======================================================================*/
static double
sampleReader_DecodeMuLaw(uint8_t encoded)
{
static int count = 0;

	int32_t	magnitude;
	uint8_t	value;

	value = (uint8_t)~encoded;
	magnitude = (((value & 0x0f) << 3) + 0x84) << ((value >> 4) & 0x07);
	magnitude -= 0x84;

	if(value & 0x80) {
		magnitude = -magnitude;
	}

if(++count >= 65536 && count < 66000) {
printf(" %i  encoded %lu, mag = %.09e\n", count, (unsigned long)encoded, (double)magnitude / 32768.0);
}
	return (double)magnitude / 32768.0;
}



/*=======================================================================
 *
 *	int sampleReader_FloatSupported(void)
 *
 *	Description:	Checks the host single-precision representation
 *
 *	Input:		None
 *
 *	Output:		Non-zero when IEEE binary32 values can be decoded
 *
 *	Notes:		Byte order is handled separately from representation.
 *
 *======================================================================*/
static int
sampleReader_FloatSupported(void)
{
	float		one;
	uint32_t	bits;

	if(sizeof(float) != 4 || FLT_RADIX != 2 || FLT_MANT_DIG != 24 || FLT_MAX_EXP != 128) {
		return 0;
	}

	one = 1.0f;
	memcpy(&bits, &one, sizeof(bits));
	return bits == UINT32_C(0x3f800000);
}


/*=======================================================================
 *
 *	int sampleReader_DoubleSupported(void)
 *
 *	Description:	Checks the host double-precision representation
 *
 *	Input:		None
 *
 *	Output:		Non-zero when IEEE binary64 values can be decoded
 *
 *	Notes:		Byte order is handled separately from representation.
 *
 *======================================================================*/
static int
sampleReader_DoubleSupported(void)
{
	double		one;
	uint64_t	bits;

	if(sizeof(double) != 8 || FLT_RADIX != 2 || DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024) {
		return 0;
	}

	one = 1.0;
	memcpy(&bits, &one, sizeof(bits));
	return bits == UINT64_C(0x3ff0000000000000);
}


/*=======================================================================
 *
 *	double sampleReader_DecodeFloat(const uint8_t *data)
 *
 *	Description:	Decodes a little-endian IEEE binary32 sample
 *
 *	Input:		data, address of four encoded bytes
 *
 *	Output:		Sample value converted to double
 *
 *	Notes:		The host representation is validated before use.
 *
 *======================================================================*/
static double
sampleReader_DecodeFloat(const uint8_t *data)
{
	float		value;
	uint32_t	bits;

	bits = (uint32_t)sampleReader_DecodeUnsigned(data, 4);
	memcpy(&value, &bits, sizeof(value));
	return value;
}


/*=======================================================================
 *
 *	double sampleReader_DecodeDouble(const uint8_t *data)
 *
 *	Description:	Decodes a little-endian IEEE binary64 sample
 *
 *	Input:		data, address of eight encoded bytes
 *
 *	Output:		Decoded double sample value
 *
 *	Notes:		The host representation is validated before use.
 *
 *======================================================================*/
static double
sampleReader_DecodeDouble(const uint8_t *data)
{
	double		value;
	uint64_t	bits;

	bits = sampleReader_DecodeUnsigned(data, 8);
	memcpy(&value, &bits, sizeof(value));
	return value;
}


/*=======================================================================
 *
 *	sampleReader_Status sampleReader_Load(wav_Handle wav_hnd, sampleStore_Handle *store_hnd)
 *
 *	Description:	Decodes every WAVE sample into temporary double storage
 *
 *	Input:		wav_hnd, handle returned by wav_Open()
 *
 *	Output:		store_hnd, decoded sample store, or NULL on failure
 *			A sampleReader_Status value, SAMPLE_READER_STATUS_OK for success
 *
 *	Notes:		Integer PCM is normalized. Floating-point values are not
 *			scaled or clamped.
 *
 *======================================================================*/
sampleReader_Status
sampleReader_Load(wav_Handle wav_hnd, sampleStore_Handle *store_hnd)
{
	double				*samples;
	uint8_t				*buffer;
	uint64_t			frame_base;
	uint64_t			frame_count;
	uint64_t			frame_index;
	uint64_t			frames_in_buffer;
	uint64_t			frames_per_buffer;
	uint64_t			sample_count;
	uint64_t			sample_index;
	size_t				byte_count;
	size_t				byte_offset;
	uint16_t			channel_index;
	uint16_t			sample_byte_count;
	sampleStore_Handle	store;
	sampleStore_Status	store_status;
	wav_Properties		properties;
	wav_Status			wav_status;

	if(store_hnd != NULL) {
		*store_hnd = NULL;
	}

	if(wav_hnd == NULL || store_hnd == NULL) {
		return SAMPLE_READER_STATUS_INVALID_ARGUMENT;
	}

	wav_status = wav_GetProperties(wav_hnd, &properties);

	if(wav_status != WAV_STATUS_OK) {
		return SAMPLE_READER_STATUS_WAVE_ERROR;
	}

	if(properties.encoding == WAV_ENCODING_IEEE_FLOAT &&
			((properties.bits_per_sample == 32 && !sampleReader_FloatSupported()) ||
			(properties.bits_per_sample == 64 && !sampleReader_DoubleSupported()))) {
		return SAMPLE_READER_STATUS_UNSUPPORTED_FLOAT;
	}

	store_status = sampleStore_Create(properties.frame_count, properties.channel_count, &store);

	if(store_status != SAMPLE_STORE_STATUS_OK) {
		return store_status == SAMPLE_STORE_STATUS_NO_MEMORY ? SAMPLE_READER_STATUS_NO_MEMORY :
			SAMPLE_READER_STATUS_STORE_ERROR;
	}

	store_status = sampleStore_GetData(store, &samples, &sample_count);

	if(store_status != SAMPLE_STORE_STATUS_OK) {
		sampleStore_Close(store);
		return SAMPLE_READER_STATUS_STORE_ERROR;
	}

	if(sample_count == 0) {
		*store_hnd = store;
		return SAMPLE_READER_STATUS_OK;
	}

	sample_byte_count = properties.bits_per_sample / 8;
	frames_per_buffer = SAMPLE_READER_BUFFER_SIZE / properties.block_align;

	if(frames_per_buffer == 0) {
		frames_per_buffer = 1;
	}

	byte_count = (size_t)(frames_per_buffer * properties.block_align);
	buffer = malloc(byte_count);

	if(buffer == NULL) {
		sampleStore_Close(store);
		return SAMPLE_READER_STATUS_NO_MEMORY;
	}

	frame_count = properties.frame_count;

	for(frame_base = 0; frame_base < frame_count; frame_base += frames_in_buffer) {
		frames_in_buffer = frame_count - frame_base;

		if(frames_in_buffer > frames_per_buffer) {
			frames_in_buffer = frames_per_buffer;
		}

		byte_count = (size_t)(frames_in_buffer * properties.block_align);
		wav_status = wav_ReadAudioBytes(wav_hnd, frame_base * properties.block_align, buffer,
			byte_count);

		if(wav_status != WAV_STATUS_OK) {
			free(buffer);
			sampleStore_Close(store);
			return SAMPLE_READER_STATUS_WAVE_ERROR;
		}

		for(frame_index = 0; frame_index < frames_in_buffer; ++frame_index) {
			for(channel_index = 0; channel_index < properties.channel_count; ++channel_index) {
				byte_offset = (size_t)(frame_index * properties.block_align +
					channel_index * sample_byte_count);
				sample_index = (frame_base + frame_index) * properties.channel_count +
					channel_index;

				if(properties.encoding == WAV_ENCODING_PCM) {
					samples[sample_index] = sampleReader_DecodePcm(&properties,
						buffer + byte_offset);
				}
				else if(properties.encoding == WAV_ENCODING_G711_ALAW) {
					samples[sample_index] = sampleReader_DecodeALaw(buffer[byte_offset]);
				}
				else if(properties.encoding == WAV_ENCODING_G711_MULAW) {
					samples[sample_index] = sampleReader_DecodeMuLaw(buffer[byte_offset]);
				}
				else if(properties.bits_per_sample == 32) {
					samples[sample_index] = sampleReader_DecodeFloat(buffer + byte_offset);
				}
				else {
					samples[sample_index] = sampleReader_DecodeDouble(buffer + byte_offset);
				}
			}
		}
	}

	free(buffer);
	*store_hnd = store;
	return SAMPLE_READER_STATUS_OK;
}


/*=======================================================================
 *
 *	const char *sampleReader_StatusText(sampleReader_Status status)
 *
 *	Description:	Returns a printable description of a reader status
 *
 *	Input:		status, status value to describe
 *
 *	Output:		Address of a constant, null-terminated string
 *
 *	Notes:		The returned string must not be modified or freed.
 *
 *======================================================================*/
const char *
sampleReader_StatusText(sampleReader_Status status)
{
	switch(status) {
	case SAMPLE_READER_STATUS_OK:
		return "Success";

	case SAMPLE_READER_STATUS_INVALID_ARGUMENT:
		return "Invalid argument";

	case SAMPLE_READER_STATUS_NO_MEMORY:
		return "Insufficient memory";

	case SAMPLE_READER_STATUS_WAVE_ERROR:
		return "Unable to read WAVE audio data";

	case SAMPLE_READER_STATUS_STORE_ERROR:
		return "Unable to create temporary sample storage";

	case SAMPLE_READER_STATUS_UNSUPPORTED_FLOAT:
		return "Host floating-point representation is unsupported";

	default:
		return "Unknown sample reader status";
	}
}
