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
 * main.c
 *
 * Command-line WAVE inspection and sample loading.
 *
 ***************************************************************************/

#include "sample_reader.h"
#include "sample_store.h"
#include "wav_file.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


/*=======================================================================
 *
 *	const char *main_EncodingText(wav_Encoding encoding)
 *
 *	Description:	Returns a printable description of a WAVE encoding
 *
 *	Input:		encoding, encoding value to describe
 *
 *	Output:		Address of a constant, null-terminated string
 *
 *	Notes:		The returned string must not be modified or freed.
 *
 *======================================================================*/
static const char *
main_EncodingText(wav_Encoding encoding)
{
	switch(encoding) {
	case WAV_ENCODING_PCM:
		return "PCM integer";

	case WAV_ENCODING_IEEE_FLOAT:
		return "IEEE floating-point";

	case WAV_ENCODING_G711_ALAW:
		return "G.711 A-law";

	case WAV_ENCODING_G711_MULAW:
		return "G.711 mu-law";

	case WAV_ENCODING_UNKNOWN:
		break;
	}

	return "Unknown";
}


/*=======================================================================
 *
 *	void main_PrintProperties(const char *filename, const wav_Properties *properties)
 *
 *	Description:	Prints the properties of an open WAVE file
 *
 *	Input:		filename, path supplied on the command line
 *			properties, WAVE properties to print
 *
 *	Output:		None
 *
 *	Notes:		Duration is calculated from frame count and sample rate.
 *
 *======================================================================*/
static void
main_PrintProperties(const char *filename, const wav_Properties *properties)
{
	double duration;

	duration = (double)properties->frame_count / (double)properties->sample_rate;
	printf("File:                  %s\n", filename);
	printf("Encoding:              %s\n", main_EncodingText(properties->encoding));
	printf("Format tag:            0x%04" PRIx16 "\n", properties->format_tag);
	printf("Channels:              %" PRIu16 "\n", properties->channel_count);
	printf("Sample rate:           %" PRIu32 " Hz\n", properties->sample_rate);
	printf("Byte rate:             %" PRIu32 " bytes/second\n", properties->byte_rate);
	printf("Block alignment:       %" PRIu16 " bytes/frame\n", properties->block_align);
	printf("Container size:        %" PRIu16 " bits/sample\n", properties->bits_per_sample);
	printf("Valid sample size:     %" PRIu16 " bits/sample\n", properties->valid_bits_per_sample);
	printf("Channel mask:          0x%08" PRIx32 "\n", properties->channel_mask);
	printf("Frame count:           %" PRIu64 "\n", properties->frame_count);
	printf("Audio data size:       %" PRIu64 " bytes\n", properties->data_byte_count);
	printf("Duration:              %.6f seconds\n", duration);
}


/*=======================================================================
 *
 *	int main(int argc, char **argv)
 *
 *	Description:	Opens, decodes, and displays a WAVE file
 *
 *	Input:		argc, number of command-line arguments
 *			argv, input filename
 *
 *	Output:		EXIT_SUCCESS for success
 *			EXIT_FAILURE for an argument or WAVE file error
 *
 *	Notes:		Exactly one WAVE filename is required.
 *
 *======================================================================*/
int
main(int argc, char **argv)
{
	sampleReader_Status	reader_status;
	sampleStore_Handle	store_hnd;
	wav_Handle			wav_hnd;
	wav_Properties		properties;
	wav_Status			wav_status;

	if(argc != 2) {
		fprintf(stderr, "Usage: %s input.wav\n", argv[0]);
		return EXIT_FAILURE;
	}

	wav_hnd = NULL;
	store_hnd = NULL;
	wav_status = wav_Open(argv[1], &wav_hnd);

	if(wav_status != WAV_STATUS_OK) {
		fprintf(stderr, "%s: %s\n", argv[1], wav_StatusText(wav_status));
		return EXIT_FAILURE;
	}

	wav_status = wav_GetProperties(wav_hnd, &properties);

	if(wav_status != WAV_STATUS_OK) {
		fprintf(stderr, "%s: %s\n", argv[1], wav_StatusText(wav_status));
		wav_Close(wav_hnd);
		return EXIT_FAILURE;
	}

	reader_status = sampleReader_Load(wav_hnd, &store_hnd);

	if(reader_status != SAMPLE_READER_STATUS_OK) {
		fprintf(stderr, "%s: %s\n", argv[1], sampleReader_StatusText(reader_status));
		wav_Close(wav_hnd);
		return EXIT_FAILURE;
	}

	main_PrintProperties(argv[1], &properties);

	sampleStore_Close(store_hnd);
	wav_Close(wav_hnd);
	return EXIT_SUCCESS;
}
