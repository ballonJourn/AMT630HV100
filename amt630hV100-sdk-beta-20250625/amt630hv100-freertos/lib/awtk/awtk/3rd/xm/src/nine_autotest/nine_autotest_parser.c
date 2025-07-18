#ifdef NINE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "nine_autotest_id.h"
#include "nine/nine_event.h"
#include <XM_event.h>

nine_event_t nine_at_event;

// 指令之间间隔时间
static int intervalTime = 0;



static  AT_TAG_STRUCT *AT_NewTag (TOKEN *token)
{
	AT_TAG_STRUCT *tag;
	if(token == NULL)
		return NULL;
	if(token->id == 0)
		return NULL;

	tag = (AT_TAG_STRUCT *)malloc (sizeof(AT_TAG_STRUCT));
	if(!tag)
		return NULL;

	memset (tag, 0, sizeof(AT_TAG_STRUCT));
	tag->id = token->id;
//	tag->raw_string = strdup(token->raw_string);
	return tag;
}

static  AT_TAG_STRUCT* AddChild (AT_TAG_STRUCT *parent, AT_TAG_STRUCT *child)
{
	if(!child || !parent)
		return NULL;

	child->tag_parent = parent;

	if(parent->tag_child)
	{
		AT_TAG_STRUCT *last = parent->tag_child;
		while(last->tag_sibling)
			last = last->tag_sibling;
		last->tag_sibling = child;
	}
	else
	{
		parent->tag_child = child;
	}

	return child;
}

// 延时, 最小延时1毫秒, 最大延时10分钟
// delay : 100 // 延时100ms
// delay : 1000 // 延时1秒
// delay : 10000 // 延时10秒
static  int Define_DELAY (TOKEN *tk)
{
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if (token->id == '\n')
			break;
		else if (token->id == ':')
		{
			continue;
		}

		else if (token->id == AT_INT_CONST)
		{
			int delay_ms = token->value;
			if (delay_ms < 1)
				delay_ms = 1;
			if (delay_ms > 10 * 60 * 1000)	// 最大延时10分钟
				delay_ms = 10 * 60 * 1000;
			Sleep(delay_ms);
		}
		else
		{
			CurrToken(token);		/* push token into stack */
			break;
		}
	}


	return 1;
}

// 间隔时间, 最小延时1毫秒, 最大延时1分钟
// intervalTime : 100 // 间隔时间100ms
// intervalTime : 1000 // 间隔时间1秒
static  int Define_IntervalTime(TOKEN *tk)
{
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if (token->id == '\n')
			break;
		else if (token->id == ':')
		{
			continue;
		}

		else if (token->id == AT_INT_CONST)
		{
			int delay_ms = token->value;
			if (delay_ms < 1)
				delay_ms = 1;
			if (delay_ms > 60 * 1000)	// 最大延时1分钟
				delay_ms = 60 * 1000;
			intervalTime = delay_ms;
		}
		else
		{
			CurrToken(token);		/* push token into stack */
			break;
		}
	}


	return 1;
}

// 手机已连接
// VCU_PHONE_CONNECTED 	
static  int Define_VCU_PHONE_CONNECTED(TOKEN *tk)
{
  int ret = 0;
  TOKEN *token;

  while (token = AT_NextToken())
  {
    if (token->id == '\n')
    {
      ret = 1;
      break;
    }
    else
    {
      ret = 0;
      CurrToken(token);		/* push token into stack */
      break;
    }
  }

  if (ret)
    nine_at_event.type = NINE_EVENT_VCU_PHONE_CONNECTED;

  return ret;
}

// 激活
// VCU_READY 	
static  int Define_VCU_READY(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(ret)
	  nine_at_event.type = NINE_EVENT_VCU_READY;

	return ret;
}

// 未激活
// VCU_NOT_READY 	: PACK_UP_TEMPLE
// VCU_NOT_READY	: SIT_SIT_BARRELS
// VCU_NOT_READY	:	PACK_UP_TEMPLE_AND_SIT_SIT_BARRELS
static int Define_VCU_NOT_READY(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(	token->id == AT_PACK_UP_TEMPLE )
		{
			nine_at_event.not_ready_event.state = NINE_VCU_NOT_READY_STATE_PACK_UP_TEMPLE; 
			ret = 1;
		}
		else if(token->id == AT_SIT_SIT_BARRELS)
		{
			nine_at_event.not_ready_event.state = NINE_VCU_NOT_READY_STATE_SIT_SIT_BARRELS; 
			ret = 1;
		}
		else if(	token->id == AT_PACK_UP_TEMPLE_AND_SIT_SIT_BARRELS
				)
		{
			nine_at_event.not_ready_event.state = NINE_VCU_NOT_READY_STATE_PACK_UP_TEMPLE_AND_SIT_SIT_BARRELS;
			ret = 1; 
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(ret)
	  nine_at_event.type = NINE_EVENT_VCU_NOT_READY;

	return ret;
}

// 骑行模式
// VCU_CYCLING_MODE : NONE			// 非骑行模式
// VCU_CYCLING_MODE	: HELP_MOVE	// 助力推行
// VCU_CYCLING_MODE	:	BACK_CAR	// 倒车中
static int Define_VCU_CYCLING_MODE(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(	token->id == AT_NONE
				  )
		{
			nine_at_event.cycling_event.mode = NINE_VCU_CYCLING_MODE_NONE; 
			ret = 1;
		}
		else if(token->id == AT_HELP_MOVE)
		{
			nine_at_event.cycling_event.mode = NINE_VCU_CYCLING_MODE_HELP_MOVE; 
			ret = 1;
		}
		else if(	token->id == AT_BACK_CAR
				)
		{
			nine_at_event.cycling_event.mode = NINE_VCU_CYCLING_MODE_BACK_CAR; 
			ret = 1;
		}
		else 
		{
			CurrToken (token);		/* push token into stack */
			ret = 0;
			break;
		}
	}
	
	if(ret)
    nine_at_event.type = NINE_EVENT_VCU_CYCLING_MODE;
	  

	return ret;
}

// 解除
// VCU_UNLOCK_MODE 	
static int Define_VCU_UNLOCK_MODE(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_VCU_UNLOCK_MODE;
    
	return ret;
}

