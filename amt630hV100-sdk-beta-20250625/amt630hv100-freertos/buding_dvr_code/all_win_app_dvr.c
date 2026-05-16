/*
*******************************************************************************************************************
*File Name   :  app_dvr.c
*Author      :  Jason.Mei
*Version     :  1.1.0
*Date        :  2017-11-09
*Description :  USB/SD DVR function
*******************************************************************************************************************
*/

#include "app_dvr.h"
#include "dvrtop_record.h"
#include "dvrtop_replay.h"
#include "dvrtop_setting.h"
#include "dvrtop_filelist.h"

#define __msg(...)

st_bd_ctrl_if_t ctrl_data;
static __u8 g_rec_status=RECSTATUS_OFF;
static __u8 g_park_status=PARK_OFF;
static __u8 g_mic_status=MIC_OFF;
static __u8 g_loop_time=RECORD_TIME_2;
static __u8 g_collision_status=COLLISION_OFF;
static __u8 g_lock_status=LOCK_OFF;
static __u8 g_sd_status=SD_FALSE;
__u8 ver_buf[128];	
extern __u32 filelist_mode;
__u32 videofile_idx_min=0;
__u32 photofile_idx_min=0;

void  __app_dvr_get_search_path(char* search_path)
{
	__s32 ret = 0;
	char root_type[50];
	char disk_name[RAT_MAX_PARTITION][4];

	eLIBs_memset(root_type, 0, sizeof(root_type));
	eLIBs_memset(disk_name, 0, sizeof(disk_name));
	eLIBs_memset(search_path, 0, sizeof(search_path));

	eLIBs_strcpy(root_type,RAT_USB_DISK);				
	ret = rat_get_partition_name(root_type, disk_name);
	__msg("search_path=%s\n", search_path);
	eLIBs_strcpy(search_path,disk_name[0]);	
	return ;
}

void bd_send_data(__u8 * buf,st_bd_ctrl_if_t *ctrl, ES_FILE *hFile)
{
#if 1  /*open file again for write if support,for cahe write to msdc*/        
	if(hFile)
	{
		int i;

		__msg("write data dump\n");
		
		for(i=0;i<6;i++){
//			__msg("0x%02x ",buf[i]);
			__msg("0x%02x\n",((__u8 *)&ctrl_data)[i]);
		}
		
//		eLIBs_fwrite(buf, 1, 512, hFile);
		eLIBs_fwrite(((const void *)&ctrl_data), 1, 512, hFile);
		eLIBs_fsync(hFile);
		//eLIBs_fclose(hFile);
	}
	else
	{
		__msg("JPG_FILE_NAME open failed!\n");
	}
#else 	
	lseek( fd, 0, SEEK_SET );	
	write(fd,buf,512);
#endif
}

//发送指令给DVR
void bd_send_normal_cmd(unsigned short cmd_id,unsigned short cmd_par)
{	
	st_bd_ctrl_if_t *pUvcCtr = &ctrl_data;	
	
	pUvcCtr->header_id = 0xaa55;	
	pUvcCtr->cmd_id = cmd_id;	
	pUvcCtr->cmd_par = cmd_par;	
	pUvcCtr->data_len = 0;	
	pUvcCtr->need_send_data = 1;
}

//数据的校验和,可不发
static __u16 bd_data_checksum(__u8 *buf,__u32 len)
{
	__u16 i,checksum=0;

	for(i=0;i<len;i++){
		checksum += *(buf+i);
	}

	return checksum;
}

//同步时间到记录仪,DVR本身不会保存时间,开机首次需要同步时间给DVR
void bd_set_time_cmd(void)
{
	__u16 *ptr16;
	st_bd_ctrl_if_t *pUvcCtr = &ctrl_data;
	__time_t time;
	__date_t date;

	ptr16 = (__u16 *)pUvcCtr->trans_buf;
	
	esTIME_GetTime(&time);
	esTIME_GetDate(&date);

	eLIBs_printf("sys time= %04d_%02d_%02d  %02d:%02d:%02d\n",date.year,date.month,date.day,time.hour,time.minute,time.second);

	date.year-=2000;

	*(ptr16+0)=(__u16)((date.month<<8)|date.year);
	*(ptr16+1)=(__u16)((time.hour<<8)|date.day);
	*(ptr16+2)=(__u16)((time.second<<8)|time.minute);

	pUvcCtr->header_id = 0xaa55;
	pUvcCtr->cmd_id = 0x13;
	pUvcCtr->cmd_par = 0;
	pUvcCtr->data_len = 6;

	pUvcCtr->need_send_data = 1;

}

//同步GPS信息到DVR,这个频繁发送会导致预览画面卡顿,建议不要此功能
void bd_set_gps_cmd(void)
{
	#define GPS_STR	  "E1140416N226492"//"E123.42.45 N45.12.34 5"

	long i,len;
	unsigned char *ptr8;
	ST_WCM_CTRL_IF *pUvcCtr = &ctrl_data;

	ptr8 = (unsigned char *)&ctrl_data.trans_buf;
	strcpy((char*)ptr8,GPS_STR);
	
	printf("gps:%s,len:%d\n",ptr8,sizeof(GPS_STR));

	pUvcCtr->header_id = 0xaa55;
	pUvcCtr->cmd_id = 0x20;
	pUvcCtr->cmd_par = 0;
	pUvcCtr->data_len = sizeof(GPS_STR);

	pUvcCtr->need_send_data = 1;

}

