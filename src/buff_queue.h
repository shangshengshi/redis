
#ifndef __BUFFER_QUEUE_H__
#define __BUFFER_QUEUE_H__

#include <stdio.h>
#include <stdint.h>
#include "sds.h"

typedef struct buffq_header
{
    uint64_t buff_size;
    uint64_t rsv_size; // must be 8
    uint64_t begin;
    uint64_t end;
    uint32_t count; // msg #
} buffq_header;

typedef struct buff_queue
{
	buffq_header header;
	char* data;
}buff_queue;

buff_queue buffq_create(uint64_t buff_size);

int32_t buffq_enqueue1(buff_queue queue,
					   const sds sbuf);

int32_t buffq_dequeue1(buff_queue queue,
					   sds* sbuf);

int32_t buffq_enqueue2(buff_queue queue,
					   const sds sbuf1,
					   const sds sbuf2);

int32_t buffq_dequeue2(buff_queue queue,
					   sds* sbuf1,
			           sds* sbuf2);

int32_t buffq_is_full(buff_queue queue, uint64_t len);

int32_t buffq_is_empty(buff_queue queue);

#endif