static int Define_VCU_RUNNING_STATUS_setting (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int tk_id = tk->id;
	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(token->id == AT_INT_CONST)
		{
			if(tk_id == AT_POWER_OUTPUT)
				nine_at_event.running_status_event.power_output = token->value;
			else if(tk_id == AT_SPEED)
				nine_at_event.running_status_event.speed = token->value;
			else if(tk_id == AT_STEERING_ANGLE)
				nine_at_event.running_status_event.steering_angle = token->value;
			ret = 1;  // OK
		}
		else if(token->id == AT_FLOAT_CONST)
		{
			if(tk_id == AT_POWER_OUTPUT)
				nine_at_event.running_status_event.power_output = token->f_value;
			else if(tk_id == AT_SPEED)
				nine_at_event.running_status_event.speed = token->f_value;
			else if(tk_id == AT_STEERING_ANGLE)
				nine_at_event.running_status_event.steering_angle = token->f_value;
			ret = 1;  // OK
		}
		else
		{
		  ret = 0;
      CurrToken (token);		/* push token into stack */
      break;
		}
	}
	return ret;			  
}

// 运行中状态  (输出功率、速度、转向角）
// VCU_RUNNING_STATUS
// POWER_OUTPUT : 100.0   // 功率输出
// SPEED : 30.1   // 速度
// STEERING_ANGLE : 0.0 // 转向角
static int Define_VCU_RUNNING_STATUS(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	nine_at_event.running_status_event.power_output = 0.0f;
	nine_at_event.running_status_event.speed = 0.0f;
	nine_at_event.running_status_event.steering_angle = 0.0f;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			continue;

		else if(	token->id == AT_POWER_OUTPUT 
		    || token->id == AT_SPEED
		    || token->id == AT_STEERING_ANGLE
				  )
		{
			int id = token->id;
		  if(Define_VCU_RUNNING_STATUS_setting(token) == 0)
		  {
		    printf ("Define_VCU_RUNNING_STATUS parser failed\n");
		    ret = 0;
		  }
		  else
		  {
			if(id == AT_POWER_OUTPUT)
			  ret |= 0x01; 
			else if (id == AT_SPEED)
				ret |= 0x02;
			else if (id == AT_STEERING_ANGLE)
				ret |= 0x04;
		  }
		}
		else 
		{
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(ret == 0x07)
	{
	  nine_at_event.type = NINE_EVENT_VCU_RUNNING_STATUS;
	}   

	return ret;
}

// 档位 
// VCU_GEAR : ASSIT
// VCU_GEAR : ECO
// VCU_GEAR : COAST
// VCU_GEAR : FURIOUS
static int Define_VCU_GEAR (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(	token->id == AT_GEAR_ASSIT
				  )
		{
			nine_at_event.gear_event.gear = NINE_VCU_GEAR_ASSIT; 
			ret = 1;
		}
		else if(	token->id == AT_GEAR_ECO
				  )
		{
			nine_at_event.gear_event.gear = NINE_VCU_GEAR_ECO; 
			ret = 1;
		}
		else if(token->id == AT_GEAR_COAST)
		{
			nine_at_event.gear_event.gear = NINE_VCU_GEAR_COAST; 
			ret = 1;
		}
		else if(	token->id == AT_GEAR_FURIOUS
				)
		{
			nine_at_event.gear_event.gear = NINE_VCU_GEAR_FURIOUS;  
			ret = 1;
		}
		else 
		{
			CurrToken (token);		/* push token into stack */
			ret = 0;
			break;
		}
	}
	
	if(ret)
    nine_at_event.type = NINE_EVENT_VCU_GEAR;
	  

	return ret;
}
// APP发起升级
// VCU_BLE_UPDATE_SYSTEM 	
static int Define_VCU_BLE_UPDATE_SYSTEM(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_VCU_BLE_UPDATE_SYSTEM;
    
	return ret;
}

// WIFI连接
// WIFI_CONNECT : DISCONNECT	// WIFI未连接
// WIFI_CONNECT	: CONNECTING	// WIFI连接中
// WIFI_CONNECT	: CONNECTED	  // WIFI已连接
static int Define_WIFI_CONNECT (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(	token->id == AT_STATE_DISCONNECT	  )
		{
			nine_at_event.wifi_event.state = NINE_WIFI_STATE_DISCONNECT; 
			ret = 1;
		}
		else if(token->id == AT_STATE_CONNECTING)
		{
			nine_at_event.wifi_event.state = NINE_WIFI_STATE_CONNECTING; 
			ret = 1;
		}
		else if(	token->id == AT_STATE_CONNECTED			)
		{
			nine_at_event.wifi_event.state = NINE_WIFI_STATE_CONNECTED; 
			ret = 1;
		}
		else 
		{
			CurrToken (token);		/* push token into stack */
			ret = 0;
			break;
		}
	}
	
	if(ret)
    nine_at_event.type = NINE_EVENT_WIFI_CONNECT;
	  

	return ret;
}


