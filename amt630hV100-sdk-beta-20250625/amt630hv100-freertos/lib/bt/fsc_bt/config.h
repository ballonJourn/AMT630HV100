
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "fscbt_interface.h"

#define TEST_STACK_SIZE 500000

extern QueueHandle_t tx_queue;
extern QueueHandle_t rx_queue;
extern int initialize_timeout;

#endif

