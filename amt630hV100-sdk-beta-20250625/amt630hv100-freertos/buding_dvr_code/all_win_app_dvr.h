

#ifndef __APP_DVR_H__
#define __APP_DVR_H__

#include "app_dvr_i.h"
#include "dvr_ui.h"
#include "dvrtop_record.h"
#include "dvrtop_replay.h"
#include "dvrtop_setting.h"
#include "dvrtop_filelist.h"

#define DVRTOP_RECORD_FRM	              			"dvrtop_record_frame"
#define DVRTOP_REPLAY_FRM	              			"dvrtop_replay_frame"
#define DVRTOP_SETTING_FRM	              			"dvrtop_setting_win"
#define DVRTOP_FILELIST_FRM	              			"dvrtop_filelist_win"

#define DVRTOP_RECORD_FRM_ID					(APP_DVR_ID+1)
#define DVRTOP_REPLAY_ID						(APP_DVR_ID+2)
#define DVRTOP_SETTING_ID						(APP_DVR_ID+3)
#define DVRTOP_FILELIST_ID						(APP_DVR_ID+4)
#define DVR_TIPS_ID								(APP_DVR_ID+5)
#define DLG_SCENE_FRM_ID						(APP_DVR_ID+6)


#define LCD_W_SIZE								SCREEN_WIDTH
#define LCD_H_SIZE								SCREEN_HEIGHT

#define DVR_LAYER_W								LCD_W_SIZE
#define DVR_LAYER_H								LCD_H_SIZE


#define BD_CTRL_GET_ID			0x00	/*获取DVR版本号*/
#define BD_CTRL_REC_START		0x01	/*录像开始-!回放模式回到录像模式也用此指令*/		
#define BD_CTRL_REC_STOP		0x02	/*停止录像*/
#define BD_CTRL_SNAP			0x03	/*拍照*/	
#define BD_CTRL_SOS				0x04	/*紧急录像*/
#define BD_CTRL_GET_LIST		0x05	/*获取文件列表,进入回放模式也发此指令-!最好先停止录像!*/
#define BD_CTRL_PB_START		0x06	/*播放对应index的文件,index根据固件回复的list来获取*/
#define BD_CTRL_PB_PAUSE		0x07	/*播放暂停*/
#define BD_CTRL_PB_STOP 		0x08	/*播放停止*/
#define BD_CTRL_GET_STS			0x09	/*获取记录仪的相关状态,例如是否录像,MIC状态以及卡状态*/
#define BD_CTRL_REC_TICK		0x0A	/*记录仪当前的录像秒数 - 暂不使用*/
#define BD_CTRL_SET_RES			0x0B	/*记录仪的录像分辨率 - 0:1080P,1:720P - 预留一般是默认不可选*/
#define BD_CTRL_SET_REC_TIME	0x0C	/*循环录像时长 1:1min,2:2min,3:3min,发送 0:(代表获取当前固件的对应设置)*/				
#define BD_CTRL_MIC_ON			0x0D	/*MIC开关,0:off,1:on*/
#define BD_CTRL_STAMP_ON	 	0x0E	/*时间水印开关,0:off,1:on - 预留,默认开*/
#define BD_CTRL_KEY_ON			0x0F	/*预留*/
#define BD_CTRL_OFF_TIME	 	0x10	/*预留*/
#define BD_CTRL_FORMAT			0x11	/*格式化卡,需要先停止录像*/			
#define BD_CTRL_DEFAULT 		0x12	/*恢复出厂设置,需先停止录像*/
#define BD_CTRL_SET_TIME 		0x13	/*设置时间*/
#define BD_CTRL_DEL_FILE		0x14	/*删除指定index的文件,一般文件列表界面操作*/
#define BD_CTRL_LOCK_FILE		0x15	/*加锁指定index的文件，一般文件列表界面操作*/
#define BD_CTRL_UNLOCK_FILE		0x16	/*解锁指定index的文件，一般文件列表界面操作*/
#define BD_CTRL_TOTAL_TIME		0x17	/*返回指定index录像文件的总时长,一般回放文件时使用*/
#define BD_CTRL_WRITE_PAR		0x18	/*预留*/
#define BD_CTRL_POWER_OFF		0x19	/*预留*/ 
#define BD_CTRL_GSENSOR			0x1a	/*G-SENSOR灵敏度 0x00=off,0x01=低,0x02=中,0x03=高 - 选配项一般不支持*/
#define BD_CTRL_PARK_DO			0x1b	/*预留*/ 
#define BD_CTRL_GET_VER			0x1d	/*预留*/ 
#define BD_CTRL_PB_FF			0x1e 	/*回放视频快进 - 快进无声音*/
#define BD_CTRL_PB_FB			0x1f 	/*回放视频快退 - 快退无声音*/
#define BD_CTRL_GPS_DATA		0x20	/*同步GPS信息到固件*/
#define BD_CTRL_PCM_DATA		0x21	/*GPS水印开关&回放时用来读取音频数据*/ 
#define BD_CTRL_SENSOR_SEL		0x22	/*多路切换预览显示 0:前路,1:后路*/


