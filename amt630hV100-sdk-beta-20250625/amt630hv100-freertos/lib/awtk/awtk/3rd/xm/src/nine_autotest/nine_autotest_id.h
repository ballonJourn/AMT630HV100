#ifndef	_AT_AUTOTEST_TAG_H_
#define	_AT_AUTOTEST_TAG_H_

#define	MAX_TAG_NAME		48

typedef struct _AT_TAG {
	char		tag_name[MAX_TAG_NAME];		// tag name
	int	  	tag_id;							// tag identifier
//	char		xml_name[MAX_TAG_NAME];		// XML Tag Name
} AT_TAG, *PAT_TAG;

#define	AT_UNDEF_TAG				0		// 未定义TAG
#define AT_POINTER					1
#define AT_UTF8_STRING				2
#define	AT_INT_CONST				3
#define	AT_FLOAT_CONST				4
#define AT_DELAY					5		// 延时
#define AT_IntervalTime				6		// 定义每条指令之间的间隙
#define AT_loop   7
#define AT_endloop  8
#define AT_include 9
#define AT_randi  10
#define AT_randf  11


enum {
	AT_NINE_EVENT_VCU_IDLE = 201,
  AT_NINE_EVENT_VCU_PHONE_CONNECTED,
	AT_NINE_EVENT_VCU_NOT_READY,

	AT_NINE_EVENT_VCU_READY,
	AT_PACK_UP_TEMPLE,
	AT_SIT_SIT_BARRELS,
	AT_PACK_UP_TEMPLE_AND_SIT_SIT_BARRELS,
	AT_NINE_EVENT_VCU_CYCLING_MODE,
	AT_NONE,
	AT_HELP_MOVE,
	AT_BACK_CAR,
	AT_NINE_EVENT_VCU_UNLOCK_MODE,
	AT_NINE_EVENT_VCU_RUNNING_STATUS,
	AT_POWER_OUTPUT,
	AT_SPEED,
	AT_STEERING_ANGLE,
	AT_NINE_EVENT_VCU_GEAR,
	AT_GEAR_ASSIT,
	AT_GEAR_ECO,
	AT_GEAR_COAST,
	AT_GEAR_FURIOUS,
	AT_NINE_EVENT_VCU_BLE_UPDATE_SYSTEM,
	AT_NINE_EVENT_WIFI_CONNECT,
	AT_STATE_DISCONNECT,
	AT_STATE_CONNECTING,
	AT_STATE_CONNECTED,
	AT_NINE_EVENT_UPDATE_PROCESS,
	AT_NINE_EVENT_UPDATE_FINISH,
	AT_NINE_EVENT_BASIC_ERROR_DETAILS,
	AT_code,
	AT_NINE_EVENT_BASIC_ERROR_CLOSE,
	AT_NINE_EVENT_WEATHER_DETAILS,
		AT_INTERVAL,
		AT_SIGN,
		AT_TEMP,
		AT_SCALE,
		AT_ALERT,
		AT_TYPE,
		AT_TIME,
		AT_TITLE,	// 标题, 允许多条
		AT_text,
	AT_NINE_EVENT_WEATHER_CLOSE,
	AT_NINE_EVENT_LOW_BATTERY,
  AT_NINE_EVENT_KEY_AUTO,   // auto键
  AT_NINE_EVENT_KEY_LONG_AUTO,  // 长按auto键, 读取剩余锁定时间
	AT_NINE_EVENT_KEY_TRIGGER,
	AT_NINE_EVENT_KEY_WHEEL,
	AT_NINE_EVENT_KEY_BREAK,    // 刹车键
	AT_NINE_EVENT_KEY_ADD,      // +键
	AT_NINE_EVENT_KEY_SUB,      // -键
	AT_NINE_EVENT_KEY_USER,     // 自定义键
	AT_NINE_EVENT_POWER_DISSIPATION,
	AT_data,
  AT_index, 
  AT_NINE_EVENT_POWER_DISSIPATION_REALTIME,
    AT_value,
  AT_NINE_EVENT_TRAFFIC_DATA,
    AT_distance,
  AT_NINE_EVENT_NAVIGATION,
		AT_totalDistance,
		AT_retainDistance,
		AT_retainTime,
		AT_iconType,
		AT_curStepRetainDis,
		AT_trafficLightNum,
		AT_gpsStrength,
		AT_currentRoadName,
		AT_nextRoadName,
		AT_NavigationText,
	AT_NINE_EVENT_HIGH_BEAM,
		AT_on,
		AT_off,
	AT_NINE_EVENT_LOW_BEAM,
	AT_NINE_EVENT_LEFT_TURN,
	AT_NINE_EVENT_RIGHT_TURN,
	AT_NINE_EVENT_ABS,
	AT_NINE_EVENT_CRUISE_CONTROL,
	AT_NINE_EVENT_READY_HINT,
	AT_NINE_EVENT_GSM,
		AT_no_signal,
		AT_one,
		AT_two,
		AT_full,
	AT_NINE_EVENT_GPS,
		AT_strong,
		AT_weak,
	AT_NINE_EVENT_BLE1,
	AT_NINE_EVENT_BLE2,
	AT_NINE_EVENT_DATETIME,
	  AT_msec,
	  AT_second,
	  AT_minute,
	  AT_hour,
	  AT_day,
	  AT_wday,
	  AT_month,
	  AT_year,
	AT_NINE_EVENT_ODO,
	AT_NINE_EVENT_TRIP,
	AT_NINE_EVENT_LIGHT_SWITCH,   // 光敏日夜切换
	  AT_night,
	AT_NINE_EVENT_THEME_SWITCH,   // 主题切换
  AT_NINE_EVENT_PASSWORD,     // 密码校验
  AT_NINE_EVENT_PAGE_SWITCH,
  AT_NINE_EVENT_REMAINING_MILEAGE,  // 电池剩余里程
  AT_NINE_EVENT_CHARGE_TIME, // 剩余充电事件及百分比
    AT_remaining_time,
    AT_percent,
};



typedef struct _TOKEN {
	char			*string; 					// string offset
	int			count;						// string count
	int			id;							// token id
	int			value;						// 数字的值
	float			f_value;						// 浮点值
} TOKEN, *PTOKEN	;

typedef struct _AT_TAG_STRUCT		*PAT_TAG_STRUCT;
/* tag's structure */
typedef struct _AT_TAG_STRUCT {
	int		id;
	char		*string; 					// string offset
	int		count;						// string count

	PAT_TAG_STRUCT	tag_parent;		// pointer to parent
	PAT_TAG_STRUCT	tag_child;  	// pointer to first child
	PAT_TAG_STRUCT	tag_sibling;	// pointer to next sibling
} AT_TAG_STRUCT;

void at_init_parser (unsigned char *data_to_parser);

TOKEN* AT_NextToken(void);
AT_TAG_STRUCT* AddChild (AT_TAG_STRUCT *parent, AT_TAG_STRUCT *child);
AT_TAG_STRUCT *AT_NewTag (TOKEN *token);
int nine_AT_Define(void);
char *GetIspTagName (int tag_id);

extern TOKEN *curr_token;

#define CurrToken(token)	\
	curr_token = token;


#endif