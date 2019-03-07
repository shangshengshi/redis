#ifndef NIO_RW_H
#define NIO_RW_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

int32_t start_nio_read_thread();

int32_t start_nio_write_thread();

#endif