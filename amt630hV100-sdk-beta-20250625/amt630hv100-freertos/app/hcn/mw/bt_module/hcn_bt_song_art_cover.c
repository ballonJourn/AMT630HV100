/**
*
* @file hcn_bt_song_art_cover.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/11/05 12:04
* @author och
*
*/

#include <FreeRTOS.h>
#include "task.h"
#include "queue.h"
#include "timers.h"
#include <string.h>
#include "log/hcn_log.h"
#include "msg_manage/hcn_msg_manage.h"
#include "bt_module/hcn_bt_song_art_cover.h"
#include "ff_stdio.h"

//#define SAVE_SONG_ART_COVER_ENABLE

#define SONG_ART_COVER_MAX_LEN  (42*1000)  ///< 顾凯专辑图片大小最大为42000字节

static char *photo_buff = NULL;

#ifdef SAVE_SONG_ART_COVER_ENABLE
static FF_FILE * song_art_cover_file = NULL;
static void save_song_art_cover(char *pic_data, int image_index, int image_len) {
    static int file_index = 0;
    if (hcn_get_usb_status() != USB_STATUS_INSERTED) {
        hcn_log_error("usb not inserted, cannot save song art cover pic!\r\n");
        return;
    }

    if (pic_data == NULL) {
        return;
    }
  
    if (song_art_cover_file == NULL) {
        char file_name[64] = {0};

        if (file_index > 100) {
            file_index = 0;
        }

        snprintf(file_name, sizeof(file_name), "/usb/%d.bin", image_index);
        file_index++;  
        song_art_cover_file = ff_fopen(file_name, "w+");
        if (song_art_cover_file == NULL) {
            hcn_log_error("open lane guide pic file failed!\n");
            return;
        }

        size_t ret = ff_fwrite(pic_data, 1, image_len, 
                                song_art_cover_file);
        if (ret != image_len) {
            hcn_log_error("write art cover pic file failed! ret:%d, len:%d\n", ret, image_len);
            ff_fclose(song_art_cover_file);
            song_art_cover_file = NULL;
            return;
        } 

        ff_fclose(song_art_cover_file);
        song_art_cover_file = NULL;

        hcn_log_info("\r\nsave song art cover success! len:%d\r\n", ret);
    }
}
#endif

void hcn_parse_avrcp_abulm_cover(bt_music_song_art_cover_t * art_cover, 
                                int photo_index) {
    if (art_cover == NULL) {
        hcn_log_error("song art cover pointer is null!\r\n");
        return;
    }

    static int last_phone_index = 0;

    if (photo_buff == NULL) {
        photo_buff = (char *)pvPortMalloc(SONG_ART_COVER_MAX_LEN);
        if (photo_buff == NULL) {
            hcn_log_error("Malloc song art cover buff failed!\r\n");
            return;
        }
    }

    if (photo_buff) {
        uint32_t pic_len = 0;
        extern void fscbt_get_coverart_data(char **data,unsigned int *len);
        fscbt_get_coverart_data(NULL, &pic_len);
        hcn_log_info("abulm len:%d\r\n", pic_len);

        if (pic_len < SONG_ART_COVER_MAX_LEN) {
            if (photo_index != last_phone_index) {
                last_phone_index = photo_index;
                memset(photo_buff, 0, SONG_ART_COVER_MAX_LEN);
                char *image = NULL;
                fscbt_get_coverart_data(&image, &pic_len);
                if (image) {
                    memcpy(photo_buff, image, pic_len);
                    art_cover->img_index = last_phone_index;
                    art_cover->img_height = 200;
                    art_cover->img_width = 200;
                    art_cover->image_len = (int)pic_len;
                    art_cover->image_buffer = photo_buff;
                    
                    #ifdef SAVE_SONG_ART_COVER_ENABLE
                    save_song_art_cover(photo_buff, last_phone_index, pic_len);
                    #endif
                    hcn_log_info("get song art cover success! len:%d\r\n", pic_len);
                }
            }
        } else {
            hcn_log_info("Song art cover is too long, do not save!\r\n");
            memset(photo_buff, 0, SONG_ART_COVER_MAX_LEN);
            art_cover->img_index = 0;
            art_cover->img_height = 200;
            art_cover->img_width = 200;
            art_cover->image_len = 0;
            art_cover->image_buffer = NULL;
        }
    }
}

void hcn_free_song_art_cover_buff(void) {
    if (photo_buff) {
        vPortFree(photo_buff); 
        photo_buff = NULL;
    }
}