//解析获取到的文件列表
void bd_filelist_parser(__u8 *buf,__s32 len)
{
	#define BYTE_PER_FILE	6
	
	st_bd_ctrl_if_t *pUvcCtr = &ctrl_data;
	__s32 cnt,name_idx;
	__u8 *ptr8=buf;
	__u32  year,mon,day,hour,min,sec,attrib,hash;
	__u16 checksum;
	char file_name[24];
	char	str[128];
	char	time[128];

	__msg("bd_filelist_parser\n");

	//计算checksum,可不用
	checksum = bd_data_checksum(buf,len);
	if(pUvcCtr->checksum !=checksum ){
		eLIBs_printf("checksum error,%x,%x\n",pUvcCtr->checksum,checksum);
	}

	for(cnt=0;cnt<(len/BYTE_PER_FILE);cnt++){

		//文件的序号
		name_idx = (*(ptr8+1)<<8)|(*ptr8);
		
		if(name_idx == 0xffff){
			eLIBs_printf("list end\n");
			break;
		}
		
		//文件修改时间
		hash = (*(ptr8+5)<<24)|(*(ptr8+4)<<16)|(*(ptr8+3)<<8)|(*(ptr8+2)<<0);
		eLIBs_printf("hash:0x%x\n",hash);

		year = (hash>>26)&0x3f;
		mon = (hash>>22)&0x0f;
		day = (hash>>17)&0x1f;
		hour = (hash>>12)&0x1f;
		min = (hash>>6)&0x3f;
		sec = (hash>>0)&0x3f;
			
		year +=2000;
		attrib = sec&0x01;

		ptr8 +=BYTE_PER_FILE;

		//首先区分是图片文件还是录像文件
		if(pUvcCtr->cmd_par&0x8000)
		{															 /*jpg*/
			eLIBs_sprintf(file_name,"PICT%04d.jpg",name_idx);
			photofile_idx_min = name_idx;
			__my("-------photofile_idx_min=%d-------------   \n ",photofile_idx_min);
		}
		else
		{
			//是否是加锁文件
			if(attrib)
			{
				eLIBs_sprintf(file_name,"LOCK%04d.avi",name_idx);
			}
			else
			{
				eLIBs_sprintf(file_name,"MOVI%04d.avi",name_idx);
			}
			videofile_idx_min = name_idx;
		}
		eLIBs_printf("file=%s\n",file_name);
		
		eLIBs_printf("%04d_%02d_%02d  %02d:%02d:%02d\n",year,mon,day,hour,min,sec);

   		//以下是UI显示用
		eLIBs_memset(str, 0, sizeof(str));
		eLIBs_memset(time, 0, sizeof(time));

		eLIBs_sprintf(time," %04d-%02d-%02d  %02d:%02d:%02d",year,mon,day,hour,min,sec);
		eLIBs_strcat(str, time);

		if(filelist_mode==DVR_VIDEO)
		{
			_dvr_video_filelist_add_one(file_name,str);
		}
		else
		{
			_dvr_photo_filelist_add_one(file_name,str);
		}
		//eLIBs_printf("l=%d\n",attrib); /*attrib==1,is lock file*/
		
	}

	eLIBs_printf("file cnt:%d\n",cnt);

}

static __s32 __dvr_capture_init(pdvr_attr_t ctrl)
{
	char path[128];	
	
	__app_dvr_get_search_path(path);
	eLIBs_strcat(path, JPG_FILE_NAME);

	__msg("DVR dummy path '%s' \r\n", path);
	ctrl->h_cap = (ES_FILE *)eLIBs_fopen(path, "rb+");
	//!!注意如果此处能支持"O_DIRECT"旗标,那就用旗标打开文件,可以提升读取速度
	//ctrl->h_cap = (ES_FILE *)eLIBs_open(path,O_RDONLY|O_NOCTTY|O_DIRECT, 0666);
	if(ctrl->h_cap == NULL){
		__wrn("Capture open fail\r\n");
		return EPDK_FAIL;
	}
	
#ifdef ALTER_SEEK_ADDR
	ctrl->read_times = 0;
	ctrl->round_flag = 0;
#endif	
	eLIBs_fstat(ctrl->h_cap, &ctrl->fstat);

	__msg("dummy file blksize=%d, blocks=%d ....\r\n",(__s32)ctrl->fstat.blksize, (__s32)ctrl->fstat.blocks);
	
#ifdef FIXED_BUF_SIZE
	ctrl->cap_blk_buf = esMEMS_Calloc(0, (__s32)ctrl->fstat.blksize, (__s32)FIXED_BUF_SIZE);
//	ctrl->cap_blk_buf = esMEMS_Balloc((__s32)ctrl->fstat.blksize*(__s32)FIXED_BUF_SIZE);
//	ctrl->cap_blk_buf = esMEMS_Palloc((__u32)FIXED_BUF_SIZE/2, 0);
#else
	ctrl->cap_blk_buf = esMEMS_Calloc(0, (__s32)ctrl->fstat.blksize, (__s32)ctrl->fstat.blocks);
#endif
	if(ctrl->cap_blk_buf == NULL)
	{
		__wrn("esMEMS_Calloc error blksize=%d, blocks=%d\r\n", (__s32)ctrl->fstat.blksize, (__s32)ctrl->fstat.blocks);
		return EPDK_FAIL;
	}
	
	ctrl->cap_jpg_buf = (__u8 *)esMEMS_Malloc(0, 300*1024);
	if(ctrl->cap_jpg_buf == NULL){
		__wrn("esMEMS_Malloc error\r\n");
		return EPDK_FAIL;
	}
	return EPDK_OK;
}


static __s32 __dvr_capture_fini(pdvr_attr_t ctrl)
{
	if(ctrl == NULL){
		return EPDK_FAIL;
	}
	
	if(ctrl->h_cap){
		eLIBs_fclose(ctrl->h_cap);
		ctrl->h_cap = NULL;
	}
	
	if(ctrl->cap_blk_buf){
#ifdef FIXED_BUF_SIZE
		//esMEMS_Bfree(ctrl->cap_blk_buf, (__s32)ctrl->fstat.blksize * (__s32)FIXED_BUF_SIZE);
		esMEMS_Mfree(0, ctrl->cap_blk_buf);
//		esMEMS_Pfree(ctrl->cap_blk_buf, (__u32)FIXED_BUF_SIZE/2);
#else
		esMEMS_Bfree(ctrl->cap_blk_buf, (__s32)ctrl->fstat.blksize * (__s32)ctrl->fstat.blocks);
#endif
		ctrl->cap_blk_buf = NULL;
	}
	if(ctrl->cap_jpg_buf){
		esMEMS_Mfree(0, ctrl->cap_jpg_buf);
		ctrl->cap_jpg_buf = NULL;
	}
	return EPDK_OK;
}

//接受指令用
void __dvr_recv_cmd_process(st_bd_ctrl_if_t *pctrl)
{
	int len;

	//__msg("pctrl->cmd_id=0x%x\n", pctrl->cmd_id);
	
	switch(pctrl->cmd_id)
	{
		case BD_CTRL_GET_ID: //ID data
		{
			len = pctrl->data_len;
			__msg("id:%s\n",pctrl->trans_buf);
			break;
		}
		case BD_CTRL_GET_LIST: //filelist
		{
			eLIBs_memcpy(&ctrl_data,pctrl,10);  /*for bd_filelist_parser() need ctrl_data*/
			len = pctrl->data_len;			
			bd_filelist_parser(pctrl->trans_buf,len);
			//__my("id:%s\n",pctrl->trans_buf);
			break;
		}
		case BD_CTRL_GET_STS: //dvr status
		{
			if(pctrl->cmd_par&0x01)
			{
				__msg("have sd\n");
				g_sd_status=SD_TRUE;
			} 
			else 
			{
				__msg("no sd\n");
				g_sd_status=SD_FALSE;
			}
			
			if(pctrl->cmd_par&0x02)
			{
				__msg("in recording\n");
				g_rec_status=RECSTATUS_ON;
			} 
			else 
			{
				__msg("not record\n");	
				g_rec_status=RECSTATUS_OFF;
			}	
			
			if(pctrl->cmd_par&0x04)
			{
				__msg("file is lock\n");
				g_lock_status=LOCK_ON;
			} 
			else 
			{
				__msg("file not lock\n");
				g_lock_status=LOCK_OFF;
			}

			if(pctrl->cmd_par&0x08)
			{
				__msg("card error\n");
				g_sd_status=SD_ERROR;
			}

			if(pctrl->cmd_par&0x10)
			{
				__msg("card full\n");
				g_sd_status=SD_FULL;
			}

			if(pctrl->cmd_par&0x20)
			{
				__msg("mic on\n");
				g_mic_status=MIC_ON;
			}
			else
			{
				__msg("mic off\n");
				g_mic_status=MIC_OFF;
			}

			//代表有多路
			if(pctrl->cmd_par&0x40)
			{
				__msg("dual rec hw\n");
			} 
			else 
			{
				__msg("single rec hw\n");
			}
		
			break;
		}
		case BD_CTRL_PCM_DATA:
		{		
			//回放时，用作解析音频数据,根据系统情况解析播放
			break;
		}
		default:
			break;
	}
}