// 升级进度  
// UPDATE_PROCESS : 1
// UPDATE_PROCESS : 50.1
// UPDATE_PROCESS : 100
static int Define_UPDATE_PROCESS (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;
		else if(token->id == ':')
		{
			continue;
		}

		else if(	token->id == AT_INT_CONST	 )
		{
		  nine_at_event.update_progress_event.step = token->value;
		  ret = 1;
		}
		else if(	token->id == AT_FLOAT_CONST	 )
		{
		  nine_at_event.update_progress_event.step = token->f_value;
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(ret == 1)
	{
	  nine_at_event.type = NINE_EVENT_UPDATE_PROCESS;
	}   

	return ret;
}

// 升级完成
// UPDATE_FINISH : 0    // 升级失败
// UPDATE_FINISH : 1	  // 升级成功
static int Define_UPDATE_FINISH (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  break;
		}
		else if(token->id == ':')
		{
			continue;
		}
		else if(	token->id == AT_INT_CONST	 )
		{
		  nine_at_event.update_finish_event.result = token->value;
		  ret = 1;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_UPDATE_FINISH;
    
	return ret;
}

// code : 1
// code : 10
static int Define_code (int event_id, TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int id = tk->id;
	int type_count = 0;
	int title_count = 0;

	memset(&nine_at_event, 0, sizeof(nine_at_event));

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  break;
		}
		else if(token->id == ':')
		{
			continue;
		}
		else if(	token->id == AT_INT_CONST	 )
		{
		  if(event_id == AT_NINE_EVENT_BASIC_ERROR_DETAILS)
		  {
		    nine_at_event.basic_error_event.code = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_NINE_EVENT_WEATHER_DETAILS)
		  {
			  if(id == AT_INTERVAL)
			    nine_at_event.weather_details_event.INTERVAL = token->value;
			  else if(id == AT_SIGN)
			    nine_at_event.weather_details_event.SIGN = token->value;
			  else if (id == AT_TEMP)
				  nine_at_event.weather_details_event.TEMP = token->value;
			  else if (id == AT_SCALE)
				  nine_at_event.weather_details_event.SCALE = token->value;
			  else if (id == AT_ALERT)
				  nine_at_event.weather_details_event.ALERT = token->value;
			  else if (id == AT_TYPE)
				  nine_at_event.weather_details_event.TYPE[type_count ++] = token->value;
			  else if (id == AT_TIME)
				  nine_at_event.weather_details_event.TIME = token->value;
			  ret = 1;
		  }
      else if (event_id == AT_NINE_EVENT_POWER_DISSIPATION_REALTIME)
      {
        if (id == AT_index)
        {
          nine_at_event.power_dissipation_event.index = token->value;
          ret = 1;
        }
      }
      else if (event_id == AT_NINE_EVENT_CHARGE_TIME)
      {
        if (id == AT_remaining_time)
        {
          nine_at_event.charge_time_event.remaining_time = token->value;
          ret = 1;
        }
        else if (id == AT_percent)
        {
          nine_at_event.charge_time_event.percent = token->value;
          ret = 1;
        }
      }
      else if (event_id == AT_NINE_EVENT_TRAFFIC_DATA)
      {
        if (id == AT_distance)
          nine_at_event.traffic_data_event.distance = token->value;
        else if (id == AT_TIME)
          nine_at_event.traffic_data_event.time = token->value;
        else if (id == AT_SPEED)
          nine_at_event.traffic_data_event.speed = token->value;
        ret = 1;
      }
		  else if(event_id == AT_totalDistance)
		  {
		    nine_at_event.navigation_event.totalDistance = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_retainDistance)
		  {
		    nine_at_event.navigation_event.retainDistance = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_retainTime)
		  {
		    nine_at_event.navigation_event.retainTime = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_iconType)
		  {
		    nine_at_event.navigation_event.iconType = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_curStepRetainDis)
		  {
		    nine_at_event.navigation_event.curStepRetainDis = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_trafficLightNum)
		  {
		    nine_at_event.navigation_event.trafficLightNum = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_gpsStrength)
		  {
		    nine_at_event.navigation_event.gpsStrength = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_msec)
		  {
		    nine_at_event.date_time_event.msec = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_second)
		  {
		    nine_at_event.date_time_event.second = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_minute)
		  {
		    nine_at_event.date_time_event.minute = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_hour)
		  {
		    nine_at_event.date_time_event.hour = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_day)
		  {
		    nine_at_event.date_time_event.day = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_wday)
		  {
		    nine_at_event.date_time_event.wday = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_month)
		  {
		    nine_at_event.date_time_event.month = token->value;
		    ret = 1;
		  }
		  else if(event_id == AT_year)
		  {
		    nine_at_event.date_time_event.year = token->value;
		    ret = 1;
		  }
		  
		  else
		  {
		    ret = 0; 
		  }
		}  
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
    
	return ret;
}



// UTF字符串
static int Define_text (int event_id, TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int id = tk->id;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  break;
		}
		else if(token->id == ':')
		{
			continue;
		}
		else if(	token->id == AT_UTF8_STRING	 )
		{
		  if(event_id == AT_NINE_EVENT_WEATHER_DETAILS)
		  {
		    memset (&nine_at_event.weather_details_event.text, 0, sizeof(nine_at_event.weather_details_event.text));
		    if(token->count >= 256)
		      token->count = 255;
		    memcpy (nine_at_event.weather_details_event.text, token->string, token->count);
		    ret = 1;
		  }
		  else if(event_id == AT_currentRoadName)
		  {
		    memset (&nine_at_event.navigation_event.currentRoadName, 0, sizeof(nine_at_event.navigation_event.currentRoadName));
		    if(token->count >= 64)
		      token->count = 63;
		    memcpy (nine_at_event.navigation_event.currentRoadName, token->string, token->count);
		    ret = 1;
		  }
		  else if(event_id == AT_nextRoadName)
		  {
		    memset (&nine_at_event.navigation_event.nextRoadName, 0, sizeof(nine_at_event.navigation_event.nextRoadName));
		    if(token->count >= 64)
		      token->count = 63;
		    memcpy (nine_at_event.navigation_event.nextRoadName, token->string, token->count);
		    ret = 1;
		  }
      else if (event_id == AT_NINE_EVENT_BASIC_ERROR_DETAILS)
      {
        memset(&nine_at_event.basic_error_event.text, 0, sizeof(nine_at_event.basic_error_event.text));
        if (token->count >= sizeof(nine_at_event.basic_error_event.text))
          token->count = sizeof(nine_at_event.basic_error_event.text) - 1;
        memcpy(nine_at_event.basic_error_event.text, token->string, token->count);
        ret = 1;
      }
		  
		  else
		  {
		    ret = 0; 
		  }
		}  
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
    
	return ret;
}


// 基本错误详情
// BASIC_ERROR_DETAILS
// code : 1		// 错误码 1
// code : 10    // 错误码 10
// TEXT @ 当前温度过高,请将车辆转移至适宜环境
static int Define_BASIC_ERROR_DETAILS (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  continue;
		}
		else if(token->id == ':')
		{
			continue;
		}
		else if(	token->id == AT_code	 )
		{
		  if(Define_code (AT_NINE_EVENT_BASIC_ERROR_DETAILS, token) == 1)
		    ret |= 0x01;
		}
		else if (token->id == AT_text)
		{
			if (Define_text(AT_NINE_EVENT_BASIC_ERROR_DETAILS, token) == 1)
				ret |= 0x2;
			else
			{
				ret = 0;
				break;
			}
		}
		else
		{
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if((ret & 0x03) == 0x03)
	  nine_at_event.type = NINE_EVENT_BASIC_ERROR_DETAILS;
    
	return ret;
}

// 基本错误详情收起
// BASIC_ERROR_CLOSE
static int Define_BASIC_ERROR_CLOSE(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_BASIC_ERROR_CLOSE;
    
	return ret;
}

// UTF字符串
// TITLE @ 20分钟又暴雨袭, 请注意不要在低洼处停车.
// TITLE @ 10分钟后出现冰雹
static int Define_title(int event_id, TOKEN *tk)
{
	int ret = 0;
	TOKEN *token;
	int id = tk->id;

	while (token = AT_NextToken())
	{
		if (token->id == '\n')
		{
			break;
		}
		else if (token->id == ':')
		{
			continue;
		}
		else if (token->id == AT_UTF8_STRING)
		{
			if (event_id == AT_NINE_EVENT_WEATHER_DETAILS)
			{
				int used = strlen(nine_at_event.weather_details_event.title);
				int free = sizeof(nine_at_event.weather_details_event.title) - 1 - used;	// 保留一个字节存放结束'\0'
				if (used)
				{
					// 存在多个标题
					if (token->count > (free - 1))	// 增加1个换行字符 '\n'
					{
						// 超出缓冲区大小
						continue;
					}
					nine_at_event.weather_details_event.title[used] = '\n';
					used++;

				}
				else
				{
					if (token->count > (free))
					{
						// 超出缓冲区大小
						continue;
					}
				}
				memcpy(nine_at_event.weather_details_event.title + used, token->string, token->count);
				ret = 1;
			}
			else
			{
				ret = 0;
			}
		}
		else
		{
			ret = 0;
			CurrToken(token);		/* push token into stack */
			break;
		}
	}

	return ret;
}

