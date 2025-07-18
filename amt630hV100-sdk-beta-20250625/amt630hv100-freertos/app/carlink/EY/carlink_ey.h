#ifndef __CARLINK_EY_H
#define __CARLINK_EY_H


typedef struct msg_header_st
{
	uint8_t sync_word[3];
	uint8_t type;
	uint32_t dummy;
	uint32_t payload_size;
	uint32_t ts;
}msg_header;

int carlink_ey_init(void);
void carlink_ey_uninit(void);

void set_carlink_video_info(int w, int h, int fps);//set h264 video stream info from phone
void set_carlink_display_info(int x, int y, int w, int h);//set carlink show area in lcd
void set_carlink_display_state(int on); // on: 1.display carlink;  0. display native ui

#define EY_CARLINK_NOTIFY_HOOK                    0
typedef enum
{
    CARLINK_EY_READY,
    CARLINK_EY_EXIT
}CARLINK_EY_STATE;

#if EY_CARLINK_NOTIFY_HOOK
void carlink_ey_notify_state_hook(CARLINK_EY_STATE state);
#endif


#endif