void bd_seek_for_align(pdvr_attr_t ctrl)
{
	__s32 offset,limit;
	
	if(ctrl->round_flag==0){
		limit = ctrl->fstat.size;
	}
	else{
		limit = ctrl->fstat.size/3;
	}
	offset = ctrl->read_times*300*1024;
	
	if(offset+300*1024>= limit)
	{
		ctrl->round_flag = 1;

		ctrl->read_times = 0;		
		offset = 100*300*1024;
		eLIBs_fseek(ctrl->h_cap,offset,SEEK_SET);
	}
	else
	{
		//__my("seek=%d\n",offset>>10);
		eLIBs_fseek(ctrl->h_cap,offset,SEEK_SET);
		ctrl->read_times++;
	}
}

//读取数据,包括指令和JPG
static __s32 __dvr_capture_get_pic_process(pdvr_attr_t ctrl)
{
	__size nb;
	st_bd_ctrl_if_t *pctrl;
	__size size;
#ifdef ALTER_SEEK_ADDR
	__u32 cnt,check_sum,read_check_sum,i; 
#endif
	
	if(ctrl == NULL)
	{
		return EPDK_FAIL;
	}
	if(ctrl->h_cap == NULL)
	{
		return EPDK_FAIL;
	}
	//eLIBs_fsync( ctrl->h_cap);
	//__msg("dummy file blksize=%d, blocks=%d ....\r\n",(__s32)ctrl->fstat.blksize, (__s32)ctrl->fstat.blocks);
	
#ifdef ALTER_SEEK_ADDR
	bd_seek_for_align(ctrl);
#else
	//!!!如果支持旗标"o_direct" 则可以seek到一个固定位置一直读数据
	eLIBs_fseek(ctrl->h_cap,0,SEEK_SET);
#endif
	nb = eLIBs_fread(ctrl->cap_blk_buf, 1, ctrl->fstat.blksize, ctrl->h_cap);
	//__msg("------------>read1\n");
	//__msg("nb=%d\n", nb);
	//__my("ctrl->h_cap=%d\n", ctrl->h_cap);
	//__my("ctrl->fstat.blksize=%d\n", ctrl->fstat.blksize);
	if (nb != ctrl->fstat.blksize)
	{
		__msg("read err\n");
		return EPDK_FAIL;
	}

	pctrl = (st_bd_ctrl_if_t*)ctrl->cap_blk_buf;

	//如果头是0xAA55,代表是指令,则进入对应的指令解析
	if(ctrl->cap_blk_buf[1]==0xAA && ctrl->cap_blk_buf[0]==0x55) 
	{
		nb = ctrl->fstat.blksize - BD_HEADER_LEN;			

		if(pctrl->data_len >nb &&  pctrl->data_len<BD_MAX_DATA_LEN-ctrl->fstat.blksize)
		{
			/*read data to cap_blk_buf will update pctrl,so need bk data len*/			
			__size data_len = pctrl->data_len;
			
			__my(" ctrl->fstat.blksize=%d\n",  ctrl->fstat.blksize);
			
			eLIBs_memcpy(&ctrl->cap_ctrl, ctrl->cap_blk_buf, ctrl->fstat.blksize);

#if 0
			eLIBs_fread(ctrl->cap_blk_buf, 1, ALIGN(data_len-nb, ctrl->fstat.blksize), ctrl->h_cap);
			eLIBs_memcpy(((char *)&ctrl->cap_ctrl) + ctrl->fstat.blksize, ctrl->cap_blk_buf, data_len-nb);	
#else			
			size = 100*1024+  (esKRNL_Time()&0x1f)*1024;
			eLIBs_fread(ctrl->cap_blk_buf, 1, size, ctrl->h_cap);/*read more data for fs not cache*/		
			eLIBs_memcpy(((char *)&ctrl->cap_ctrl) + ctrl->fstat.blksize, ctrl->cap_blk_buf, 512*2*20);	
#endif

			__my(" pctrl->data_len-nb=%d\n",  data_len-nb);

			/*pointer to new data*/
			pctrl = (st_bd_ctrl_if_t*)&ctrl->cap_ctrl;
	
			//eLIBs_memcpy(pctrl, &ctrl->cap_ctrl, sizeof(st_bd_ctrl_if_t));
		}

		__dvr_recv_cmd_process(pctrl);
		return EPDK_FAIL;
	} 
	//如果不是0xAA55,代表是一张图片
	else 
	{
		size = *(__s32 *)ctrl->cap_blk_buf;
		read_check_sum = *(__u32 *)(ctrl->cap_blk_buf+8);
		
		__msg("jpg size:0x%x\n", size);
		
		if(size < 1024)
		{
			__msg("jpeg size error\n");
			size = 100*1024+  (esKRNL_Time()&0x1f)*1024;
			eLIBs_fread(ctrl->cap_blk_buf+512, 1, size, ctrl->h_cap); /*read dummy data for fs not cache*/			
			return EPDK_FAIL;
		}
	
		nb = ctrl->fstat.blksize - BD_JPG_OFFSET;
		eLIBs_memcpy(ctrl->cap_jpg_buf, ctrl->cap_blk_buf + BD_JPG_OFFSET, nb);
		//__msg("header:%x,%x\n",ctrl->cap_jpg_buf[0],ctrl->cap_jpg_buf[1]);

		if((ctrl->cap_jpg_buf[0] != 0xff)||(ctrl->cap_jpg_buf[1] != 0xd8)){
			size = 100*1024+  (esKRNL_Time()&0x1f)*1024;
			eLIBs_fread(ctrl->cap_blk_buf+512, 1, size, ctrl->h_cap); /*read dummy data for fs not cache*/
			return EPDK_FAIL;
		}
		

		if(size < (300 * 1024))
		{
			eLIBs_fread(ctrl->cap_blk_buf+512, 1, size, ctrl->h_cap);
			//__msg("cap_blk_buf tail:%x, %x\n", ctrl->cap_blk_buf[size-2], ctrl->cap_blk_buf[size-1]);

			eLIBs_memcpy(ctrl->cap_jpg_buf + nb, ctrl->cap_blk_buf+512, size);
			//__msg("TAIL:%x,%x\n",ctrl->cap_jpg_buf[size+nb-BD_JPG_OFFSET-2],ctrl->cap_jpg_buf[size+nb-BD_JPG_OFFSET-1]);

			cnt = (size-511)>>9;
			check_sum = 0;
			for(i=1;i<cnt;i++){
				check_sum += *(__u32*)(ctrl->cap_blk_buf+i*512);
			}

			//可不校验checksum
			if((check_sum&0xff) != (read_check_sum&0xff)){
				__my("check sum err,size:%d,0x%08x,0x%08x\n",size,check_sum,read_check_sum);
				return EPDK_FAIL;
			}

			//校验FF D9,确保收到是完整JPG
			if((ctrl->cap_jpg_buf[size-BD_JPG_OFFSET-2] != 0xff)||(ctrl->cap_jpg_buf[size-BD_JPG_OFFSET-1] != 0xd9))
			{
				__my("tail error\n");
				__msg("TAIL:%x,%x\n",ctrl->cap_jpg_buf[size-2],ctrl->cap_jpg_buf[size-1]);
				return EPDK_FAIL;
			}

			__my("ok size:0x%x\n", size);

			//拷贝数据
			ctrl->jpeg_info.jpegData = (void *)(ctrl->cap_jpg_buf);
			ctrl->jpeg_info.jpegData_len = size + nb;		
		} 
		else 
		{
			__msg("Over size=%d then 300x1024\r\n", size);
		}
	}
	return EPDK_OK;
}