// 恶劣天气详情
// WEATHER_DETAILS
// INTERVAL ; 10	// 请求间隔, 最小10, 最大60, 分钟
// SIGN : 0			// 天气预报标识 0 ~ 11
// TEMP : -50		// 天气预报温度 -50 ~ 100
// SCALE : 0		// 天气预报风力等级 0 ~ 12
// ALERT : 1		// 天气预报是否预警  0x01是 0x02否
// TYPE : 0			// 天气预警类型 0 ~ 13
// TIME : 0			// 天气预警时间（uint32_t 时间戳）
// TITLE :	@雷雨警告	// 天气预警标题
// TEXT : @20分钟又暴雨袭,请注意不要在低洼处停车.	
static int Define_WEATHER_DETAILS (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	memset(&nine_at_event, 0, sizeof(nine_at_event));
	memset(&nine_at_event.weather_details_event.TYPE, 0xff, sizeof(nine_at_event.weather_details_event.TYPE));

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  continue;
		}
		else if(token->id == ':')
		{
			continue;
		}
		else if(	token->id == AT_INTERVAL)
		{
		  if(Define_code (AT_NINE_EVENT_WEATHER_DETAILS, token) == 1)
		    ret |= 0x01;
		  else
		  {
			  ret = 0;
			  break;
		  }
		}
		else if (token->id == AT_SIGN)
		{
			if (Define_code(AT_NINE_EVENT_WEATHER_DETAILS, token) == 1)
				ret |= 0x02;
			else
			{
				ret = 0;
				break;
			}
		}
		else if (token->id == AT_TEMP)
		{
			if (Define_code(AT_NINE_EVENT_WEATHER_DETAILS, token) == 1)
				ret |= 0x04;
			else
			{
				ret = 0;
				break;
			}
		}
		else if (token->id == AT_SCALE)
		{
			if (Define_code(AT_NINE_EVENT_WEATHER_DETAILS, token) == 1)
				ret |= 0x08;
			else
			{
				ret = 0;
				break;
			}
		}
		else if (token->id == AT_ALERT)
		{
			if (Define_code(AT_NINE_EVENT_WEATHER_DETAILS, token) == 1)
				ret |= 0x10;
			else
			{
				ret = 0;
				break;
			}
		}
		else if (token->id == AT_TYPE)
		{
			if (Define_code(AT_NINE_EVENT_WEATHER_DETAILS, token) == 1)
				ret |= 0x20;
			else
			{
				ret = 0;
				break;
			}
		}
		else if (token->id == AT_TIME)
		{
			if (Define_code(AT_NINE_EVENT_WEATHER_DETAILS, token) == 1)
				ret |= 0x40;
			else
			{
				ret = 0;
				break;
			}
		}
		else if(	token->id == AT_text	 )
		{
		  if(Define_text (AT_NINE_EVENT_WEATHER_DETAILS, token) == 1)
		    ret |= 0x80;
		  else
		  {
			  ret = 0;
			  break;
		  }
		}  
		else if (token->id == AT_TITLE)
		{
			if (Define_title(AT_NINE_EVENT_WEATHER_DETAILS, token) == 1)
				ret |= 0x100;
			else
			{
				ret = 0;
				break;
			}
		}
		else
		{
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_WEATHER_DETAILS;
    
	return ret;
}

// WEATHER_CLOSE
// 天气预警详情关起
static int Define_WEATHER_CLOSE(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_WEATHER_CLOSE;
    
	return ret;
}

