#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "nine_autotest_id.h"

enum CHAR_CLASSES { F_END, OTHER, SPACE, DIGIT, LETTER, ENTER};

static const unsigned char	charclass[] = {
	F_END,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	//  .......
	OTHER,	SPACE,	ENTER,	OTHER,	OTHER,	SPACE,	OTHER,	OTHER,	// .  _
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	// ........
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	// .. .....
	SPACE,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	//  !"#$%&'
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	//	()*+,-./
	DIGIT,	DIGIT,	DIGIT,	DIGIT,	DIGIT,	DIGIT,	DIGIT,	DIGIT,	// 01234567
	DIGIT,	DIGIT,	OTHER,	SPACE,	OTHER,	OTHER,	OTHER,	LETTER,	//	89:;<=>?
	OTHER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	//	@ABCDEFG
	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	// HIJKLMNO
	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	//	PQRSTUVW
	LETTER,	LETTER,	LETTER,	OTHER,	OTHER,	OTHER,	OTHER,	LETTER,	//	XYZ[\]^_
	OTHER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	//	`abcdefg
	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	// hijklmno
	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	// pqrstuvw
	LETTER,	LETTER,	LETTER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	// xyz{|}~
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	
};


static  AT_TAG  at_tag[] = {

  "loop", AT_loop,
  "endloop", AT_endloop,
  "include", AT_include,
  "randi",   AT_randi,
  "randf",   AT_randf,


	"VCU_IDLE",								AT_NINE_EVENT_VCU_IDLE,		
  "VCU_PHONE_CONNECTED",        AT_NINE_EVENT_VCU_PHONE_CONNECTED,
	"VCU_NOT_READY",							AT_NINE_EVENT_VCU_NOT_READY,
	"VCU_READY",								AT_NINE_EVENT_VCU_READY,
	"PACK_UP_TEMPLE",							AT_PACK_UP_TEMPLE,
	"SIT_SIT_BARRELS",							AT_SIT_SIT_BARRELS,
	"PACK_UP_TEMPLE_AND_SIT_SIT_BARRELS",								AT_PACK_UP_TEMPLE_AND_SIT_SIT_BARRELS,
	"VCU_CYCLING_MODE",							AT_NINE_EVENT_VCU_CYCLING_MODE,
	"NONE",							AT_NONE,
	"HELP_MOVE",						AT_HELP_MOVE,
	"BACK_CAR",							AT_BACK_CAR,
	"VCU_UNLOCK_MODE",							AT_NINE_EVENT_VCU_UNLOCK_MODE,
	"VCU_RUNNING_STATUS",							AT_NINE_EVENT_VCU_RUNNING_STATUS,
	"POWER_OUTPUT",							AT_POWER_OUTPUT,
	"SPEED",						AT_SPEED,
	"STEERING_ANGLE",					AT_STEERING_ANGLE,
	"VCU_GEAR",						AT_NINE_EVENT_VCU_GEAR,
	"ASSIT",            AT_GEAR_ASSIT,
	"ECO",							AT_GEAR_ECO,
	"COAST",								AT_GEAR_COAST,
	"FURIOUS",							AT_GEAR_FURIOUS,
	"VCU_BLE_UPDATE_SYSTEM",							AT_NINE_EVENT_VCU_BLE_UPDATE_SYSTEM,
	"WIFI_CONNECT",							AT_NINE_EVENT_WIFI_CONNECT,
	"DISCONNECT",						AT_STATE_DISCONNECT,
	"CONNECTING",							AT_STATE_CONNECTING,
	"CONNECTED",						AT_STATE_CONNECTED,
	"UPDATE_PROCESS",							AT_NINE_EVENT_UPDATE_PROCESS,
	"UPDATE_FINISH",						AT_NINE_EVENT_UPDATE_FINISH,
	"BASIC_ERROR_DETAILS",						AT_NINE_EVENT_BASIC_ERROR_DETAILS,	
	"code",							AT_code,	
	"BASIC_ERROR_CLOSE",							AT_NINE_EVENT_BASIC_ERROR_CLOSE,
	"WEATHER_DETAILS",								AT_NINE_EVENT_WEATHER_DETAILS,
	"INTERVAL",						AT_INTERVAL,
	"SIGN",			AT_SIGN,
	"TEMP",			AT_TEMP,
	"SCALE",		AT_SCALE,
	"ALERT",		AT_ALERT,
	"TYPE",			AT_TYPE,
	"TIME",			AT_TIME,
	"text",							AT_text,
	"WEATHER_CLOSE",							AT_NINE_EVENT_WEATHER_CLOSE,
	"LOW_BATTERY",							AT_NINE_EVENT_LOW_BATTERY,
  "KEY_AUTO",               AT_NINE_EVENT_KEY_AUTO,
  "KEY_LONG_AUTO",       AT_NINE_EVENT_KEY_LONG_AUTO,
	"KEY_TRIGGER",								AT_NINE_EVENT_KEY_TRIGGER,
	"KEY_WHEEL",							AT_NINE_EVENT_KEY_WHEEL,
	"KEY_BREAK",               AT_NINE_EVENT_KEY_BREAK,
	"KEY_ADD",               AT_NINE_EVENT_KEY_ADD,
	"KEY_SUB",               AT_NINE_EVENT_KEY_SUB,
	"KEY_USER",               AT_NINE_EVENT_KEY_USER,
	"POWER_DISSIPATION",							AT_NINE_EVENT_POWER_DISSIPATION,
  "data",             AT_data,
  "POWER_DISSIPATION_REALTIME", AT_NINE_EVENT_POWER_DISSIPATION_REALTIME,
  "value",    AT_value,
	"NAVIGATION",							AT_NINE_EVENT_NAVIGATION,
	"totalDistance",							AT_totalDistance,
	"retainDistance",							AT_retainDistance,
	"retainTime",							AT_retainTime,
	"iconType",								AT_iconType,
	"curStepRetainDis",							AT_curStepRetainDis,
	"trafficLightNum",						AT_trafficLightNum,
	"gpsStrength",							AT_gpsStrength,
	"currentRoadName",						AT_currentRoadName,
	"nextRoadName",							AT_nextRoadName,
	"NavigationText",							AT_NavigationText,
	"HIGH_BEAM",						AT_NINE_EVENT_HIGH_BEAM,
	"on",						AT_on,
	"off",						AT_off,
	"LOW_BEAM",							AT_NINE_EVENT_LOW_BEAM,
	"LEFT_TURN",							AT_NINE_EVENT_LEFT_TURN,
	"RIGHT_TURN",					AT_NINE_EVENT_RIGHT_TURN,
	"ABS",							AT_NINE_EVENT_ABS,
	"CRUISE_CONTROL",						AT_NINE_EVENT_CRUISE_CONTROL,
	"READY_HINT",						AT_NINE_EVENT_READY_HINT,
	"GSM",							AT_NINE_EVENT_GSM,
	"no_signal",							AT_no_signal,
	"one",							AT_one,
	"two",							AT_two,
	"full",							AT_full,
	"GPS",							AT_NINE_EVENT_GPS,
	"strong",							AT_strong,
	"weak",							AT_weak,
	"BLE1",								AT_NINE_EVENT_BLE1,
	"BLE2",							AT_NINE_EVENT_BLE2,
	"Delay",						AT_DELAY,
	"IntervalTime",						AT_IntervalTime,
	"DATETIME",       AT_NINE_EVENT_DATETIME,
	"msec",     AT_msec,
	"second",   AT_second,
	"minute",   AT_minute,
	"hour",     AT_hour,
	"day",      AT_day,
	"wday",     AT_wday,
	"month",    AT_month,
	"year",     AT_year,
	"ODO",     AT_NINE_EVENT_ODO,
	"TRIP",     AT_NINE_EVENT_TRIP,
	"LIGHT_SWITCH", AT_NINE_EVENT_LIGHT_SWITCH,
	"night",      AT_night,
  "THEME_SWITCH", AT_NINE_EVENT_THEME_SWITCH,
  "TRAFFIC_DATA", AT_NINE_EVENT_TRAFFIC_DATA,
  "distance", AT_distance,
  "PASSWORD", AT_NINE_EVENT_PASSWORD,
  "PAGE_SWITCH", AT_NINE_EVENT_PAGE_SWITCH,
  "TITLE", AT_TITLE,
  "index", AT_index,
  "REMAINING_MILEAGE", AT_NINE_EVENT_REMAINING_MILEAGE,
  "CHARGE_TIME", AT_NINE_EVENT_CHARGE_TIME,
    "remaining_time", AT_remaining_time,
    "percent", AT_percent,
	"", 0,
};