__s32 dvr_cmd2parent(H_WIN hwin, __u16 id, __u32 data2, __u32 reserved)
{
	__gui_msg_t msg;

	msg.h_deswin 	= GUI_WinGetParent(hwin);
	msg.h_srcwin 	= NULL;
	msg.id 			= GUI_MSG_COMMAND;
	msg.dwAddData1 	= id;
	msg.dwAddData2 	= data2;
	msg.dwReserved 	= reserved;

	return GUI_SendNotifyMessage(&msg);
}

static __s32 dvr_cmd2root(H_WIN hwin, __s32 id, __s32 data2, __s32 reserved)
{
	H_WIN hparent;
	__gui_msg_t msg;

	hparent = GUI_WinGetParent(hwin);
	if (!hparent)
	{
		__err("hparent is null...\n");
		return EPDK_FAIL;
	}

	msg.h_deswin = hparent;
	msg.h_srcwin = hwin;
	msg.id = GUI_MSG_COMMAND;
	msg.dwAddData1 = MAKELONG(GUI_WinGetItemId(hwin), id);
	msg.dwAddData2 = data2;
	msg.dwReserved = reserved;

	GUI_SendNotifyMessage(&msg);

	return EPDK_OK;
}

void _dvr_show_title(void)
{
    char title[256]={0};
    eLIBs_memset(title, 0, sizeof(title));
    dsk_langres_get_menu_text(STRING_HOME_DVR, title,sizeof(title));
    gscene_hbar_set_title(title, sizeof(title));
}

//DVR 操作线程
static __s32 __dvr_display_thread(void *argv)
{
	__u32 arg[3];
	__disp_video_fb_t tmpPara;
	pdvr_attr_t ctrl = (pdvr_attr_t )argv;

	while(1)
	{
		if(esKRNL_TDelReq(OS_PRIO_SELF) == OS_TASK_DEL_REQ)
		{
			esKRNL_TDel(OS_PRIO_SELF);
		}
		
		#ifdef SEND_DATA_IN_THREAD //线程中发送指令,根据系统实际情况可更换发送的位置
		if(ctrl_data.need_send_data)
		{
			__u8 * buf;
			buf = esMEMS_Balloc(512);
			if(buf)
			{
				eLIBs_memcpy(buf,&ctrl_data,512);
				bd_seek_for_align(ctrl);
				bd_send_data(buf,&ctrl_data, ctrl->h_cap);
				esMEMS_Bfree(buf, 512);
			}
			else
			{
				__msg("esMEMS_Balloc failed\n");
			}
			
			ctrl_data.need_send_data = 0;
		}
		#endif

		//接收到JPG数据送入解码器解码显示
		if(__dvr_capture_get_pic_process(ctrl) == EPDK_OK)
		{
			ctrl->jpeg_info.pic_height = 0;
			ctrl->jpeg_info.pic_width = 0;

			if(ctrl->filelist_flag==0)
			{
				if(esMODS_MIoctrl(ctrl->vcoder, MPEJ_CODEC_CMD_DECODER, 0, &ctrl->jpeg_info) == EPDK_OK)
				{
					tmpPara.id = 0;
					tmpPara.addr[0] = ctrl->jpeg_info.y_buf;
					tmpPara.addr[1] = ctrl->jpeg_info.c_buf;
					tmpPara.addr[2] = 0;

					if(ctrl->decoder_flag==0)
					{
						arg[0] = ctrl->disp_layer;
						arg[1] = 0;
						arg[2] = 0;
						eLIBs_fioctrl(ctrl->p_disp,DISP_CMD_LAYER_OPEN,0,(void*)arg);
						eLIBs_fioctrl(ctrl->p_disp,DISP_CMD_VIDEO_START,0,(void*)arg);
						ctrl->decoder_flag=1;
					}
					arg[0] = ctrl->disp_layer;
					arg[1] = (__u32)&tmpPara;
					arg[2] = 0;
					
					eLIBs_fioctrl(ctrl->p_disp,DISP_CMD_VIDEO_SET_FB,0,(void*)arg);
				} 
				else 
				{
					__wrn(">>MPEJ_CODEC_CMD_DECODER fail!\r\n");
				}
			}
		}
		esKRNL_TimeDly(2);
	}
}


static __s32 __dvr_display_fini(pdvr_attr_t ctrl)
{
	__u8  err;
	__u32 arg[3];	

	if(ctrl->filelist_flag==0)
	{
		if(ctrl->dvr_tid)
		{
			while(1)
			{       
				if(esKRNL_TDelReq(ctrl->dvr_tid) == OS_TASK_NOT_EXIST)
				{
				    break;
				}
				esKRNL_TimeDly(1);
			}
			ctrl->dvr_tid = 0;
		}
	}
	__my("-------------2222------------------\n");

	if(ctrl->dvr_sem){		
		esKRNL_SemDel(ctrl->dvr_sem, OS_DEL_ALWAYS, &err);
		ctrl->dvr_sem = NULL;
	}
	__my("-------------3333------------------\n");

	if(ctrl->jpeg_info.c_buf){
		esMEMS_Pfree((void *)ctrl->jpeg_info.c_buf, ctrl->yc_size);
		ctrl->jpeg_info.c_buf = 0;
	}
	__my("-------------4444------------------\n");
	if(ctrl->jpeg_info.y_buf){
		esMEMS_Pfree((void *)ctrl->jpeg_info.y_buf, ctrl->yc_size);
		ctrl->jpeg_info.y_buf = 0;
	}
	__my("-------------5555------------------\n");

	if(ctrl->p_disp){
		if(ctrl->disp_layer){
			arg[0] = ctrl->disp_layer;
			arg[1] = 0;
			arg[2] = 0;
			eLIBs_fioctrl(ctrl->p_disp, DISP_CMD_VIDEO_STOP, 0, (void*)arg);
			eLIBs_fioctrl(ctrl->p_disp, DISP_CMD_LAYER_CLOSE, 0, (void*)arg);
			eLIBs_fioctrl(ctrl->p_disp, DISP_CMD_LAYER_RELEASE, 0, (void*)arg);
			ctrl->disp_layer = NULL;
		}
		eLIBs_fclose(ctrl->p_disp);
		ctrl->p_disp = NULL;
	}
	__my("-------------6666------------------\n");

	if(ctrl->vcoder){
		esMODS_MClose(ctrl->vcoder);
		ctrl->vcoder = NULL;
	}
	__my("-------------7777------------------\n");
	if(ctrl->vcoder_mod == NULL){
		esMODS_MUninstall(ctrl->vcoder_mod);
		ctrl->vcoder_mod = NULL;
	}

	ctrl->decoder_flag=0;
	return EPDK_OK;
}