// 单位为mv
// 表示第一块电池电压为3100mv, 第二块电池电压为3200mv, 第三块电池电压为5000mv
// LOW_BATTERY : 3100 3200 5000  
// 表示第二块电池电压为3200mv
// LOW_BATTERY : -1 3200 -1  
static int Define_LOW_BATTERY (TOKEN *tk)
{
  int ret = 0;
  int count = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;
		else if(token->id == ':')
		{
			continue;
		}

		else if(	token->id == AT_INT_CONST	 )
		{
		  nine_at_event.low_battery_event.votage[count ++] = token->value;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(count == 3)
	{
	  nine_at_event.type = NINE_EVENT_LOW_BATTERY;
	  ret = 1;
	}   

	return ret;
}

// PAGE_SWITCH  页面切换
// PAGE_SWITCH 1   // 切换到页标识为1的页面
// PAGE_SWITCH 3   // 切换到页标识为3的页面
static int Define_PAGE_SWITCH(TOKEN *tk)
{
  int ret = 0;
  int count = 0;
  TOKEN *token;

  while (token = AT_NextToken())
  {
    if (token->id == '\n')
      break;
    else if (token->id == ':')
    {
      continue;
    }

    else if (token->id == AT_INT_CONST)
    {
      nine_at_event.page_switch_event.page = token->value;
      ret = 1;
    }
    else
    {
      ret = 0;
      CurrToken(token);		/* push token into stack */
      break;
    }
  }

  if (ret)
  {
    nine_at_event.type = NINE_EVENT_PAGE_SWITCH;
  }

  return ret;
}

// PASSWORD  密码校验结果
// PASSWORD 0   // 密码校验失败
// PASSWORD 1   // 密码校验成功
static int Define_PASSWORD(TOKEN *tk)
{
  int ret = 0;
  int count = 0;
  TOKEN *token;

  while (token = AT_NextToken())
  {
    if (token->id == '\n')
      break;
    else if (token->id == ':')
    {
      continue;
    }

    else if (token->id == AT_INT_CONST)
    {
      nine_at_event.password_event.result = token->value;
      ret = 1;
    }
    else
    {
      ret = 0;
      CurrToken(token);		/* push token into stack */
      break;
    }
  }

  if (ret)
  {
    nine_at_event.type = NINE_EVENT_PASSWORD;
  }

  return ret;
}

// long_auto键  长按AUTO事件, 密码解锁
// KEY_LONG_AUTO 0   // 剩余锁定时间 0秒
// KEY_LONG_AUTO 60   // 剩余锁定时间 60秒
static int Define_KEY_LONG_AUTO (TOKEN *tk)
{
  int ret = 0;
  int count = 0;
  TOKEN *token;

  while (token = AT_NextToken())
  {
    if (token->id == '\n')
      break;
    else if (token->id == ':')
    {
      continue;
    }

    else if (token->id == AT_INT_CONST)
    {
      nine_at_event.key_long_auto_event.locked_time = token->value;
      ret = 1;
    }
    else
    {
      ret = 0;
      CurrToken(token);		/* push token into stack */
      break;
    }
  }

  if (ret)
  {
    nine_at_event.type = NINE_EVENT_KEY_LONG_AUTO;
  }

  return ret;
}

// REMAINING_MILEAGE : 1    // 电池剩余里程 1公里
// REMAINING_MILEAGE : 10   // 电池剩余里程 10公里
static int Define_REMAINING_MILEAGE(TOKEN *tk)
{
  int ret = 0;
  int count = 0;
  TOKEN *token;

  while (token = AT_NextToken())
  {
    if (token->id == '\n')
      break;
    else if (token->id == ':')
    {
      continue;
    }

    else if (token->id == AT_INT_CONST)
    {
      nine_at_event.remaining_mileage_event.remaining_mileage = token->value;
      ret = 1;
    }
    else
    {
      ret = 0;
      CurrToken(token);		/* push token into stack */
      break;
    }
  }

  if (ret)
  {
    nine_at_event.type = NINE_EVENT_REMAINING_MILEAGE;
  }

  return ret;
}

// CHARGE_TIME
// remaining_time : 10 // 10分钟
// percent : 30   // 百分比
static int Define_CHARGE_TIME(TOKEN *tk)
{
  int ret = 0;
  int count = 0;
  TOKEN *token;

  while (token = AT_NextToken())
  {
    if (token->id == '\n')
      continue;
    else if (token->id == ':')
    {
      continue;
    }
    else if (token->id == AT_remaining_time)
    {
      if (Define_code(AT_NINE_EVENT_CHARGE_TIME, token) == 1)
        ret |= 0x01;
    }
    else if (token->id == AT_percent)
    {
      if (Define_code(AT_NINE_EVENT_CHARGE_TIME, token) == 1)
        ret |= 0x02;
    }

    else
    {
      CurrToken(token);		/* push token into stack */
      break;
    }
  }

  if ((ret & 0x03) == 0x03)
  {
    nine_at_event.type = NINE_EVENT_CHARGE_TIME;
  }

  return ret;
}

// auto键
// KEY_AUTO
static int Define_KEY_AUTO(TOKEN *tk)
{
  int ret = 0;
  TOKEN *token;

  while (token = AT_NextToken())
  {
    if (token->id == '\n')
    {
      ret = 1;
      break;
    }
    else
    {
      ret = 0;
      CurrToken(token);		/* push token into stack */
      break;
    }
  }

  if (ret)
    nine_at_event.type = NINE_EVENT_KEY_AUTO;

  return ret;
}

// 扳机键
// KEY_TRIGGER
static int Define_KEY_TRIGGER (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_KEY_TRIGGER;
    
	return ret;
}

// 滚轮
// KEY_WHEEL
static int Define_KEY_WHEEL (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_KEY_WHEEL;
    
	return ret;
}

// 刹车键
// KEY_BREAK
static int Define_KEY_BREAK (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_KEY_BREAK;
    
	return ret;
}

// +键
// KEY_ADD
static int Define_KEY_ADD (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_KEY_ADD;
    
	return ret;
}

// -键
// KEY_SUB
static int Define_KEY_SUB (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_KEY_SUB;
    
	return ret;
}

// 自定义键
// KEY_USER
static int Define_KEY_USER (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  ret = 1;
		  break;
		}
    else
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if(ret)
	  nine_at_event.type = NINE_EVENT_KEY_USER;
    
	return ret;
}


// 能耗曲线 (1024个数据, 每行20个, 25行)
// POWER_DISSIPATION :
// index : 12 //取值索引，即是从该位置开始往后取30个点
// 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 
// 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 
// 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 
// 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 
// 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 
// 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 
// 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 120 
static int Define_POWER_DISSIPATION(TOKEN *tk)
{
	int ret = 0;
	int count = 0;
	TOKEN *token;

	nine_at_event.power_dissipation_event.count = 0;

	while (token = AT_NextToken())
	{
		if (token->id == '\n')
		{
			continue;
		}
		else if (token->id == ':')
		{
			continue;
		}
    else if (token->id == AT_index)
    {
      if (Define_code(AT_NINE_EVENT_POWER_DISSIPATION, token) == 1)
        ret |= 0x01;
    }

		else if (token->id == AT_INT_CONST)
		{
      if (nine_at_event.power_dissipation_event.count < 1024)
      {
        nine_at_event.power_dissipation_event.data[nine_at_event.power_dissipation_event.count++] = token->value;
        ret |= 0x02;
      }
		}
		else
		{
			ret = 0;
			CurrToken(token);		/* push token into stack */
			break;
		}
	}

	if ((ret & 0x03) == 0x03)
	{
		nine_at_event.type = NINE_EVENT_POWER_DISSIPATION;
	}

	return ret;
}

// POWER_DISSIPATION_REALTIME : 0
// POWER_DISSIPATION_REALTIME : 100
static int Define_POWER_DISSIPATION_REALTIME(TOKEN *tk)
{
  int ret = 0;
  TOKEN *token;

  while (token = AT_NextToken())
  {
    if (token->id == '\n')
    {
      break;
    }
    else if (token->id == ':')
    {
      continue;
    }

    else if (token->id == AT_INT_CONST)
    {
      nine_at_event.power_dissipation_realtime_event.value = token->value;
      ret = 1;
    }
    else
    {
      ret = 0;
      CurrToken(token);		/* push token into stack */
      break;
    }
  }

  if (ret)
  {
    nine_at_event.type = NINE_EVENT_POWER_DISSIPATION_REALTIME;
  }

  return ret;
}

// 行驶数据
// TRAFFIC_DATA
// distance : 320000
// time : 21000
// speed : 1000
static int Define_TRAFFIC_DATA(TOKEN *tk)
{
  int ret = 0;
  TOKEN *token;

  while (token = AT_NextToken())
  {
    if (token->id == '\n')
    {
      continue;
    }
    else if (token->id == AT_distance)
    {
      if (Define_code(AT_NINE_EVENT_TRAFFIC_DATA, token) == 1)
        ret |= 0x01;
    }
    else if (token->id == AT_TIME)
    {
      if (Define_code(AT_NINE_EVENT_TRAFFIC_DATA, token) == 1)
        ret |= 0x02;
    }
    else if (token->id == AT_SPEED)
    {
      if (Define_code(AT_NINE_EVENT_TRAFFIC_DATA, token) == 1)
        ret |= 0x04;
    }
    else
    {
      CurrToken(token);		/* push token into stack */
      break;
    }
  }

  if ((ret & 0x7) == 0x7)
    nine_at_event.type = NINE_EVENT_TRAFFIC_DATA;

  return ret;
}