static TOKEN token;
TOKEN *curr_token;
static unsigned char *NextChar;


//#define	stricmp strcasecmp
static int AtTagComp (const void* a, const void* b)
{
	return stricmp (((AT_TAG *) a)->tag_name, ((AT_TAG *) b)->tag_name);
}

void AT_SortKeyword(void)
{
	qsort (at_tag, sizeof(at_tag)/sizeof(at_tag[0]), sizeof (at_tag[0]), AtTagComp);
}

void at_init_parser (unsigned char *data_to_parser)
{
	curr_token = NULL;
	NextChar	= data_to_parser;

	AT_SortKeyword();
}



static int GetAtTagID (char *name, int len)
{
	int			cc, lo, mid, hi;
	AT_TAG		*Ip;
	char			str[MAX_TAG_NAME+1];
	int			size;
	
	size = len;
	if(size > MAX_TAG_NAME)
		size = MAX_TAG_NAME;

	strncpy (str, name, size);
	str[size] = 0;


	// Binary search.
	lo = 0;
	hi = sizeof(at_tag)/sizeof(at_tag[0]) - 1;

	while (lo <= hi)
	{
		mid = (lo + hi) / 2;
		Ip = &at_tag[mid];
		cc = stricmp (str, Ip->tag_name);

		if (!cc)
			return Ip->tag_id;
		else if (cc < 0)
			hi = mid - 1;
		else
			lo = mid + 1;
	}

	printf ("GetAtTagID %s\n", str);
	//assert (0);
	return AT_UNDEF_TAG;				// Not found.
}