static __s32 __dvr_display_init(pdvr_attr_t ctrl)
{
	__s32 ret;
	__u32 arg[3];
	__disp_layer_info_t layer_para;
	
	ctrl->vcoder_mod = esMODS_MInstall("d:\\mod\\cedar\\vcoder.plg", 0);
	if(ctrl->vcoder_mod == NULL){
		__wrn("VCODER mod install fail!\r\n");
		return EPDK_FAIL;
	}
	ctrl->vcoder = esMODS_MOpen(ctrl->vcoder_mod, 0);
	if(ctrl->vcoder == NULL){
		__wrn("VCODER mod open fail!\r\n");
		goto RET_ERR;
	}
	ctrl->p_disp = eLIBs_fopen("b:\\DISP\\DISPLAY", "r+");
	if(ctrl->p_disp == NULL){
		__wrn("DISPLAY fopen error!\r\n");
		goto RET_ERR;
	}
	arg[0] = DISP_LAYER_WORK_MODE_SCALER;
	arg[1] = 0;
	arg[2] = 0;
	ctrl->disp_layer = eLIBs_fioctrl(ctrl->p_disp, DISP_CMD_LAYER_REQUEST, 0, (void *)arg);
	if(ctrl->disp_layer == NULL){
		__wrn("DISPLAY fioctrl DISP_CMD_LAYER_REQUEST!\r\n");
		goto RET_ERR;
	}
	
	layer_para.fb.addr[0] = 0;
	layer_para.fb.size.width    = 1280;//ctrl->disp_size.width;
	layer_para.fb.size.height   = 720; //ctrl->disp_size.height;
	layer_para.fb.mode          = DISP_MOD_MB_UV_COMBINED;
	layer_para.fb.format        = DISP_FORMAT_YUV420;
	layer_para.fb.br_swap       = 0;
	layer_para.fb.seq           = DISP_SEQ_UVUV;
	layer_para.ck_enable        = 0;
	layer_para.alpha_en         = 1;
	layer_para.alpha_val        = 0xff;
	layer_para.pipe             = 0;
	layer_para.src_win.x        = 0;
	layer_para.src_win.y        = 0;
	layer_para.src_win.width    = 1280;//ctrl->disp_size.width;
	layer_para.src_win.height   = 720;//ctrl->disp_size.height;
	if(ctrl->disp_size.height > ctrl->disp_size.width) {
		layer_para.scn_win.width = ctrl->disp_size.width * LCD_H_SIZE / ctrl->disp_size.height;
	} else {
		layer_para.scn_win.width = LCD_W_SIZE;
	}
	layer_para.scn_win.x = (LCD_W_SIZE-layer_para.scn_win.width)/2;
	layer_para.scn_win.y = 0;
	layer_para.scn_win.height = LCD_H_SIZE;
	layer_para.mode = DISP_LAYER_WORK_MODE_SCALER;
	arg[0] = ctrl->disp_layer;
	arg[1] = (__u32)&layer_para;
	arg[2] = 0;
	eLIBs_fioctrl(ctrl->p_disp, DISP_CMD_LAYER_SET_PARA, 0, (void *)arg);
	eLIBs_fioctrl(ctrl->p_disp,DISP_CMD_LAYER_CLOSE,0,(void*)arg);
	arg[0] = ctrl->disp_layer;
	arg[1] = 0;
	arg[2] = 0;
	eLIBs_fioctrl(ctrl->p_disp, DISP_CMD_LAYER_BOTTOM, 0, (void*)arg);
	//arg[0] = ctrl->disp_layer;
	//arg[1] = 0;
	//arg[2] = 0;
	//eLIBs_fioctrl(ctrl->p_disp,DISP_CMD_LAYER_OPEN,0,(void*)arg);
	//eLIBs_fioctrl(ctrl->p_disp,DISP_CMD_VIDEO_START,0,(void*)arg);
	ctrl->yc_size = (ctrl->disp_size.width * ctrl->disp_size.height + 1024) / 1024 + 500;
	ctrl->jpeg_info.c_buf = (__u32)esMEMS_Palloc(ctrl->yc_size,0);
	if(ctrl->jpeg_info.c_buf == 0){
		__wrn("esMEMS_Palloc c_buf [yc_size=%d] fail!\r\n", ctrl->yc_size);
		goto RET_ERR;
	}
	ctrl->jpeg_info.y_buf = (__u32)esMEMS_Palloc(ctrl->yc_size,0);
	if(ctrl->jpeg_info.y_buf == 0){
		__wrn("esMEMS_Palloc y_buf [yc_size=%d] fail!\r\n", ctrl->yc_size);
		goto RET_ERR;
	}	
	ctrl->dvr_sem =  esKRNL_SemCreate(1);
	if(ctrl->dvr_sem == NULL){
		__wrn("dvr_sem create fail\r\n");
		goto RET_ERR;
	}
	if((ctrl->filelist_flag==0)&&(!ctrl->dvr_tid))
	{
		ctrl->dvr_tid =  esKRNL_TCreate(__dvr_display_thread, (void *)ctrl, 0x800, KRNL_priolevel2);
		if(ctrl->dvr_tid == NULL){
			__wrn("dvr_tid create fail\r\n");
			goto RET_ERR;
		}
	}
	return EPDK_OK;
RET_ERR:
	__dvr_display_fini(ctrl);
	return EPDK_FAIL;
}


