
#ifndef __FSCBT_INTERFACE_H__
#define __FSCBT_INTERFACE_H__

#include <stdint.h>

#define AT_CMD_TX_QUEUE_LEN 20
#define AT_CMD_RX_QUEUE_LEN 100
#define AT_CMD_PAYLOAD_LEN 4096

typedef enum
{
    BT_A2DP_PLAY_START = 0,
    BT_A2DP_PLAY_STOP  = 1,
    BT_HFP_PLAY_START = 2,
    BT_HFP_PLAY_STOP = 3,
    BT_PLAY_STATE_END
} BT_PLAY_STATE_E;

typedef enum
{
    BLE_EASY_CONNECTION = 0,   /* for EASYCONNECTION */
    BLE_ERYA_CONNECTION = 1,   /* for ERYA CONNECTION */
    BLE_DEFAULT_CONNECTION = 2,   /* for DEFAULT CONNECTION */
    BLE_DEFAULT1_CONNECTION = 3,   /* for DEFAULT1 CONNECTION */
    BLE_CONNECTION_END
} BLE_CONNECTION_TYPE_E;

typedef enum
{
    BT_UART_PORT_1 = 1,   /*  Uart 1  */
    BT_UART_PORT_3 = 3,   /*  Uart 3  */
    BT_UART_PORT_END
} BT_UART_PORT_E;


typedef int (* bt_play_state_callback)(BT_PLAY_STATE_E state, unsigned short samplerate, unsigned char channel);
typedef int (* bt_pcm_data_callback)(unsigned char* buffer, unsigned short length);

typedef struct
{
    unsigned int bt_en_pin;
    unsigned int uartport;
}bt_hw_cfg_t;

typedef struct
{
    void* tx_queue;                          /* AT-Command sent from upper layer application to blueware */
    void* rx_queue;                          /* AT-Response sent from blueware to upper layer application */
    int   a2dp_resampler;                    /* not support.default 0 */
    int   debug_mode;                        /* turn blueware debug log on/off */
    bt_play_state_callback play_state_cb;    /* play state callback function */
    bt_pcm_data_callback a2dp_cb;            /* a2dp pcm data callback function */
    bt_pcm_data_callback hfp_spk_cb;         /* hfp spk pcm data callback function */
    unsigned char   ble_connection_type;               /* BLE_CONNECTION_TYPE_E */
    unsigned char   ancs_enable;               /* ancs on/off */
    unsigned char   absvol_enable;               /* abs vol on/off */
}bt_sw_cfg_t;

/*!
    \brief:  initialise bluetooth stack (blueware)
    \param hw_cfg:  hardware configuration
     \param sw_cfg:  software configuration
*/
int fscbt_init(const bt_hw_cfg_t *hw_cfg,const bt_sw_cfg_t *sw_cfg);

/*!
    \brief:  blueware main loop , running in a standalone task
*/
void fscbt_run(void);

/*!
    \brief:  wake up blueooth task from sleeping
    \param from_isr:  set to 1 if call from isr, othewise set to 0
*/
void fscbt_wakeup(int from_isr);

/*!
    \brief:  push mic data to bt send buffer.

    \param *buffer: data to Send
            length: data length. 160 bytes default.
     \return: sent data length
*/
int fscbt_push_mic_data(unsigned char* buffer, unsigned short length);

/*!
    \brief:  get avrcp coverart image data..

    \param * pu32len: Image Data Length.
            0:No Data.
     \return: 
     JPG Image Data Address:
      start data: 0xff, 0xd8
      end data:0xff,0xd9
*/
void fscbt_get_coverart_data(char** pImage,unsigned int* pu32len);

#endif