// 获取下一个语法基本单元TOKEN
TOKEN* AT_NextToken(void)
{
	int sign = 1;		// 符号位 +1, -1
	
	if(curr_token)
	{
		TOKEN *temp = curr_token;
		curr_token = NULL;
		return temp;
	}
	//curr_token = NULL;
	while ( 1 )
	{
		// 过滤连续的空格字符
		while(charclass[*NextChar] == SPACE)
		{
			NextChar ++;
		}

		/* end of file */
		if (*NextChar == 0)
		{
			return (0);
		} /* end if (c==EOF) */

		if(*NextChar == '\n')
		{
			token.id = '\n';
			token.string = (char *)NextChar;
			token.count = 1;
			NextChar ++;
			return &token;
		}

		// 处理注释
		if (*NextChar == '/' && *(NextChar+1) == '/')
		{
			// 注释行直到行尾
			NextChar += 2;
			while( *NextChar && *NextChar != '\r' && *NextChar != '\n' )
				NextChar ++;
			if(*NextChar == '\r')
				NextChar ++;
			continue;
		}

		if (*NextChar == '/' && *(NextChar+1) == ' ')
		{
			NextChar += 2;
		}

		if (*NextChar == '=' && *(NextChar+1) == '=' && *(NextChar+2) == '=')
		{
			// 注释行直到行尾
			NextChar += 3;
			while( *NextChar && *NextChar != '\r' && *NextChar != '\n' )
				NextChar ++;
			if(*NextChar == '\r')
				NextChar ++;
			continue;			
		}
		if (*NextChar == '(')
		{
			NextChar += 1;
			while( *NextChar && *NextChar != '\r' && *NextChar != '\n' )
				NextChar ++;
			if(*NextChar == '\r')
				NextChar ++;
			continue;
		}
		if (*NextChar == ';')
		{
			NextChar += 1;
			while( *NextChar && *NextChar != '\r' && *NextChar != '\n' )
				NextChar ++;
			if(*NextChar == '\r')
				NextChar ++;
			continue;
		}
		if(*NextChar == '=' && *(NextChar+1) == '>')
		{
			NextChar += 2;
			while( *NextChar && *NextChar != '\r' && *NextChar != '\n' )
				NextChar ++;
			if(*NextChar == '\r')
				NextChar ++;
			continue;
		}


		// 处理操作符
		if(*NextChar == ':')
		{
			token.id = ':';
			token.string = (char *)NextChar;
			token.count = 1;
			NextChar ++;
			return &token;
		}
		else if(*NextChar == '=')
		{
			token.id = '=';
			token.string = (char *)NextChar;
			token.count = 1;
			NextChar ++;
			return &token;
		}
		else if(*NextChar == '.')
		{
			token.id = '.';
			token.string = (char *)NextChar;
			token.count = 1;
			NextChar ++;
			return &token;
		}
		else if(*NextChar == ',')
		{
			token.id = ',';
			token.string = (char *)NextChar;
			token.count = 1;
			NextChar ++;
			return &token;
		}
		else if(*NextChar == '[' || *NextChar == ']')
		{
			token.id = *NextChar;
			token.string = (char *)NextChar;
			token.count = 1;
			NextChar ++;
			return &token;
		}
		else if(*NextChar == '-')
		{
			if( *(NextChar+1) == '>')
			{
				token.id = AT_POINTER;
				token.string = (char *)NextChar;
				token.count = 2;
				NextChar += 2;
				return &token;
			}
			else
			{
				token.id = '-';		// 负数
				token.string = (char *)NextChar;
				token.count = 1;
				NextChar += 1;
				
				// 过滤连续的空格字符
				while(charclass[*NextChar] == SPACE)
				{
					NextChar ++;
				}

				if(charclass[*NextChar] == DIGIT)
				{
					sign = -1;
					goto negative_number_process;
				}

				return &token;
			}
		}
		else if(*NextChar == '\\')
		{
			NextChar += 1;
			while( *NextChar && *NextChar != '\r' && *NextChar != '\n' )
				NextChar ++;
			if(*NextChar == '\r')
				NextChar ++;
			continue;
		}
		else if(*NextChar == '@')
		{
		  // UTF8 字符串
			NextChar += 1;
			char *str = (char *)NextChar;
			while( *NextChar && *NextChar != '\r' && *NextChar != '\n' )
				NextChar ++;
			token.id = AT_UTF8_STRING;	
			token.string = str;
			token.count = (char *)NextChar - str;
			return &token;
		    
		}    


		if(*NextChar == '0' && (*(NextChar+1) == 'x' || *(NextChar+1) == 'X') )
		{
			// 0x816c68a0
			unsigned int value = 0;
			char *str = (char *)NextChar;
			NextChar += 2;
			while(*NextChar && ( (*NextChar >= '0' && *NextChar <= '9') || (*NextChar >= 'a' && *NextChar <= 'f') || (*NextChar >= 'A' && *NextChar <= 'F')) )
			{
				value = value * 16 + ((*NextChar <= '9') ? (*NextChar - '0') : (toupper(*NextChar) - 'A' + 10));
				NextChar ++;
			}

			token.id = AT_INT_CONST;	
			token.value = value;
			token.string = str;
			token.count = (char *)NextChar - str;
			return &token;
		}

		if(charclass[*NextChar] == DIGIT)
		{
			// 正整数
			unsigned int value;
			char *str;

			sign = 1;

negative_number_process:

			value = 0;
			str = (char *)NextChar;
			while(*NextChar && (*NextChar >= '0' && *NextChar <= '9'))
			{
				value = value * 10 + *NextChar - '0';
				NextChar ++;
			}

			if(*NextChar == '.')	// 小数点
			{
				// 浮点数
				float f_value = (float)value;
				float f_base = 0.1f;
				NextChar ++;
				while(*NextChar && (*NextChar >= '0' && *NextChar <= '9'))
				{
					f_value = f_value + (*NextChar - '0') * f_base;
					NextChar ++;
					f_base = (float)(f_base / 10.0);
				}
				token.id = AT_FLOAT_CONST;	
				token.f_value = sign * f_value;
				token.string = str;
				token.count = (char *)NextChar - str;
				return &token;
				
			}
			else
			{
				// 正整数
				token.id = AT_INT_CONST;	
				token.value = sign * value;
				token.string = str;
				token.count = (char *)NextChar - str;
				return &token;
			}
		}

		if(charclass[*NextChar] == LETTER)
		{
			// 关键字
			char key[128];
			int len = 0;
			key[len ++] = *NextChar;
			NextChar ++;
			while( *NextChar && (charclass[*NextChar] == LETTER || charclass[*NextChar] == DIGIT || *NextChar == '_') )
			{
				key[len ++] = *NextChar;
				NextChar ++;
			}
			key[len] = 0;
			token.id = GetAtTagID (key, len);
			token.string = (char *)NextChar - len;
			token.count = len;
			return &token;
		}

		// 非法字符
		NextChar++;

	}
	//return 0;
}

char *AT_GetAtTagName (int tag_id)
{
	int i;
	for (i = 0; i < sizeof(at_tag)/sizeof(at_tag[0]); i++)
		if(at_tag[i].tag_id == tag_id)
			return at_tag[i].tag_name;
	
	return NULL;
}