/***********************************************************************************************************
	����ͼ��
************************************************************************************************************/
static H_LYR dvrtop_setting_layer_create(RECT *rect)
{
	H_LYR layer = NULL;
	FB  fb =
	{
	    {0, 0},                                   		/* size      */
	    {0, 0, 0},                                      /* buffer    */
	    {FB_TYPE_RGB, {PIXEL_COLOR_ARGB8888, 0, (__rgb_seq_t)0}},    /* fmt       */
	};

	__disp_layer_para_t lstlyr =
	{
	    DISP_LAYER_WORK_MODE_NORMAL,                    /* mode      */
	    0,                                              /* ck_mode   */
	    0,                                              /* alpha_en  */
	    0,                                              /* alpha_val */
	    1,                                              /* pipe      */
	    0xff,                                           /* prio      */
	    {0, 0, 0, 0},                           		/* screen    */
	    {0, 0, 0, 0},                               	/* source    */
	    DISP_LAYER_OUTPUT_CHN_DE_CH1,                   /* channel   */
	    NULL                                            /* fb        */
	};

	__layerwincreate_para_t lyrcreate_info =
	{
	    "setting layer",
	    NULL,
	    GUI_LYRWIN_STA_SUSPEND,
	    GUI_LYRWIN_NORMAL
	};
	
	fb.size.width		= rect->width;
	fb.size.height		= rect->height;	
	
	lstlyr.src_win.x  		= 0;
	lstlyr.src_win.y  		= 0;
	lstlyr.src_win.width 	= rect->width;
	lstlyr.src_win.height 	= rect->height;
	
	lstlyr.scn_win.x		= rect->x;
	lstlyr.scn_win.y		= rect->y;
	lstlyr.scn_win.width  	= rect->width;
	lstlyr.scn_win.height 	= rect->height;
	
	lstlyr.pipe = 1;
	lstlyr.fb = &fb;
	lyrcreate_info.lyrpara = &lstlyr;
	
	layer = GUI_LyrWinCreate(&lyrcreate_info);
	if( !layer )
	{
		__err("app bar layer create error !\n");
	} 
		
	return layer;	
}


//设置界面创建,根据实际系统现实
static __s32 __dvr_setting_create(__gui_msg_t *msg)
{
       RECT rect;						
      	dvr_uipara_t* uipara=NULL;
       dvrtop_setting_para_t setting_para;
	pdvr_attr_t pdvr_attr;
	
	pdvr_attr = (pdvr_attr_t)GUI_WinGetAttr(msg->h_deswin);
	
	if( pdvr_attr == NULL)
		return EPDK_OK;
	
       uipara = (dvr_uipara_t*)dvr_get_uipara();
	   
	if( pdvr_attr->dvrtop_record_frmwin )
	{
		dvrtop_record_destory(pdvr_attr->dvrtop_record_frmwin);
		pdvr_attr->dvrtop_record_frmwin = NULL;
	}
		
       if( pdvr_attr->lyr_setting)
	{
		GUI_LyrWinDelete(pdvr_attr->lyr_setting);
		pdvr_attr->lyr_setting = NULL ;
	}

      	rect.x = uipara->dvr_rect.x;
      	rect.y = uipara->dvr_rect.y;
      	rect.width = uipara->dvr_rect.width;
      	rect.height = uipara->dvr_rect.height;

	pdvr_attr->lyr_setting = dvrtop_setting_layer_create(&rect);
	setting_para.layer = pdvr_attr->lyr_setting;
	setting_para.font= pdvr_attr->pfont;
	setting_para.focus_id = 0;
	pdvr_attr->dvrtop_setting_frmwin= dvrtop_setting_win_create(msg->h_deswin, &setting_para);
	GUI_WinSetFocusChild(pdvr_attr->dvrtop_setting_frmwin);
	return EPDK_OK;
}

static __s32 _dvr_destory_all_scene(__gui_msg_t *msg)
{
	pdvr_attr_t pdvr_attr;
	pdvr_attr = (pdvr_attr_t)GUI_WinGetAttr(msg->h_deswin);
	if( pdvr_attr == NULL)
		return EPDK_OK;

	if( pdvr_attr->h_dialoag_win )
	{
		app_dialog_destroy( pdvr_attr->h_dialoag_win );
		pdvr_attr->h_dialoag_win = NULL ;
	}

	if( pdvr_attr->dvrtop_record_frmwin )
	{
		dvrtop_record_destory(pdvr_attr->dvrtop_record_frmwin);
		pdvr_attr->dvrtop_record_frmwin = NULL;
	}
	
	if( pdvr_attr->dvrtop_replay_frmwin)
	{
		dvrtop_replay_destory(pdvr_attr->dvrtop_replay_frmwin);
		pdvr_attr->dvrtop_replay_frmwin = NULL;
	}
	
	if( pdvr_attr->dvrtop_filelist_frmwin)
	{
		dvrtop_filelist_destory(pdvr_attr->dvrtop_filelist_frmwin);
		pdvr_attr->dvrtop_filelist_frmwin = NULL;
	}

       if( pdvr_attr->lyr_setting)
	{
		GUI_LyrWinDelete(pdvr_attr->lyr_setting);
		pdvr_attr->lyr_setting = NULL ;
	}
	__msg("before __dvr_capture_fini\n");
	__dvr_capture_fini(pdvr_attr);	
	__msg("before __dvr_display_fini\n");
	__dvr_display_fini(pdvr_attr);
	uninit_dvr_res( &pdvr_attr->ui );
	pdvr_attr->filelist_flag=0;
	return EPDK_OK;
}

static __s32 _dvr_on_create(__gui_msg_t *msg)
{
	pdvr_attr_t pdvr_attr;
	pdvr_attr = (pdvr_attr_t)GUI_WinGetAttr(msg->h_deswin);
	
	if( pdvr_attr == NULL)
		return EPDK_OK;

	init_dvr_res( &pdvr_attr->ui );

	if(__dvr_capture_init(pdvr_attr) == EPDK_FAIL){
		__wrn("__dvr_capture_init error\r\n");
		return EPDK_FAIL;
	}
	pdvr_attr->disp_size.width = LCD_W_SIZE;
	pdvr_attr->disp_size.height = LCD_H_SIZE;

	pdvr_attr->dvrtop_record_frmwin = dvrtop_record_create(msg->h_deswin);
	_dvr_video_filelist_create();
	_dvr_photo_filelist_create();

	bd_set_time_cmd();
	
	return __dvr_display_init(pdvr_attr);
}

static __s32 _dvr_on_close(__gui_msg_t *msg)
{
	pdvr_attr_t pdvr_attr;
	pdvr_attr = (pdvr_attr_t)GUI_WinGetAttr(msg->h_deswin);
	if( pdvr_attr == NULL)
		return EPDK_OK;

	GUI_ManWinDelete(msg->h_deswin);
       GUI_WinSetFocusChild(msg->h_deswin);
	return EPDK_OK;
}
static __s32 _dvr_on_destory(__gui_msg_t *msg)
{
	pdvr_attr_t pdvr_attr;
	pdvr_attr = (pdvr_attr_t)GUI_WinGetAttr(msg->h_deswin);
	if( pdvr_attr == NULL)
		return EPDK_OK;

	_dvr_video_filelist_end();
	_dvr_photo_filelist_end();
	_dvr_destory_all_scene(msg);
	esMEMS_Bfree(pdvr_attr, sizeof(dvr_attr_t));
	GUI_WinSetAttr(msg->h_deswin, NULL);
	return EPDK_OK;
}

