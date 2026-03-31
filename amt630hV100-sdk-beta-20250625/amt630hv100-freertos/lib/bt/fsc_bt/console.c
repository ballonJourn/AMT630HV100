
#include "config.h"
#include "console.h"

QueueHandle_t tx_queue = NULL;
QueueHandle_t rx_queue = NULL;

int initialize_timeout = 0;
bt_at_callback gCallback = NULL;
static void* gCbContex = NULL;

/*
	example to demonstrate that:
	1. recv AT response/event from blueware and print out
	the application should parse these data according to "Feasycom BT90X AT Command interface_v4.1.pdf"
	2. send AT command to blueware for call/music/phonebook control
	note:
	the applicaton should process data as soon as possilbe (otherwise the queue may overflow)
*/
static void* console_task(void* arg)
{
	char buf[AT_CMD_PAYLOAD_LEN];
        
        gCbContex = gCbContex;//make iar happy
    for (;;)
	{
		buf[0] = '\0';
		char *response = buf;
		if (xQueueReceive(rx_queue, response, portMAX_DELAY) == pdTRUE)
		{
			response[AT_CMD_PAYLOAD_LEN-1] = '\0';

			int  response_len = strlen(response);
			// response always start/end with \r\n
			if((response_len < 4) ||
				(response[0] != '\r' || response[1] != '\n') ||
				(response[response_len-2] != '\r' || response[response_len-1] != '\n'))
			{
				printf("invalid response format !!! \n");
				continue;
			}

			response[response_len-2] = '\0';
			response += 2;

            if (NULL != gCallback)
			{
			    gCallback(response);
            }
			if(strlen(response) > 4 && !memcmp(response,"+VER",4))
			{
			    console_send_atcmd("AT+NAME\r\n", strlen("AT+NAME\r\n"));
			}
		}
	}
    return NULL;
}



int console_init(bt_at_callback cb)
{
	pthread_t task;
	pthread_attr_t attr;
	if (NULL == gCallback)
		gCallback = cb;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 4096 * 3);
	pthread_create(&task, &attr, console_task, NULL);
	return 0;
}

void console_register_cb(void *ctx, bt_at_callback cb)
{
	gCbContex = ctx;
	gCallback = cb;
}

int console_send_atcmd(char* cAtCmd, unsigned short length)
{
    if (tx_queue == NULL)
    {
	printf("console_send_atcmd tx_queue NULL\n");
	return -1;
    }

	char buf[AT_CMD_PAYLOAD_LEN] = {0};
    char *command;

    if (AT_CMD_PAYLOAD_LEN <= length)
    {
        printf("console_send_atcmd invalid length. %d\r\n", length);
        return -1;
    }
    command = buf;
    memcpy(command,cAtCmd, length);
    if (pdTRUE != xQueueSend(tx_queue,command,pdMS_TO_TICKS(200)))
    {
        printf("console_send_atcmd Send failed.\r\n");
        return -1;
    }
    fscbt_wakeup(0);
    return 0;
}

int console_deinit(void)
{
	if(rx_queue) vQueueDelete(rx_queue);
	if(tx_queue) vQueueDelete(tx_queue);
    return 0;
}


