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
 * sample_reader.h
 *
 * Interface for decoding WAVE samples into a temporary double store.
 *
 ***************************************************************************/

#ifndef SAMPLE_READER_H
#define SAMPLE_READER_H

#include "sample_store.h"
#include "wav_file.h"


/***************************************************************************
 *
 * Public types
 *
 ***************************************************************************/

typedef enum {
	SAMPLE_READER_STATUS_OK = 0,
	SAMPLE_READER_STATUS_INVALID_ARGUMENT,
	SAMPLE_READER_STATUS_NO_MEMORY,
	SAMPLE_READER_STATUS_WAVE_ERROR,
	SAMPLE_READER_STATUS_STORE_ERROR,
	SAMPLE_READER_STATUS_UNSUPPORTED_FLOAT
} sampleReader_Status;


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
extern sampleReader_Status
sampleReader_Load(wav_Handle wav_hnd, sampleStore_Handle *store_hnd);


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
extern const char *
sampleReader_StatusText(sampleReader_Status status);


#endif
