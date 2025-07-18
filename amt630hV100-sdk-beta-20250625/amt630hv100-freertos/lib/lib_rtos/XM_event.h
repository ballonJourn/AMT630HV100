#ifndef XM_EVENT_H
#define XM_EVENT_H



enum {
	XM_EVENT_FIRST = 0,
	XM_EVENT_KEYDOWN = 0,
	XM_EVENT_KEYUP,
	XM_EVENT_TOUCHDOWN,
	XM_EVENT_TOUCHUP,
	XM_EVENT_TOUCHMOVE,
	XM_EVENT_TEXTINPUT,
	
	XM_EVENT_TIMER,
	XM_EVENT_LAST
};

typedef struct XM_KeyboardEvent
{
    unsigned int type;        //  KEYDOWN or KEYUP
    unsigned int timestamp;   // In milliseconds
    unsigned int scancode;
    unsigned char press;        // PRESSED or RELEASED
    unsigned char repeat;       // Non-zero if this is a key repeat 
 } XM_KeyboardEvent;


typedef struct XM_TouchEvent
{
    unsigned int type;         // MOUSEMOTION
    unsigned int timestamp;    // In milliseconds
    //unsigned int press;       // PRESSED
    int x;           /**< X coordinate, relative to window */
    int y;           /**< Y coordinate, relative to window */
    int xrel;        /**< The relative motion in the X direction */
    int yrel;        /**< The relative motion in the Y direction */
} XM_TouchEvent;

#define XM_TEXTINPUTEVENT_TEXT_SIZE (32)
/**
*  \brief Keyboard text input event structure (event.text.*)
*/
typedef struct XM_TextInputEvent
{
	unsigned int type;                              /**< ::SDL_TEXTINPUT */
	unsigned int timestamp;                         /**< In milliseconds, populated using SDL_GetTicks() */
	char text[XM_TEXTINPUTEVENT_TEXT_SIZE];  /**< The input text */
} XM_TextInputEvent;

typedef struct _XM_EVENT {
	unsigned int type;
	union {
		XM_KeyboardEvent	key;
		XM_TouchEvent		tp;
		XM_TextInputEvent	text;
	};
} XM_EVENT;

// 投递触摸事件到事件消息队列
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
int XM_TpEventProc (unsigned int tp_event, unsigned int xPos, unsigned int yPos, unsigned int ticket);

// 投递按键事件到事件消息队列
// scancode     扫描码
// press        按键是否按下 1 按下 0 释放
// repeat       是否重复键
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
int XM_KeyEventProc(unsigned int key_event, unsigned int scancode, unsigned char press, unsigned char repeat, unsigned int ticket);

// 投递UTF8字符串事件到事件消息队列
// text         UTF8编码的字符串, 以'\0'结束
int XM_TextInputEventProc(const char *text, unsigned int ticket);


// 等待外部事件
int XM_WaitEvent (XM_EVENT *event, unsigned int timeout);

#endif 