static __s32 _dvr_on_command(__gui_msg_t *msg)
{
	pdvr_attr_t pdvr_attr;
	pdvr_attr = (pdvr_attr_t)GUI_WinGetAttr(msg->h_deswin);
	if( pdvr_attr == NULL)
		return EPDK_OK;

	switch( msg->dwAddData1 )
	{
		case EXIT_DVR_TO_HOME_MSG: //返回到MP5Z主界面
		{
			_dvr_destory_all_scene(msg);
			dvr_cmd2root(msg->h_deswin, SWITCH_TO_MMENU, 0, 0);
			break;
		}
		case EXIT_RECORD_TO_FILELIST_MSG:  //切到DVR列表界面(回放模式)
		{
			if( pdvr_attr->h_dialoag_win )
			{
				app_dialog_destroy( pdvr_attr->h_dialoag_win );
				pdvr_attr->h_dialoag_win = NULL ;
			}
			if( pdvr_attr->dvrtop_record_frmwin )
			{
				dvrtop_record_destory(pdvr_attr->dvrtop_record_frmwin);
				pdvr_attr->dvrtop_record_frmwin = NULL;
			}
			pdvr_attr->filelist_flag=1;
			__dvr_display_fini(pdvr_attr);
			esKRNL_TimeDly(70);
			pdvr_attr->dvrtop_filelist_frmwin= dvrtop_filelist_create(msg->h_deswin);
			break;
		}
		case EXIT_RECORD_TO_SETTING_MSG:	//切到DVR设置界面
		{
			__dvr_setting_create(msg);
			break;
		}
		case EXIT_SETTING_TO_RECORD_MSG:   //从DVR设置界面返回预览
		{
			if( pdvr_attr->lyr_setting)
			{
				GUI_LyrWinDelete(pdvr_attr->lyr_setting);
				pdvr_attr->lyr_setting = NULL ;
			}
			pdvr_attr->dvrtop_record_frmwin = dvrtop_record_create(msg->h_deswin);
			break;
		}
		case EXIT_FILELIST_TO_RECORD_MSG:  //从DVR列表界面返回预览
		{
			if( pdvr_attr->dvrtop_filelist_frmwin)
			{
				dvrtop_filelist_destory(pdvr_attr->dvrtop_filelist_frmwin);
				pdvr_attr->dvrtop_filelist_frmwin = NULL;
			}
			_dvr_video_filelist_end();
			_dvr_photo_filelist_end();
			if(dvr_get_rec_status()!= RECSTATUS_ON)
			{
				bd_start_rec();
			}
			__dvr_display_init(pdvr_attr);
			pdvr_attr->filelist_flag=0;
			pdvr_attr->dvrtop_record_frmwin = dvrtop_record_create(msg->h_deswin);
			break;
		}
		case EXIT_FILELIST_TO_REPLAY_MSG:  //DVR列表界面进入播放文件界面
		{
			if( pdvr_attr->dvrtop_filelist_frmwin)
			{
				dvrtop_filelist_destory(pdvr_attr->dvrtop_filelist_frmwin);
				pdvr_attr->dvrtop_filelist_frmwin = NULL;
			}
			__dvr_display_init(pdvr_attr);
			pdvr_attr->filelist_flag=0;
			pdvr_attr->dvrtop_replay_frmwin = dvrtop_replay_create(msg->h_deswin);
			break;
		}
		case EXIT_REPLAY_TO_FILELIST_MSG:  //DVR播放文件界面进入列表界面
		{
			if( pdvr_attr->dvrtop_replay_frmwin)
			{
				dvrtop_replay_destory(pdvr_attr->dvrtop_replay_frmwin);
				pdvr_attr->dvrtop_replay_frmwin = NULL;
			}
			pdvr_attr->filelist_flag = 1;
			__dvr_display_fini(pdvr_attr);
			pdvr_attr->dvrtop_filelist_frmwin = dvrtop_filelist_create(msg->h_deswin);
			break;
		}
		case RECORD_CARD_FORMAT_MSG:    //格式化操作提示对话框,需点击"确认后执行"
		{
			__s32 lang_id[]={STRING_SET_CUE , STRING_DVR_FORMAT_MEMORY_CARD};

			app_cmd2parent(APP_ROOT,DSK_MSG_AUDIO_DELETE);

			default_dialog(pdvr_attr->h_dialoag_win , msg->h_deswin , DVR_TIPS_ID, ADLG_YESNO, lang_id);
			break;
		}
		case DVR_TIPS_ID:
		{	
			switch(HIWORD(msg->dwAddData1))
			{
				case ADLG_CMD_EXIT:
				{
					app_dialog_destroy( pdvr_attr->h_dialoag_win) ;
					pdvr_attr->h_dialoag_win = NULL ;

					if(ADLG_IDYES == msg->dwAddData2 )
					{
						bd_card_format();
					}
					return EPDK_OK ;
				}
				default:
				{
					break;
				}
			}
			break ;
		}
		default:
			break;
	}

	return EPDK_OK;
}
static __s32 _dvr_manager_win_proc(__gui_msg_t *msg)
{
	switch(msg->id)
	{
		case GUI_MSG_CREATE:
			return _dvr_on_create(msg);
		case GUI_MSG_CLOSE:
			return _dvr_on_close(msg);
		case GUI_MSG_DESTROY:
			return _dvr_on_destory(msg);
		case GUI_MSG_COMMAND:
			return _dvr_on_command(msg);
		case GUI_MSG_KEY:
			break;
		case GUI_MSG_TOUCH:
			break;
		case DSK_MSG_HOME:
		{
			_dvr_destory_all_scene(msg);
			dvr_cmd2root(msg->h_deswin, SWITCH_TO_MMENU, 0, 0);
			return EPDK_OK;
		}
		//DVR掉线或者被拔出,需要回到MP5主界面
		case DSK_MSG_FS_PART_PLUGOUT:
		{
			__u32 root_type = 0;
			__u8 usb_root = 0;	// sd_root=0, 
			
			__msg("DSK_MSG_FS_PART_PLUGOUT\n");
			root_type = root_check_disk();	
			//sd_root = (root_type >> 8)&0x00ff;
			usb_root = (root_type)&0x00ff; 
			if(!usb_root)
			{
				_dvr_destory_all_scene(msg);
				dvr_cmd2root(msg->h_deswin, SWITCH_TO_MMENU, 0, 0);
			}
			return EPDK_OK;								
		}
	}
	return GUI_ManWinDefaultProc(msg);
}

//检查DVR是否插入并且在线
dvr_exist_e app_dvr_check_online(void)
{
	ES_FILE *h_tmp;
	dvr_exist_e exist;
	char path[128];
	
	__app_dvr_get_search_path(path);
	eLIBs_strcat(path, JPG_FILE_NAME);

	__msg("DVR dummy path '%s' \r\n", path);
	h_tmp = (ES_FILE *)eLIBs_fopen(path,"r");
	
	if(h_tmp != NULL) {
		 eLIBs_fclose(h_tmp);
		 h_tmp = NULL;
		exist = DVR_ONLINE;
	} else {
		exist = DVR_OFFLINE;
	}
	return exist;
}