#define ALIGN(x,a) 				(((x)+(a)-1)&~(a-1)) 
#define BD_MAX_DATA_LEN			(60*1024)
#define BD_FILE_JPG_BIT			0x8000
#define BD_FILE_BACK_BIT		0x4000 
#define BD_HEADER_LEN			10
#define BD_JPG_OFFSET			(16+1)
#define BD_READ_ONCE_SIZE      	(100*1024)

#define O_ACCMODE       00000003
#define O_RDONLY        00000000
#define O_WRONLY        00000001
#define O_RDWR          00000002
#define O_CREAT         00000100  /* not fcntl */
#define O_EXCL          00000200  /* not fcntl */
#define O_NOCTTY        00000400  /* not fcntl */
#define O_TRUNC         00001000  /* not fcntl */
#define O_APPEND        00002000
#define O_NONBLOCK      00004000
#define O_SYNC          00010000
#define FASYNC          00020000  /* fcntl, for BSD compatibility */
#define O_DIRECT        00040000  /* direct disk access hint */
#define O_LARGEFILE     00100000
#define O_DIRECTORY     00200000  /* must be a directory */
#define O_NOFOLLOW      00400000  /* don't follow links */
#define O_NOATIME       01000000
#define O_ATOMICLOOKUP  02000000  /* do atomic file lookup */

#define FIXED_BUF_SIZE			600		//600*1024
#define ALTER_SEEK_ADDR			300*1024
#define JPG_FILE_NAME			"\\elene"
#define SEND_DATA_IN_THREAD

#define EXIT_DVR_TO_HOME_MSG				0
#define EXIT_RECORD_TO_FILELIST_MSG		1
#define EXIT_RECORD_TO_SETTING_MSG		2
#define EXIT_SETTING_TO_RECORD_MSG		3
#define EXIT_FILELIST_TO_RECORD_MSG		4
#define EXIT_FILELIST_TO_REPLAY_MSG		5
#define EXIT_REPLAY_TO_FILELIST_MSG		6
#define RECORD_CARD_FORMAT_MSG			7

typedef enum tag_dvr_exist_e
{
	DVR_ONLINE = 0,
	DVR_OFFLINE,
} dvr_exist_e;

typedef struct tag_st_bd_ctrl_if
{ 
	__u16 header_id;
	__u16 cmd_id;
	__u16 cmd_par;
	__u16 data_len;
	__u16 checksum;
	__u8 trans_buf[BD_MAX_DATA_LEN];	
	__u16 need_send_data;
}st_bd_ctrl_if_t;

typedef struct tag_dvr_attr
{
	H_WIN				dvrtop_record_frmwin;
	H_WIN				dvrtop_replay_frmwin;
	H_WIN				dvrtop_filelist_frmwin;
	H_WIN				dvrtop_setting_frmwin;
	H_WIN 				lyr_setting;
	H_WIN 				h_dialoag_win ;		

	GUI_FONT 			*pfont;
	dvr_ui_t 	ui;
	
	H_WIN *thiswin;
	__u32 vcoder_mod;
	__mp *vcoder;	
	H_LYR disp_layer;
	SIZE disp_size;
	ES_FILE *p_disp;

	ES_FILE *h_cap;
	__ES_FSTAT fstat;
	__u8 *cap_blk_buf;
	__u8 *cap_jpg_buf;
	st_bd_ctrl_if_t cap_ctrl;

	__krnl_event_t   *dvr_sem;
	__u8   dvr_tid;	
	_jpeg_frame_info jpeg_info;
	__u32 yc_size;	
#ifdef ALTER_SEEK_ADDR
	__u32 read_times;
	__u8 round_flag;
#endif
	 __s32 decoder_flag;
}dvr_attr_t, *pdvr_attr_t;

extern void bd_get_file_list(__u8 mode);
extern void bd_playback_file(__u8 mode);
extern void bd_start_takepic(void);
extern void bd_start_rec(void);
extern void bd_stop_rec(void);
extern void bd_lock_file(void);
extern void bd_set_park(__u8 onoff);
extern void bd_set_mic(__u8 onoff);
extern void bd_set_loop_time(__u8 time);
extern void bd_set_collision(__u8 status);
extern void bd_set_parking_onoff(__u8 onoff);
extern void bd_card_format(void);
extern void bd_get_ver(void);

extern __u8 dvr_get_rec_status( void );
extern __u8 dvr_get_park_status( void );
extern __u8 dvr_get_mic_status( void );
extern __u8 dvr_get_loop_time( void );
extern __u8 dvr_get_collision_status( void );
extern __u8 dvr_get_cur_lock_status( void );
extern __u8 dvr_get_sd_status( void );

extern __s32  dvr_cmd2parent(H_WIN hwin, __u16 id, __u32 data2, __u32 reserved);
extern H_WIN app_dvr_create(root_para_t *para);
extern void _dvr_show_title(void);


#endif//__APP_DVR_H__
