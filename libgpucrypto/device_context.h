#ifndef DEVICE_CONTEXT_H
#define DEVICE_CONTEXT_H

#include "cuda_mem.h"

#include <cuda_runtime.h>
#include <assert.h>

#define MAX_STREAM 16
#define MAX_BLOCKS 8192

#define true 1
#define false 0

/**
 * Enum for stream state
 **/
enum state {
	READY,
	WAIT_KERNEL,
	WAIT_COPY,
};

typedef struct stream_context_s {
	cudaStream_t stream;

	enum state state;
	uint8_t finished;

	uint8_t *checkbits;
	uint8_t *checkbits_d;
	unsigned int num_blks;

	uint64_t begin_usec;
	uint64_t end_usec;
} stream_context_t;

typedef struct device_context_s {
	stream_context_t stream_ctx[MAX_STREAM + 1]; //stream_ctx 0 is for default stream
	unsigned int nstream;
} device_context_t;

/**
 * Initialize device context. This function will allocate device memory and streams.
 *
 * @param size Total amount of memory per stream
 * @param nstream Number of streams to use. Maximum 16 and minimum is 0.
 *                Use 0 if you want use default stream only.
 *                Be aware that using a single stream and no stream is different.
 *                For more information on how they differ, refer to CUDA manual.
 * @return true when succesful, false otherwise
 **/
uint8_t device_context_init(device_context_t *dc, const unsigned nstream);

/**
 * Check whether current operation has finished.
 *
 * @param stream_id Index of stream. If initilized to 0 stream then 0,
 *                  otherwise from 1 to number of streams initialized.
 * @param block Wait for current operation to finish. true by default
 * @return true if stream is idle, and false if data copy or execution is in progress
 **/
uint8_t device_context_sync(device_context_t *dc, const unsigned stream_id, const uint8_t block);

/**
 * Set the state of the stream. States indicates what is the current operation on-going.
 *
 * @param stream_id Index of stream. If initilized to 0 stream then 0, otherwise from 1 to number of streams initialized.
 * @param state Describe the current state of stream. Currently there are three different states: READY, WAIT_KERNEL, WAIT_COPY.
 **/
void device_context_set_state(device_context_t *dc, const unsigned stream_id, const enum state state);

/**
 * retrieve the state of the stream
 *
 * @param stream_id Index of stream. If initilized to 0 stream then 0, otherwise from 1 to number of streams initialized.
 * @return Current state of the stream
 **/
enum state device_context_get_state(device_context_t *dc, const unsigned stream_id);

/**
 * Retreive the buffer storing the kernel execution finish check bits.
 *
 * This buffer is used in sync function to check whether the kernel has finished or not.
 * Executed kernel will set this each byte to 1 at the end of execution.
 * We use checkbits instead of cudaStreamSynchronize because
 * Calling cudaStreamSynchronize to a stream will block other streams
 * from launching new kernels and it won't return before
 * all previos launched kernel start execution according to CUDA manual 3.2.
 * We use host mapped memory to avoid calling cudaStreamSyncrhonize and
 * check whether kernel execution has finished or not so that
 * multiple stream can run simultaneously without blocking others.
 *
 * @param stream_id Index of stream. If initilized to 0 stream then 0, otherwise from 1 to number of streams initialized.
 * @return pointer to a buffer
 **/
uint8_t *device_context_get_dev_checkbits(device_context_t *dc, const unsigned stream_id);

/**
 * Reset checkbits to 0 for new kernel execution.
 *
 * @param stream_id Index of stream. If initilized to 0 stream then 0, otherwise from 1 to number of streams initialized.
 * @param num_blks Length in bytes to be reset to 0. It should be same as the number of cuda blocks for kernel execution.
 **/
void device_context_clear_checkbits(device_context_t *dc, const unsigned stream_id, const unsigned num_blks);


/**
 * Retrieves cudaStream from stream index.
 *
 * @param stream_id Index of stream. If initilized to 0 stream then 0, otherwise from 1 to number of streams initialized.
 * @return cudaStream
 **/
cudaStream_t device_context_get_stream(device_context_t *dc, const unsigned stream_id);

/**
 * Query whether the device context is initialized to use stream or not.
 *
 * @return
 */
uint8_t device_context_use_stream(device_context_t *dc);


/**
 * Retrieves the time took for the processing in the stream
 * It will only return correct value when all the processing in the stream is finished
 * and state of the stream is back to READY again.
 *
 * @param stream_id index of stream 1 to 16 for streams, and 0 for default stream
 *
 * @return
 */
uint64_t device_context_get_elapsed_time(device_context_t *dc, const unsigned stream_id);


#endif /* DEVICE_CONTEXT_H */
