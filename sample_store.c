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
 * sample_store.c
 *
 * Temporary file-backed storage for decoded double samples.
 *
 ***************************************************************************/

#include "sample_store.h"

#include <limits.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>


/***************************************************************************
 *
 * Private types
 *
 ***************************************************************************/

typedef struct sampleStore_Control sampleStore_Control;

struct sampleStore_Control {
	double		*samples;
	size_t		map_length;
	uint64_t	frame_count;
	uint64_t	sample_count;
	uint16_t	channel_count;
};


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
sampleStore_Status
sampleStore_Create(uint64_t frame_count, uint16_t channel_count, sampleStore_Handle *hnd)
{
	char				filename[] = "/tmp/wavmod-samples-XXXXXX";
	sampleStore_Control	*ctrl;
	uint64_t			map_length;
	uint64_t			sample_count;
	void				*map;
	int					fd;

	if(hnd != NULL) {
		*hnd = NULL;
	}

	if(hnd == NULL || channel_count == 0) {
		return SAMPLE_STORE_STATUS_INVALID_ARGUMENT;
	}

	if(frame_count != 0 && channel_count > UINT64_MAX / frame_count) {
		return SAMPLE_STORE_STATUS_SIZE_OVERFLOW;
	}

	sample_count = frame_count * channel_count;

	if(sample_count > SIZE_MAX / sizeof(double)) {
		return SAMPLE_STORE_STATUS_SIZE_OVERFLOW;
	}

	map_length = sample_count * sizeof(double);

	if(map_length > (uint64_t)INT64_MAX) {
		return SAMPLE_STORE_STATUS_SIZE_OVERFLOW;
	}

	ctrl = calloc(1, sizeof(*ctrl));

	if(ctrl == NULL) {
		return SAMPLE_STORE_STATUS_NO_MEMORY;
	}

	ctrl->frame_count = frame_count;
	ctrl->sample_count = sample_count;
	ctrl->channel_count = channel_count;
	ctrl->map_length = (size_t)map_length;

	if(map_length == 0) {
		*hnd = ctrl;
		return SAMPLE_STORE_STATUS_OK;
	}

	fd = mkstemp(filename);

	if(fd < 0) {
		free(ctrl);
		return SAMPLE_STORE_STATUS_CREATE_FAILED;
	}

	unlink(filename);

	if(ftruncate(fd, (off_t)map_length) != 0) {
		close(fd);
		free(ctrl);
		return SAMPLE_STORE_STATUS_SET_LENGTH_FAILED;
	}

	map = mmap(NULL, (size_t)map_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	if(map == MAP_FAILED) {
		free(ctrl);
		return SAMPLE_STORE_STATUS_MAP_FAILED;
	}

	ctrl->samples = map;
	*hnd = ctrl;
	return SAMPLE_STORE_STATUS_OK;
}


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
sampleStore_Status
sampleStore_GetData(sampleStore_Handle hnd, double **data, uint64_t *sample_count)
{
	if(hnd == NULL || data == NULL || sample_count == NULL) {
		return SAMPLE_STORE_STATUS_INVALID_ARGUMENT;
	}

	*data = hnd->samples;
	*sample_count = hnd->sample_count;
	return SAMPLE_STORE_STATUS_OK;
}


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
sampleStore_Status
sampleStore_GetLayout(sampleStore_Handle hnd, uint64_t *frame_count, uint16_t *channel_count)
{
	if(hnd == NULL || frame_count == NULL || channel_count == NULL) {
		return SAMPLE_STORE_STATUS_INVALID_ARGUMENT;
	}

	*frame_count = hnd->frame_count;
	*channel_count = hnd->channel_count;
	return SAMPLE_STORE_STATUS_OK;
}


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
sampleStore_Status
sampleStore_GetSample(sampleStore_Handle hnd, uint64_t frame_index, uint16_t channel_index,
					  double *value)
{
	if(hnd == NULL || value == NULL) {
		return SAMPLE_STORE_STATUS_INVALID_ARGUMENT;
	}

	if(frame_index >= hnd->frame_count) {
		return SAMPLE_STORE_STATUS_INVALID_FRAME;
	}

	if(channel_index >= hnd->channel_count) {
		return SAMPLE_STORE_STATUS_INVALID_CHANNEL;
	}

	*value = hnd->samples[frame_index * hnd->channel_count + channel_index];
	return SAMPLE_STORE_STATUS_OK;
}


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
sampleStore_Status
sampleStore_SetSample(sampleStore_Handle hnd, uint64_t frame_index, uint16_t channel_index,
					  double value)
{
	if(hnd == NULL) {
		return SAMPLE_STORE_STATUS_INVALID_ARGUMENT;
	}

	if(frame_index >= hnd->frame_count) {
		return SAMPLE_STORE_STATUS_INVALID_FRAME;
	}

	if(channel_index >= hnd->channel_count) {
		return SAMPLE_STORE_STATUS_INVALID_CHANNEL;
	}

	hnd->samples[frame_index * hnd->channel_count + channel_index] = value;
	return SAMPLE_STORE_STATUS_OK;
}


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
const char *
sampleStore_StatusText(sampleStore_Status status)
{
	switch(status) {
	case SAMPLE_STORE_STATUS_OK:
		return "Success";

	case SAMPLE_STORE_STATUS_INVALID_ARGUMENT:
		return "Invalid argument";

	case SAMPLE_STORE_STATUS_NO_MEMORY:
		return "Insufficient memory";

	case SAMPLE_STORE_STATUS_SIZE_OVERFLOW:
		return "Sample store is too large";

	case SAMPLE_STORE_STATUS_CREATE_FAILED:
		return "Unable to create temporary sample file";

	case SAMPLE_STORE_STATUS_SET_LENGTH_FAILED:
		return "Unable to size temporary sample file";

	case SAMPLE_STORE_STATUS_MAP_FAILED:
		return "Unable to map temporary sample file";

	case SAMPLE_STORE_STATUS_INVALID_FRAME:
		return "Invalid frame index";

	case SAMPLE_STORE_STATUS_INVALID_CHANNEL:
		return "Invalid channel index";

	default:
		return "Unknown sample store status";
	}
}


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
void
sampleStore_Close(sampleStore_Handle hnd)
{
	if(hnd == NULL) {
		return;
	}

	if(hnd->samples != NULL) {
		munmap(hnd->samples, hnd->map_length);
	}

	free(hnd);
}