H_WIN app_dvr_create(root_para_t  *para)
{
	__gui_manwincreate_para_t 	create_info;
	pdvr_attr_t	pdvr_attr = NULL;
	__inf("****************************************************************************************\n");
	__inf("********  enter multi screen home application  **************\n");
	__inf("****************************************************************************************\n");

       gscene_bgd_set_state(BGD_STATUS_HIDE);    
       gscene_hbar_set_state(HBAR_ST_HIDE);

	if(app_dvr_check_online() != DVR_ONLINE){
		__wrn("DVR_OFFLINE \r\n");
		return NULL;
	}

	pdvr_attr = (pdvr_attr_t)esMEMS_Balloc(sizeof(dvr_attr_t));
	if( pdvr_attr == NULL )
	{
		__msg("esMEMS_Balloc fail\n");
		return NULL;
	}
	
	eLIBs_memset(pdvr_attr, 0, sizeof(dvr_attr_t));
	eLIBs_memset(&create_info, 0, sizeof(__gui_manwincreate_para_t));

	pdvr_attr->pfont 		= para->font;

	create_info.name            = APP_DVR;
	create_info.hParent         = para->h_parent;
	create_info.ManWindowProc   = (__pGUI_WIN_CB)esKRNL_GetCallBack((__pCBK_t)_dvr_manager_win_proc);
	create_info.attr            = (void*)pdvr_attr;
	create_info.id				= APP_DVR_ID;
	create_info.hHosting        = NULL;
 
      {
		reg_root_para_t* reg_root_para;
		reg_root_para = (reg_root_para_t*)dsk_reg_get_para_by_app(REG_APP_ROOT);
		reg_root_para->last_app_id = APP_DVR_ID;
		reg_root_para->bt_last_app_id = APP_DVR_ID;
		reg_root_para->calendar_last_app_id = APP_DVR_ID;
	}
	return(GUI_ManWinCreate(&create_info));
}

//获取前路视频文件和图片文件
void bd_get_file_list(__u8 mode)
{
	__u16 par = 0;
	
	__my("-------------bd_get_file_list------------------\n");

	if(mode == DVR_VIDEO)
	{
		par |= 0;/*获取  录像文件 list*/
	}
	else
	{
		par |= (0|BD_FILE_JPG_BIT);/*获取 拍照文件 list*/
	}
	
	bd_send_normal_cmd(BD_CTRL_GET_LIST,par);
}

//播放视频文件或者图片文件
void bd_playback_file(__u8 mode,__u32 index)
{
	__u16 par=0,idx;

	idx = index; /*文件序号*/

	par = idx;

	if(mode == DVR_VIDEO)
	{
		par |=0;/*播放录像文件 */
	}
	else
	{
		par |=(0|BD_FILE_JPG_BIT);/*播放图片文件 */
	}
	__my("par=0x%x\n",par);
	bd_send_normal_cmd(BD_CTRL_PB_START,par);
}

//播放视频文件停止
void bd_playback_stop(void)
{
	__my("bd_playback_stop\n");
	bd_send_normal_cmd(BD_CTRL_PB_STOP,0);
}

//拍照
void bd_start_takepic(void)
{	
	__msg("bd_start_takepic\n");
	bd_send_normal_cmd(BD_CTRL_SNAP,0);
}

//开始录像
void bd_start_rec(void)
{	
	__my("bd_start_rec\n");
	bd_send_normal_cmd(BD_CTRL_REC_START,0);
}

//停止录像
void bd_stop_rec(void)
{	
	__my("bd_stop_rec\n");
	bd_send_normal_cmd(BD_CTRL_REC_STOP,0);
}

//紧急录像
void bd_lock_file(void)
{	
	__msg("bd_lock_file\n");
	bd_send_normal_cmd(BD_CTRL_SOS,0);
}

//预留
void bd_set_park(__u8 onoff)
{	
	__msg("bd_set_park\n");
	bd_send_normal_cmd(BD_CTRL_PARK_DO,onoff);
	g_park_status=onoff;
}

//开关录音
void bd_set_mic(__u8 onoff)
{	
 	reg_dvr_para_t* dvr_para;
	dvr_para = (reg_dvr_para_t*)dsk_reg_get_para_by_app(REG_APP_DVR);

	__msg("bd_set_mic\n");

	bd_send_normal_cmd(BD_CTRL_MIC_ON,onoff);
	dvr_para->mic_onoff=onoff;
}

//设置循环录像时长
void bd_set_loop_time(__u8 time)
{	
 	reg_dvr_para_t* dvr_para;
	dvr_para = (reg_dvr_para_t*)dsk_reg_get_para_by_app(REG_APP_DVR);

	__msg("bd_set_loop_time\n");
	bd_send_normal_cmd(BD_CTRL_SET_REC_TIME,time);
	g_loop_time=time;
	dvr_para->loop_time=time;
}

//设置G-SENSOR灵敏度
void bd_set_collision(__u8 status)
{	
 	reg_dvr_para_t* dvr_para;
	dvr_para = (reg_dvr_para_t*)dsk_reg_get_para_by_app(REG_APP_DVR);

	__msg("bd_set_collision\n");
	bd_send_normal_cmd(BD_CTRL_GSENSOR,status);
	g_collision_status=status;
	dvr_para->collision_status=status;
}

//预留
void bd_set_parking_onoff(__u8 onoff)
{	
 	reg_dvr_para_t* dvr_para;
	dvr_para = (reg_dvr_para_t*)dsk_reg_get_para_by_app(REG_APP_DVR);

	__msg("bd_set_parking_onoff\n");

	bd_send_normal_cmd(BD_CTRL_PARK_DO,onoff);
	dvr_para->parking_onoff=onoff;
}

//操作卡格式化,需先操作停止录像
void bd_card_format(void)
{	
	__msg("bd_card_format\n");
	if(dvr_get_rec_status() == RECSTATUS_ON)
	{
		bd_stop_rec();
	}
	esKRNL_TimeDly(30);
	bd_send_normal_cmd(BD_CTRL_FORMAT,0);
}

//获取DVR固件版本号
void bd_get_ver(void)
{	
	__msg("bd_get_ver\n");
	bd_send_normal_cmd(BD_CTRL_GET_ID,0);
}

//以下是MP5自身用到的状态，根据系统实际情况设计
__u8 dvr_get_rec_status( void )
{
	return g_rec_status;
}

__u8 dvr_get_park_status( void )
{
	return g_park_status;
}

__u8 dvr_get_mic_status( void )
{
	return g_mic_status;
}

__u8 dvr_get_loop_time( void )
{	
	return g_loop_time;
}

__u8 dvr_get_collision_status( void )
{	
	return g_collision_status;
}

__u8 dvr_get_cur_lock_status( void )
{	
	return g_lock_status;

}
__u8 dvr_get_sd_status( void )
{	
	return g_sd_status;

}