// 导航事件
// NAVIGATION
// totalDistance : 320000
// retainDistance : 21000
// retainTime : 1000
// iconType : 0
// curStepRetainDis : 300
// trafficLightNum : 2
// gpsStrength : 0
// currentRoadName @ 南海大道
// nextRoadName @工业7路
static int Define_NAVIGATION (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  continue;
		}
		else if(	token->id == AT_totalDistance	 )
		{
		  if(Define_code (AT_totalDistance, token) == 1)
		    ret |= 0x01;
		}
		else if(	token->id == AT_retainDistance	 )
		{
		  if(Define_code (AT_retainDistance, token) == 1)
		    ret |= 0x02;
		}  
		else if(	token->id == AT_retainTime	 )
		{
		  if(Define_code (AT_retainTime, token) == 1)
		    ret |= 0x04;
		}  
		else if(	token->id == AT_iconType	 )
		{
		  if(Define_code (AT_iconType, token) == 1)
		    ret |= 0x08;
		}  
		else if(	token->id == AT_curStepRetainDis	 )
		{
		  if(Define_code (AT_curStepRetainDis, token) == 1)
		    ret |= 0x10;
		}  
		else if(	token->id == AT_trafficLightNum	 )
		{
		  if(Define_code (AT_trafficLightNum, token) == 1)
		    ret |= 0x20;
		}  
		else if(	token->id == AT_gpsStrength	 )
		{
		  if(Define_code (AT_gpsStrength, token) == 1)
		    ret |= 0x40;
		}  
		else if(	token->id == AT_currentRoadName	 )
		{
		  if(Define_text (AT_currentRoadName, token) == 1)
		    ret |= 0x80;
		}  
		else if(	token->id == AT_nextRoadName	 )
		{
		  if(Define_text (AT_nextRoadName, token) == 1)
		    ret |= 0x100;
		}  
    else
		{
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if((ret & 0x1ff) == 0x1ff)
	  nine_at_event.type = NINE_EVENT_NAVIGATION;
    
	return ret;
}

