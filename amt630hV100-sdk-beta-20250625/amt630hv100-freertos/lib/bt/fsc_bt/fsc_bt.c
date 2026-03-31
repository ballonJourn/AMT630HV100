
#include "config.h"
#include "console.h"
#include "board.h"

static bt_sw_cfg_t g_bt_cfg;
static TaskHandle_t fsc_bt_task_handle = NULL;

void fsc_bt_register_pcm_interface(void *itfc)
{
	memcpy((void*)&g_bt_cfg, itfc, sizeof(bt_sw_cfg_t));
}

static void fsc_bt_callback(char * cAtStr)
{
	char* cmd = NULL;
    //printf("fsc_bt_callback %s\r\n", cAtStr);
	if (0) {
	} else if (0 == strncmp(cAtStr, "+VER", 4)) {
		cmd = "AT+ADDR\r\n";
		console_send_atcmd(cmd, strlen(cmd));//get mac addr
	} else if (0 == strncmp(cAtStr, "+ADDR=", 6)) {
		char cmd_str[64] = {0};
		sprintf(cmd_str, "AT+NAME=EY_%s\r\n", (cAtStr + 6));
		console_send_atcmd(cmd_str, strlen(cmd_str));//get mac addr
	}
}
static int fsc_bt_play_state_callback(BT_PLAY_STATE_E state, unsigned short samplerate, unsigned char channel)
{
    //printf("fsc_bt_play_state_callback state %d samplerate %d channel %d\r\n", state, samplerate, channel);
    return 0;
}

static int fsc_bt_a2dp_pcm_data_callback(unsigned char* buffer, unsigned short length)
{
    /* À¶ÑÀÒôÀÖ²¥·ÅÊý¾Ý ²ÉÑùÂÊÎª44100»ò48000£¬2 channel 16bit£¬Ö±½ÓÊä³öµ½I2SÉè±¸ */
    //printf("a2dp length %d\r\n", length);
    return 0;
}

static int fsc_bt_hfp_spk_pcm_data_callback(unsigned char* buffer, unsigned short length)
{
    /* À¶ÑÀµç»°ÏÂÐÐÊý¾Ý ²ÉÑùÂÊÎª8000»ò16000£¬1 channel 16bit£¬Ö±½ÓÊä³öµ½I2SÉè±¸ */
    //printf("spk length %d\r\n", length);
    return 0;
}

#if 0
static int fsc_bt_hfp_mic_pcm_data_callback(unsigned char* buffer, unsigned short length)
{
    /* À¶ÑÀµç»°ÉÏÐÐÊý¾Ý ²ÉÑùÂÊÎª8000»ò16000£¬1 channel 16bit£¬°´ÕÕ²ÎÊýlength»ñÈ¡micÊý¾ÝÌî³äbuffer */
    //printf("mic length %d\r\n", length);
    return 0;
}
#endif

static void bt_task(void* arg)
{
	// bluetooth stack require none volatile storage to store configuration (e.g. device name)  and paired record link key and etc.
	// the file name is "bw_conf0.db" and "bw_conf1.db"
   // do not setup uart in  itpInit , as blueware will do this job.
   bt_hw_cfg_t bt_hw_cfg = {BT_RESET_IO, BT_UART_PORT};
   bt_sw_cfg_t bt_sw_cfg;
   // for AT-Command sent from upper layer application to blueware
   tx_queue = xQueueCreate(AT_CMD_TX_QUEUE_LEN, (unsigned portBASE_TYPE) sizeof(char)*AT_CMD_PAYLOAD_LEN);
   // for AT-Response sent from blueware to upper layer application
   rx_queue = xQueueCreate(AT_CMD_RX_QUEUE_LEN, (unsigned portBASE_TYPE) sizeof(char)*AT_CMD_PAYLOAD_LEN);

   if(!tx_queue || !rx_queue)
   {
	   goto die;
   }

   console_init(fsc_bt_callback);

   bt_hw_cfg.bt_en_pin = BT_RESET_IO;
   bt_hw_cfg.uartport = BT_UART_PORT;

   bt_sw_cfg.rx_queue = rx_queue;
   bt_sw_cfg.tx_queue = tx_queue;
   bt_sw_cfg.a2dp_resampler = 0;
   bt_sw_cfg.debug_mode = 0; // debug toggle
#if CARLINK_EC
   bt_sw_cfg.ble_connection_type = BLE_EASY_CONNECTION;
#else
   bt_sw_cfg.ble_connection_type = BLE_ERYA_CONNECTION;
#endif
   bt_sw_cfg.ancs_enable = 0;
   bt_sw_cfg.absvol_enable = 0;

   if (g_bt_cfg.play_state_cb)
   	bt_sw_cfg.play_state_cb = g_bt_cfg.play_state_cb;
   else
   	bt_sw_cfg.play_state_cb = fsc_bt_play_state_callback;
   
   if (g_bt_cfg.a2dp_cb)
   	bt_sw_cfg.a2dp_cb = g_bt_cfg.a2dp_cb;
   else
   	bt_sw_cfg.a2dp_cb = fsc_bt_a2dp_pcm_data_callback;
   
   if (g_bt_cfg.hfp_spk_cb)
   	bt_sw_cfg.hfp_spk_cb = g_bt_cfg.hfp_spk_cb;
   else
   	bt_sw_cfg.hfp_spk_cb = fsc_bt_hfp_spk_pcm_data_callback;
#if 0
   if (g_bt_cfg.hfp_mic_cb)
   	bt_sw_cfg.hfp_mic_cb = g_bt_cfg.hfp_mic_cb;
   else
   	bt_sw_cfg.hfp_mic_cb = fsc_bt_hfp_mic_pcm_data_callback;
#endif
   fscbt_init(&bt_hw_cfg, &bt_sw_cfg);

   while(!initialize_timeout)
   {
	   // do nothing else here
	   fscbt_run();
   }

die:
   console_deinit();
}

int fsc_bt_main(void)
{
	if (fsc_bt_task_handle)
		return 0;
	/* Create a task to process uart rx data */
	if (xTaskCreate(bt_task, "bt_main", 4096 * 2, NULL,
		configMAX_PRIORITIES / 3, &fsc_bt_task_handle) != pdPASS) {
		printf("create fsc_bt_thread task fail.\n");
		return -1;
	}

	return 0;
}

