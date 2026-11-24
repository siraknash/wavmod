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
 * sample_store.h
 *
 * Temporary file-backed storage for decoded double samples.
 *
 ***************************************************************************/

#ifndef SAMPLE_STORE_H
#define SAMPLE_STORE_H

#include <stdint.h>


/***************************************************************************
 *
 * Public types
 *
 ***************************************************************************/

typedef struct sampleStore_Control	*sampleStore_Handle;

typedef enum {
	SAMPLE_STORE_STATUS_OK = 0,
	SAMPLE_STORE_STATUS_INVALID_ARGUMENT,
	SAMPLE_STORE_STATUS_NO_MEMORY,
	SAMPLE_STORE_STATUS_SIZE_OVERFLOW,
	SAMPLE_STORE_STATUS_CREATE_FAILED,
	SAMPLE_STORE_STATUS_SET_LENGTH_FAILED,
	SAMPLE_STORE_STATUS_MAP_FAILED,
	SAMPLE_STORE_STATUS_INVALID_FRAME,
	SAMPLE_STORE_STATUS_INVALID_CHANNEL
} sampleStore_Status;


/*=======================================================================
 *
 *	sampleStore_Status sampleStore_Create(uint64_t frame_count, uint16_t channel_count,
 *									 sampleStore_Handle *hnd)
 *
 *	Description:	Creates temporary storage for decoded samples
 *
 *	Input:		frame_count, number of audio frames
 *			channel_count, number of samples in each frame
 *
 *	Output:		hnd, handle used for subsequent operations, or NULL
 *			if the operation fails
 *			A sampleStore_Status value, SAMPLE_STORE_STATUS_OK for success
 *
 *	Notes:		Samples are stored in frame-major channel order.
 *
 *======================================================================*/
extern sampleStore_Status
sampleStore_Create(uint64_t frame_count, uint16_t channel_count, sampleStore_Handle *hnd);


/*=======================================================================
 *
 *	sampleStore_Status sampleStore_GetData(sampleStore_Handle hnd, double **data,
 *									 uint64_t *sample_count)
 *
 *	Description:	Returns direct access to the mapped sample array
 *
 *	Input:		hnd, handle returned by sampleStore_Create()
 *
 *	Output:		data, address of first double sample, or NULL when empty
 *			sample_count, total number of individual samples
 *			A sampleStore_Status value, SAMPLE_STORE_STATUS_OK for success
 *
 *	Notes:		The address remains valid until sampleStore_Close().
 *
 *======================================================================*/
extern sampleStore_Status
sampleStore_GetData(sampleStore_Handle hnd, double **data, uint64_t *sample_count);


/*=======================================================================
 *
 *	sampleStore_Status sampleStore_GetLayout(sampleStore_Handle hnd, uint64_t *frame_count,
 *									   uint16_t *channel_count)
 *
 *	Description:	Returns the frame and channel dimensions
 *
 *	Input:		hnd, handle returned by sampleStore_Create()
 *
 *	Output:		frame_count, number of frames
 *			channel_count, number of channels
 *			A sampleStore_Status value, SAMPLE_STORE_STATUS_OK for success
 *
 *	Notes:		None
 *
 *======================================================================*/
extern sampleStore_Status
sampleStore_GetLayout(sampleStore_Handle hnd, uint64_t *frame_count, uint16_t *channel_count);


/*=======================================================================
 *
 *	sampleStore_Status sampleStore_GetSample(sampleStore_Handle hnd, uint64_t frame_index,
 *									   uint16_t channel_index, double *value)
 *
 *	Description:	Returns one decoded sample value
 *
 *	Input:		hnd, sample store handle; frame_index, zero-based frame
 *			channel_index, zero-based channel
 *
 *	Output:		value, decoded sample value
 *			A sampleStore_Status value, SAMPLE_STORE_STATUS_OK for success
 *
 *	Notes:		None
 *
 *======================================================================*/
extern sampleStore_Status
sampleStore_GetSample(sampleStore_Handle hnd, uint64_t frame_index, uint16_t channel_index,
					  double *value);


/*=======================================================================
 *
 *	sampleStore_Status sampleStore_SetSample(sampleStore_Handle hnd, uint64_t frame_index,
 *									   uint16_t channel_index, double value)
 *
 *	Description:	Changes one decoded sample value
 *
 *	Input:		hnd, sample store handle; frame_index, zero-based frame
 *			channel_index, zero-based channel; value, new sample value
 *
 *	Output:		A sampleStore_Status value, SAMPLE_STORE_STATUS_OK for success
 *
 *	Notes:		The value is not scaled or clamped.
 *
 *======================================================================*/
extern sampleStore_Status
sampleStore_SetSample(sampleStore_Handle hnd, uint64_t frame_index, uint16_t channel_index,
					  double value);


/*=======================================================================
 *
 *	const char *sampleStore_StatusText(sampleStore_Status status)
 *
 *	Description:	Returns a printable description of a sample store status
 *
 *	Input:		status, status value to describe
 *
 *	Output:		Address of a constant, null-terminated string
 *
 *	Notes:		The returned string must not be modified or freed.
 *
 *======================================================================*/
extern const char *
sampleStore_StatusText(sampleStore_Status status);


/*=======================================================================
 *
 *	void sampleStore_Close(sampleStore_Handle hnd)
 *
 *	Description:	Releases a temporary sample store
 *
 *	Input:		hnd, sample store handle to release
 *
 *	Output:		None
 *
 *	Notes:		A NULL handle is ignored.
 *
 *======================================================================*/
extern void
sampleStore_Close(sampleStore_Handle hnd);


#endif