// 远光灯
// HIGH_BEAM : on
// HIGH_BEAM : off
static int Define_HIGH_BEAM(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int tk_id = tk->id;

	AT_TAG_STRUCT *tag = NULL;
	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(token->id == AT_on)
		{
		  nine_at_event.high_beam_event.on = 1;			
		  ret = 1;
		}
		else if(token->id == AT_off)
		{
		  nine_at_event.high_beam_event.on = 0;			
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(ret)
	  nine_at_event.type = NINE_EVENT_HIGH_BEAM;

	return ret;
}

// 近光灯
// LOW_BEAM : on
// LOW_BEAM : off
static int Define_LOW_BEAM(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int tk_id = tk->id;

	AT_TAG_STRUCT *tag = NULL;
	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(token->id == AT_on)
		{
		  nine_at_event.low_beam_event.on = 1;			
		  ret = 1;
		}
		else if(token->id == AT_off)
		{
		  nine_at_event.low_beam_event.on = 0;			
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	if(ret)
	  nine_at_event.type = NINE_EVENT_LOW_BEAM;

	return ret;
}

// 左转向
// left_TURN : on
// left_TURN : off
static int Define_LEFT_TURN(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int tk_id = tk->id;

	AT_TAG_STRUCT *tag = NULL;
	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(token->id == AT_on)
		{
		  nine_at_event.left_turn_event.on = 1;			
		  ret = 1;
		}
		else if(token->id == AT_off)
		{
		  nine_at_event.left_turn_event.on = 0;			
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	if(ret)
	  nine_at_event.type = NINE_EVENT_LEFT_TURN;

	return ret;
}

// 右转向
// right_TURN : on
// right_TURN : off
static int Define_RIGHT_TURN(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int tk_id = tk->id;

	AT_TAG_STRUCT *tag = NULL;
	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(token->id == AT_on)
		{
		  nine_at_event.right_turn_event.on = 1;			
		  ret = 1;
		}
		else if(token->id == AT_off)
		{
		  nine_at_event.right_turn_event.on = 0;			
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	if(ret)
	  nine_at_event.type = NINE_EVENT_RIGHT_TURN;

	return ret;
}

// ABS
// abs : on
// abs : off
static int Define_ABS(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int tk_id = tk->id;

	AT_TAG_STRUCT *tag = NULL;
	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(token->id == AT_on)
		{
		  nine_at_event.abs_event.on = 1;			
		  ret = 1;
		}
		else if(token->id == AT_off)
		{
		  nine_at_event.abs_event.on = 0;			
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	if(ret)
	  nine_at_event.type = NINE_EVENT_ABS;

	return ret;
}

// 定速巡航
// CRUISE_CONTROL : on
// CRUISE_CONTROL : off
static int Define_CRUISE_CONTROL(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int tk_id = tk->id;

	AT_TAG_STRUCT *tag = NULL;
	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(token->id == AT_on)
		{
		  nine_at_event.cruise_control_event.on = 1;			
		  ret = 1;
		}
		else if(token->id == AT_off)
		{
		  nine_at_event.cruise_control_event.on = 0;			
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	if(ret)
	  nine_at_event.type = NINE_EVENT_CRUISE_CONTROL;

	return ret;
}

// GSM信号
// GSM : NO_SIGNAL	// 无信号
// GSM : ONE		// 强度一格
// GSM : TWO		// 强度二格
// GSM : FULL		// 强度满格
static int Define_GSM(TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int tk_id = tk->id;

	AT_TAG_STRUCT *tag = NULL;
	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(token->id == AT_no_signal)
		{
		  nine_at_event.gsm_event.state = NINE_GSM_STATE_NO_SIGNAL;			
		  ret = 1;
		}
		else if(token->id == AT_one)
		{
		  nine_at_event.gsm_event.state = NINE_GSM_STATE_ONE;						
		  ret = 1;
		}
		else if(token->id == AT_two)
		{
		  nine_at_event.gsm_event.state = NINE_GSM_STATE_TWO;						
		  ret = 1;
		}
		else if(token->id == AT_full)
		{
		  nine_at_event.gsm_event.state = NINE_GSM_STATE_FULL;						
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	if(ret)
	  nine_at_event.type = NINE_EVENT_GSM;

	return ret;
}

// GPS信号
// GPS : NO_SIGNAL		// 无信号
// GPS : STRONG			// 信号强
// GPS : WEAK			// 信号弱
static int Define_GPS (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;
	int tk_id = tk->id;

	AT_TAG_STRUCT *tag = NULL;
	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;

		else if(token->id == ':')
		{
			continue;
		}

		else if(token->id == AT_no_signal)
		{
		  nine_at_event.gps_event.state = NINE_GPS_STATE_NO_SIGNAL;			
		  ret = 1;
		}
		else if(token->id == AT_strong)
		{
		  nine_at_event.gps_event.state = NINE_GPS_STATE_STRONG;						
		  ret = 1;
		}
		else if(token->id == AT_weak)
		{
		  nine_at_event.gps_event.state = NINE_GPS_STATE_WEAK;						
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	if(ret)
	  nine_at_event.type = NINE_EVENT_GPS;

	return ret;
}

// 系统时间事件
// DATETIME
// msec : 100
// second : 59
// minute : 10
// hour : 12
// day : 31
// wday : 1
// month : 12
// year : 2022
static int Define_DATETIME (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
		{
		  continue;
		}
		else if(	token->id == AT_msec	 )
		{
		  if(Define_code (AT_msec, token) == 1)
		    ret |= 0x01;
		}
		else if(	token->id == AT_second	 )
		{
		  if(Define_code (AT_second, token) == 1)
		    ret |= 0x02;
		}  
		else if(	token->id == AT_minute	 )
		{
		  if(Define_code (AT_minute, token) == 1)
		    ret |= 0x04;
		}  
		else if(	token->id == AT_hour	 )
		{
		  if(Define_code (AT_hour, token) == 1)
		    ret |= 0x08;
		}  
		else if(	token->id == AT_day	 )
		{
		  if(Define_code (AT_day, token) == 1)
		    ret |= 0x10;
		}  
		else if(	token->id == AT_wday	 )
		{
		  if(Define_code (AT_wday, token) == 1)
		    ret |= 0x20;
		}  
		else if(	token->id == AT_month	 )
		{
		  if(Define_code (AT_month, token) == 1)
		    ret |= 0x40;
		}  
		else if(	token->id == AT_year	 )
		{
		  if(Define_code (AT_year, token) == 1)
		    ret |= 0x80;
		}  
    else
		{
			CurrToken (token);		/* push token into stack */
			break;
		}
	}

  if((ret & 0xff) == 0xff)
	  nine_at_event.type = NINE_EVENT_DATETIME;
    
	return ret;
}

// ODO  
// ODO : 1000
static int Define_ODO (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;
		else if(token->id == ':')
		{
			continue;
		}

		else if(	token->id == AT_INT_CONST	 )
		{
		  nine_at_event.odo_event.odo = token->value;
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(ret == 1)
	{
	  nine_at_event.type = NINE_EVENT_ODO;
	}   

	return ret;
}

// TRIP  
// TRIP : 1000
static int Define_TRIP (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;
		else if(token->id == ':')
		{
			continue;
		}

		else if(	token->id == AT_INT_CONST	 )
		{
		  nine_at_event.trip_event.trip = token->value;
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(ret == 1)
	{
	  nine_at_event.type = NINE_EVENT_TRIP;
	}   

	return ret;
}

// LIGHT_SWITCH  
// LIGHT_SWITCH : day // 切换到白天
// LIGHT_SWITCH : night // 切换到黑夜
static int Define_LIGHT_SWITCH (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;
		else if(token->id == ':')
		{
			continue;
		}
		else if(	token->id == AT_day	 )
		{
		  nine_at_event.light_switch_event.mode = 0; // 白天
		  ret = 1;
		}
		else if(	token->id == AT_night	 )
		{
		  nine_at_event.light_switch_event.mode = 1; // 夜晚
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(ret == 1)
	{
	  nine_at_event.type = NINE_EVENT_LIGHT_SWITCH;
	}   

	return ret;
}

// THEME_SWITCH  // 主题切换, 总共支持3套主题
// THEME_SWITCH : 1 // 切换到主题1
// THEME_SWITCH : 2 // 切换到主题2
// THEME_SWITCH : 3 // 切换到主题3
static int Define_THEME_SWITCH (TOKEN *tk)
{
  int ret = 0;
	TOKEN *token;

	while (token = AT_NextToken())
	{
		if(token->id == '\n')
			break;
		else if(token->id == ':')
		{
			continue;
		}
		else if(	token->id == AT_INT_CONST	 )
		{
		  nine_at_event.theme_switch_event.theme = token->value;
		  ret = 1;
		}
		else 
		{
		  ret = 0;
			CurrToken (token);		/* push token into stack */
			break;
		}
	}
	
	if(ret == 1)
	{
	  nine_at_event.type = NINE_EVENT_THEME_SWITCH;
	}   

	return ret;
}

extern int nine_at_define(unsigned  char *script_data);

// loop : 100 // 循环100次
// ...
// endloop
//
static int Define_loop(TOKEN *tk)
{
  int loop = 0;
  int ret = 0;
  TOKEN saved;
  TOKEN *token;
  char *repeat_script_start;
  char *repeat_script_end;
  char end_char;

  int recursion_count = 1;
  
  // 获取循环体的次数
  while (token = AT_NextToken())
  {
    if (token->id == '\n')
      break;
    else if (token->id == ':')
    {
      continue;
    }
    else if (token->id == AT_INT_CONST)
    {
      loop = token->value;
    }
  }

  // 记录循环体开始的位置
  token = AT_NextToken();
  repeat_script_start = token->string;

  // 定位endloop
  while (1)
  {
    // 考虑递归包含
    if (token->id == AT_endloop)
    {
      recursion_count--;
    }
    else if (token->id == AT_loop)
    {
      recursion_count++;
    }
    if (recursion_count == 0)
      break;

    token = AT_NextToken();
  }
  // 定位循环体结束后的下一个token起始位置
  token = AT_NextToken();
  repeat_script_end = token->string;
  // 保存隔断符号 repeat_script_end的内容
  saved = *token;
  end_char = *repeat_script_end;
  *repeat_script_end = '\0';

  // 执行循环体, loop为循环的次数
  while (loop > 0)
  {
    nine_at_define(repeat_script_start);
    loop--;
  }

  // 恢复隔断符号 repeat_script_end的内容
  *repeat_script_end = end_char;

  // 定位到循环体结束后的下一个token起始位置
  token = &saved;
  // 标记该token为下一步解析的起始位置
  CurrToken(token);
  ret = 1;
  return ret;
}

int nine_at_define (unsigned  char *script_data)
{
	TOKEN* token;
	int ret;
  if (script_data[0] == 0xEF && script_data[1] == 0xBB && script_data[2] == 0xBF)
    script_data += 3;
  at_init_parser((unsigned char *)script_data);
  while (token = AT_NextToken())
  {
    memset(&nine_at_event, 0, sizeof(nine_at_event));
    ret = 0;
    int id = token->id;
    if (token->id == AT_NINE_EVENT_VCU_READY)
      ret = Define_VCU_READY(token);
    else if (token->id == AT_NINE_EVENT_VCU_PHONE_CONNECTED)
      ret = Define_VCU_PHONE_CONNECTED(token);
    else if (token->id == AT_NINE_EVENT_VCU_NOT_READY)
      ret = Define_VCU_NOT_READY(token);
    else if (token->id == AT_NINE_EVENT_VCU_CYCLING_MODE)
      ret = Define_VCU_CYCLING_MODE(token);
    else if (token->id == AT_NINE_EVENT_VCU_UNLOCK_MODE)
      ret = Define_VCU_UNLOCK_MODE(token);
    else if (token->id == AT_NINE_EVENT_VCU_RUNNING_STATUS)
      ret = Define_VCU_RUNNING_STATUS(token);
    else if (token->id == AT_NINE_EVENT_VCU_GEAR)
      ret = Define_VCU_GEAR(token);
    else if (token->id == AT_NINE_EVENT_VCU_BLE_UPDATE_SYSTEM)
      ret = Define_VCU_BLE_UPDATE_SYSTEM(token);
    else if (token->id == AT_NINE_EVENT_WIFI_CONNECT)
      ret = Define_WIFI_CONNECT(token);
    else if (token->id == AT_NINE_EVENT_UPDATE_PROCESS)
      ret = Define_UPDATE_PROCESS(token);
    else if (token->id == AT_NINE_EVENT_UPDATE_FINISH)
      ret = Define_UPDATE_FINISH(token);
    else if (token->id == AT_NINE_EVENT_BASIC_ERROR_DETAILS)
      ret = Define_BASIC_ERROR_DETAILS(token);
    else if (token->id == AT_NINE_EVENT_BASIC_ERROR_CLOSE)
      ret = Define_BASIC_ERROR_CLOSE(token);
    else if (token->id == AT_NINE_EVENT_WEATHER_DETAILS)
      ret = Define_WEATHER_DETAILS(token);
    else if (token->id == AT_NINE_EVENT_WEATHER_CLOSE)
      ret = Define_WEATHER_CLOSE(token);
    else if (token->id == AT_NINE_EVENT_LOW_BATTERY)
      ret = Define_LOW_BATTERY(token);
    else if (token->id == AT_NINE_EVENT_PAGE_SWITCH)
      ret = Define_PAGE_SWITCH(token);
    else if (token->id == AT_NINE_EVENT_PASSWORD)
      ret = Define_PASSWORD(token);
    else if (token->id == AT_NINE_EVENT_KEY_AUTO)
      ret = Define_KEY_AUTO(token);
    else if (token->id == AT_NINE_EVENT_KEY_LONG_AUTO)
      ret = Define_KEY_LONG_AUTO(token);
    else if (token->id == AT_NINE_EVENT_KEY_TRIGGER)
      ret = Define_KEY_TRIGGER(token);
    else if (token->id == AT_NINE_EVENT_KEY_WHEEL)
      ret = Define_KEY_WHEEL(token);
    else if (token->id == AT_NINE_EVENT_KEY_BREAK)
      ret = Define_KEY_BREAK(token);
    else if (token->id == AT_NINE_EVENT_KEY_ADD)
      ret = Define_KEY_ADD(token);
    else if (token->id == AT_NINE_EVENT_KEY_SUB)
      ret = Define_KEY_SUB(token);
    else if (token->id == AT_NINE_EVENT_KEY_USER)
      ret = Define_KEY_USER(token);
    else if (token->id == AT_NINE_EVENT_POWER_DISSIPATION)
      ret = Define_POWER_DISSIPATION(token);
    else if (token->id == AT_NINE_EVENT_POWER_DISSIPATION_REALTIME)
      ret = Define_POWER_DISSIPATION_REALTIME(token);
    else if (token->id == AT_NINE_EVENT_TRAFFIC_DATA)
      ret = Define_TRAFFIC_DATA(token);
    else if (token->id == AT_NINE_EVENT_NAVIGATION)
      ret = Define_NAVIGATION(token);
    else if (token->id == AT_NINE_EVENT_HIGH_BEAM)
      ret = Define_HIGH_BEAM(token);
    else if (token->id == AT_NINE_EVENT_LOW_BEAM)
      ret = Define_LOW_BEAM(token);
    else if (token->id == AT_NINE_EVENT_LEFT_TURN)
      ret = Define_LEFT_TURN(token);
    else if (token->id == AT_NINE_EVENT_RIGHT_TURN)
      ret = Define_RIGHT_TURN(token);
    else if (token->id == AT_NINE_EVENT_ABS)
      ret = Define_ABS(token);
    else if (token->id == AT_NINE_EVENT_CRUISE_CONTROL)
      ret = Define_CRUISE_CONTROL(token);
    else if (token->id == AT_NINE_EVENT_GSM)
      ret = Define_GSM(token);
    else if (token->id == AT_NINE_EVENT_GPS)
      ret = Define_GPS(token);
    else if (token->id == AT_DELAY)	// 延时指令
      ret = Define_DELAY(token);
    else if (token->id == AT_IntervalTime)	// 每条指令之间的间隙
      ret = Define_IntervalTime(token);
    else if (token->id == AT_NINE_EVENT_DATETIME)
      ret = Define_DATETIME(token);
    else if (token->id == AT_NINE_EVENT_ODO)
      ret = Define_ODO(token);
    else if (token->id == AT_NINE_EVENT_TRIP)
      ret = Define_TRIP(token);
    else if (token->id == AT_NINE_EVENT_LIGHT_SWITCH) // 光敏日夜切换
      ret = Define_LIGHT_SWITCH(token);
    else if (token->id == AT_NINE_EVENT_THEME_SWITCH) // 主题切换
      ret = Define_THEME_SWITCH(token);
    else if (token->id == AT_NINE_EVENT_REMAINING_MILEAGE) // 电池剩余里程
      ret = Define_REMAINING_MILEAGE(token);
    else if (token->id == AT_NINE_EVENT_CHARGE_TIME) // 充电剩余时间及百分比
      ret = Define_CHARGE_TIME(token);
    else if (token->id == AT_loop)
      ret = Define_loop(token);
    

    if (ret && nine_at_event.type)
    {
      // 发送事件   
      XM_NineEventProc(&nine_at_event, 0);

    }

    // 指令之间的间隔时间
    if (ret && intervalTime && id != AT_DELAY && id != '\n')
      Sleep(intervalTime);
  }

  return 0;
}
#endif