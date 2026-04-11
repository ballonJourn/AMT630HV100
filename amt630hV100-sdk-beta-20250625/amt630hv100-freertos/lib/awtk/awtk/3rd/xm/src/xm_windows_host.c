
// 用于Windows平台模拟
#if XM_WINDOWS_HOST
#include <Winsock2.h>
#include <windows.h>
#include "xm_windows_host.h"
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <VG/openvg.h>
#include <VG/vgu.h>
#include <VG/vgext.h> 
#include "xm_event.h"
#include <assert.h>
#include "xm_scancode.h"
#include "scancodes_windows.h"

#define CLASS_NAME 	"XM_VG"
#define WINDOW_TITLE "XM_VG"


// Windows variables
static HDC deviceContext = NULL;
static HWND nativeWindow = NULL;
static HINSTANCE applicationInstance;

static CRITICAL_SECTION event_critical_section;
static HANDLE event_handle;

#define	MAX_BUFFER_SIZE		0x2F

static XM_EVENT				eventBuffer[MAX_BUFFER_SIZE + 1];		// 循环队列
static volatile	int	   eventBPos;			// 循环队列首部
static volatile	int		eventEPos;			// 循环队列尾部

VGubyte vgSurfaceBitmapBuffer[sizeof(BITMAPINFO) + 16];
BITMAPINFO* vgSurfaceBitmapInfo;

static float dpi_ratio = 1.0;

static char pc_hw_sync_port_name[32] = "COM17";
static HANDLE handle_com;

#define   OK				1
#define   COMM_ERROR        0

// 串口
#define COMM_BUFFER_SIZE	1024
static CRITICAL_SECTION comm_critical_section;
static char* comm_tx_buffer[COMM_BUFFER_SIZE];
static char* comm_rx_buffer[COMM_BUFFER_SIZE];
static int comm_tx_count[COMM_BUFFER_SIZE];
static int comm_rx_count[COMM_BUFFER_SIZE];
static volatile int comm_tx_head, comm_tx_tail;
static volatile int comm_rx_head, comm_rx_tail;
static int pc_hw_sync_read(char *data, int size);
static void init_pc_autotest(void);	// 加入基于文本脚本的自动测试

HANDLE hCommThread;

static DWORD WINAPI pc_hw_sync_task(
  LPVOID lpParameter   // thread data
)
{
  while (1)
  {
    // 检查tx
    int tx_size = 0;
    char *tx_buff = NULL;
    int index;
    int count = 10;
    while (count > 0)
    {
      tx_buff = 0;
      tx_size = 0;

      EnterCriticalSection(&comm_critical_section);
      if (comm_tx_head < comm_tx_tail)
      {
        index = comm_tx_head % COMM_BUFFER_SIZE;
        assert(comm_tx_count[index] && comm_tx_buffer[index]);
        tx_size = comm_tx_count[index];
        comm_tx_count[index] = 0;
        tx_buff = comm_tx_buffer[index];
        comm_tx_buffer[index] = NULL;
        comm_tx_head++;
      }
      LeaveCriticalSection(&comm_critical_section);

      if (tx_buff)
      {
        DWORD dwBytesWrite = 0;
        WriteFile(handle_com, tx_buff, tx_size, &dwBytesWrite, NULL);
        free(tx_buff);
      }
      else
        break;
      count--;
    }


    char info[512];
    //MINMAXINFO* minmaxInfo;
    {
      int to_read = pc_hw_sync_read(info, 511);
      if (to_read)
      {
        info[to_read] = 0;
        printf("%s", info);
      }
    }

  }

}


static int init_pc_hw_sync_comm(void)
{
  char port_name[64];
  DCB  dcb;
  BOOL status;
  DWORD dwError = CE_IOE | CE_OVERRUN | CE_TXFULL;
  DWORD error_number;

  COMMTIMEOUTS comtimeout;

  InitializeCriticalSection(&comm_critical_section);
  memset(comm_tx_buffer, 0, sizeof(comm_tx_buffer));
  memset(comm_rx_buffer, 0, sizeof(comm_rx_buffer));
  memset(comm_tx_count, 0, sizeof(comm_tx_count));
  memset(comm_rx_count, 0, sizeof(comm_rx_count));

  comm_tx_head = 0;
  comm_tx_tail = 0;
  comm_rx_head = 0;
  comm_rx_tail = 0;

  status = OK;

  memset(port_name, 0, sizeof(port_name));
  sprintf(port_name, "\\\\.\\%s", pc_hw_sync_port_name);
  handle_com = CreateFile(port_name,
    GENERIC_WRITE | GENERIC_READ,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    OPEN_EXISTING,
    0,//NULL,
    NULL);
  if (handle_com == INVALID_HANDLE_VALUE)
  {
    printf("open pc_hw_sync port(%s) failed\r\n", pc_hw_sync_port_name);
    return -1;
  }
  if (!GetCommState(handle_com, &dcb))
  {
    CloseHandle(handle_com);
    handle_com = INVALID_HANDLE_VALUE;
    printf("can't get port %s's state\n", pc_hw_sync_port_name);
    return -1;
  }

  //需要设置超时，否则，WriteFile() 或ReadFile()会一直阻塞任务。、
  comtimeout.ReadIntervalTimeout = 1;	// 1ms
  comtimeout.ReadTotalTimeoutMultiplier = 1;	// 每次读操作总时间12ms
  comtimeout.ReadTotalTimeoutConstant = 0x0;

  comtimeout.WriteTotalTimeoutMultiplier = 1;//0x64; //100  // 表示平均写一个字节的时间上限
  comtimeout.WriteTotalTimeoutConstant = 1;//0x3e8; //992 //单位毫秒 //表示写数据总超时常量
  if (!SetCommTimeouts(handle_com, &comtimeout))
  {
    printf("SetCommTimeouts 失败\n");
  }

  dcb.BaudRate = 115200;// 115200
  dcb.ByteSize = 8;// 8
  dcb.Parity = NOPARITY;// NOPARITY
  dcb.StopBits = ONESTOPBIT;
  dcb.fOutxCtsFlow = 0; // CTS output flow control 指定CTS是否用于检测发送控制。当为TRUE时CTS为
              // OFF，发送将被挂起。（发送清除）
  dcb.fRtsControl = RTS_CONTROL_DISABLE;
  dcb.fDtrControl = DTR_CONTROL_DISABLE;
  dcb.EofChar = (char)0xa0;
  // 2014-07-17 增加
  dcb.fBinary = TRUE;  // binary mode, no EOF check　指定是否允许二进制模式WIN95中须为TRUE

  // 设置输入/输出缓冲区字节大小
  if (!SetupComm(handle_com, 2048, 2048))
  {
    printf("SetupComm 失败\n");
  }

  if (!SetCommState(handle_com, &dcb))
  {
    error_number = GetLastError();
    status = COMM_ERROR;
  }
  //    PurgeComm(handle_com, PURGE_TXABORT | PURGE_TXCLEAR); 
  //	ClearCommBreak(handle_com);
  //    ClearCommError(handle_com,&dwError,&commstat );

  if (status != OK)
  {
    CloseHandle(handle_com);
    handle_com = INVALID_HANDLE_VALUE;
    return status;
  }
  PurgeComm(handle_com, PURGE_RXCLEAR);  // 清除通信端口数据

  SetCommMask(handle_com, EV_RXCHAR);
  ClearCommError(handle_com, &dwError, NULL);
  //    WriteFile(handle_com,Clear_Ch,1,&dwBytes,NULL);


  hCommThread = CreateThread(NULL, 0x10000, pc_hw_sync_task, NULL, 0, NULL);
  SetThreadPriority(hCommThread, THREAD_PRIORITY_TIME_CRITICAL);

  return 0;
}

static int pc_hw_sync_write(const char *data, int size)
{
  DWORD dwBytesWrite = 0;
  int ret = -1;
  int index;

  EnterCriticalSection(&comm_critical_section);

  do {
    if (handle_com == INVALID_HANDLE_VALUE)
      break;
    if ((comm_tx_tail - comm_tx_head) >= COMM_BUFFER_SIZE)
    {
      // TX FIFO 已满
      break;
    }

    index = comm_tx_tail % COMM_BUFFER_SIZE;
    assert(comm_tx_buffer[index] == 0);
    assert(comm_tx_count[index] == 0);
    comm_tx_buffer[index] = malloc(size + 1);
    if (comm_tx_buffer[index])
    {
      memcpy(comm_tx_buffer[index], data, size);
      comm_tx_buffer[index][size] = 0;
      comm_tx_count[index] = size;
      comm_tx_tail++;
    }

    ret = 0;
  } while (0);

  LeaveCriticalSection(&comm_critical_section);
  return ret;
}

static int pc_hw_sync_read(char *data, int size)
{
  DWORD dwBytesRead = 0;
  BOOL status;

  if (handle_com == INVALID_HANDLE_VALUE)
    return 0;
  status = ReadFile(handle_com, data, size, &dwBytesRead, NULL);
  if (status && dwBytesRead)
    return dwBytesRead;
  else
    return 0;
}

// 投递触摸事件到事件消息队列
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
int XM_TpEventProc(unsigned int tp_event, unsigned int xPos, unsigned int yPos, unsigned int ticket)
{
  int ret;
  EnterCriticalSection(&event_critical_section);
  // 检查循环队列是否已满
  if (((eventEPos + 1) & MAX_BUFFER_SIZE) == (eventBPos & MAX_BUFFER_SIZE))
  {
    ret = 0;
  }
  else
  {
    XM_EVENT *event;
    // 将触摸事件插入到循环队列尾部
    event = eventBuffer + (eventEPos & MAX_BUFFER_SIZE);
    memset(event, 0, sizeof(XM_TouchEvent));
    event->type = tp_event;
    event->tp.timestamp = ticket;
    event->tp.type = tp_event;
    event->tp.x = xPos;
    event->tp.y = yPos;

    eventEPos++;

    // 计数值累加
    ReleaseSemaphore(event_handle, 1, NULL);

    ret = 1;
  }

  LeaveCriticalSection(&event_critical_section);
  return ret;
}

// 投递按键事件到事件消息队列
// scancode     扫描码
// press        按键是否按下 1 按下 0 释放
// repeat       是否重复键
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
int XM_KeyEventProc(unsigned int key_event, unsigned int scancode, unsigned char press, unsigned char repeat, unsigned int ticket)
{
  int ret;
  EnterCriticalSection(&event_critical_section);
  // 检查循环队列是否已满
  if (((eventEPos + 1) & MAX_BUFFER_SIZE) == (eventBPos & MAX_BUFFER_SIZE))
  {
    ret = 0;
  }
  else
  {
    XM_EVENT *event;
    // 将按键事件插入到循环队列尾部
    event = eventBuffer + (eventEPos & MAX_BUFFER_SIZE);
    memset(event, 0, sizeof(XM_TouchEvent));
    event->type = key_event;
    event->key.timestamp = ticket;
    event->key.type = key_event;
    event->key.scancode = scancode;
    event->key.press = press;
    event->key.repeat = repeat;

    eventEPos++;

    // 计数值累加
    ReleaseSemaphore(event_handle, 1, NULL);

    ret = 1;
  }

  LeaveCriticalSection(&event_critical_section);
  return ret;
}

#ifdef NINE
// 投递NINE事件到事件消息队列
// nine_event     nine事件
// ticket         事件时间戳
// 返回值定义
//		1		事件投递到事件缓冲队列成功
//		0		事件投递到事件缓冲队列失败
int XM_NineEventProc(nine_event_t *nine_event, unsigned int ticket)
{
  int ret;
  EnterCriticalSection(&event_critical_section);
  // 检查循环队列是否已满
  if (((eventEPos + 1) & MAX_BUFFER_SIZE) == (eventBPos & MAX_BUFFER_SIZE))
  {
    ret = 0;
  }
  else
  {
    XM_EVENT *event;
    // 将按键事件插入到循环队列尾部
    event = eventBuffer + (eventEPos & MAX_BUFFER_SIZE);
    memset(event, 0, sizeof(XM_NineEvent));
    event->type = XM_EVENT_NINE;
    event->nine.type = XM_EVENT_NINE;
    event->nine.timestamp = ticket;
    memcpy(&event->nine.event, nine_event, sizeof(nine_event_t));

    eventEPos++;

    // 计数值累加
    ReleaseSemaphore(event_handle, 1, NULL);

    ret = 1;
  }

  LeaveCriticalSection(&event_critical_section);
  return ret;
}
#endif

#define UTF8_IsLeadByte(c) ((c) >= 0xC0 && (c) <= 0xF4)
#define UTF8_IsTrailingByte(c) ((c) >= 0x80 && (c) <= 0xBF)

static int UTF8_TrailingBytes(unsigned char c)
{
  if (c >= 0xC0 && c <= 0xDF)
    return 1;
  else if (c >= 0xE0 && c <= 0xEF)
    return 2;
  else if (c >= 0xF0 && c <= 0xF4)
    return 3;
  else
    return 0;
}

size_t
XM_utf8strlcpy(char *dst, const char *src, size_t dst_bytes)
{
  size_t src_bytes = strlen(src);
  size_t bytes = min(src_bytes, dst_bytes - 1);
  size_t i = 0;
  char trailing_bytes = 0;
  if (bytes)
  {
    unsigned char c = (unsigned char)src[bytes - 1];
    if (UTF8_IsLeadByte(c))
      --bytes;
    else if (UTF8_IsTrailingByte(c))
    {
      for (i = bytes - 1; i != 0; --i)
      {
        c = (unsigned char)src[i];
        trailing_bytes = UTF8_TrailingBytes(c);
        if (trailing_bytes)
        {
          if (bytes - i != trailing_bytes + 1)
            bytes = i;

          break;
        }
      }
    }
    memcpy(dst, src, bytes);
  }
  dst[bytes] = '\0';
  return bytes;
}

#define XM_arraysize(array)    (sizeof(array)/sizeof(array[0]))

// 投递UTF8字符串事件到事件消息队列
// text         UTF8编码的字符串, 以'\0'结束
int XM_TextInputEventProc(const char *text, unsigned int ticket)
{
  int ret;
  EnterCriticalSection(&event_critical_section);
  // 检查循环队列是否已满
  if (((eventEPos + 1) & MAX_BUFFER_SIZE) == (eventBPos & MAX_BUFFER_SIZE))
  {
    ret = 0;
  }
  else
  {
    XM_EVENT *event;
    // 将按键事件插入到循环队列尾部
    event = eventBuffer + (eventEPos & MAX_BUFFER_SIZE);
    memset(event, 0, sizeof(XM_TouchEvent));
    event->type = XM_EVENT_TEXTINPUT;
    event->text.timestamp = ticket;
    event->text.type = XM_EVENT_TEXTINPUT;
    XM_utf8strlcpy(event->text.text, text, XM_arraysize(event->text.text));

    eventEPos++;

    // 计数值累加
    ReleaseSemaphore(event_handle, 1, NULL);

    ret = 1;
  }

  LeaveCriticalSection(&event_critical_section);
  return ret;
}


// 等待外部事件
int XM_WaitEvent(XM_EVENT *event, unsigned int timeout)
{
  int ret = 0;
  MSG msg;
  DWORD start_ticks = GetTickCount() + 1;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    /* Make sure we don't busy loop here forever if there are lots of events coming in */
    if (msg.time >= start_ticks)
      break;
  }
  if (WaitForSingleObject(event_handle, timeout) == WAIT_OBJECT_0)
  {
    EnterCriticalSection(&event_critical_section);
    if (eventBPos != eventEPos)
    {
      memcpy(event, eventBuffer + (eventBPos & MAX_BUFFER_SIZE), sizeof(XM_EVENT));
      eventBPos++;
      ret = 1;
    }
    LeaveCriticalSection(&event_critical_section);
  }
  return ret;
}

static XM_Scancode
VKeytoScancode(WPARAM vkey)
{
  switch (vkey) {
  case VK_CLEAR: return XM_SCANCODE_CLEAR;
  case VK_MODECHANGE: return XM_SCANCODE_MODE;
  case VK_SELECT: return XM_SCANCODE_SELECT;
  case VK_EXECUTE: return XM_SCANCODE_EXECUTE;
  case VK_HELP: return XM_SCANCODE_HELP;
  case VK_PAUSE: return XM_SCANCODE_PAUSE;
  case VK_NUMLOCK: return XM_SCANCODE_NUMLOCKCLEAR;

  case VK_F13: return XM_SCANCODE_F13;
  case VK_F14: return XM_SCANCODE_F14;
  case VK_F15: return XM_SCANCODE_F15;
  case VK_F16: return XM_SCANCODE_F16;
  case VK_F17: return XM_SCANCODE_F17;
  case VK_F18: return XM_SCANCODE_F18;
  case VK_F19: return XM_SCANCODE_F19;
  case VK_F20: return XM_SCANCODE_F20;
  case VK_F21: return XM_SCANCODE_F21;
  case VK_F22: return XM_SCANCODE_F22;
  case VK_F23: return XM_SCANCODE_F23;
  case VK_F24: return XM_SCANCODE_F24;

  case VK_OEM_NEC_EQUAL: return XM_SCANCODE_KP_EQUALS;
  case VK_BROWSER_BACK: return XM_SCANCODE_AC_BACK;
  case VK_BROWSER_FORWARD: return XM_SCANCODE_AC_FORWARD;
  case VK_BROWSER_REFRESH: return XM_SCANCODE_AC_REFRESH;
  case VK_BROWSER_STOP: return XM_SCANCODE_AC_STOP;
  case VK_BROWSER_SEARCH: return XM_SCANCODE_AC_SEARCH;
  case VK_BROWSER_FAVORITES: return XM_SCANCODE_AC_BOOKMARKS;
  case VK_BROWSER_HOME: return XM_SCANCODE_AC_HOME;
  case VK_VOLUME_MUTE: return XM_SCANCODE_AUDIOMUTE;
  case VK_VOLUME_DOWN: return XM_SCANCODE_VOLUMEDOWN;
  case VK_VOLUME_UP: return XM_SCANCODE_VOLUMEUP;

  case VK_MEDIA_NEXT_TRACK: return XM_SCANCODE_AUDIONEXT;
  case VK_MEDIA_PREV_TRACK: return XM_SCANCODE_AUDIOPREV;
  case VK_MEDIA_STOP: return XM_SCANCODE_AUDIOSTOP;
  case VK_MEDIA_PLAY_PAUSE: return XM_SCANCODE_AUDIOPLAY;
  case VK_LAUNCH_MAIL: return XM_SCANCODE_MAIL;
  case VK_LAUNCH_MEDIA_SELECT: return XM_SCANCODE_MEDIASELECT;

  case VK_OEM_102: return XM_SCANCODE_NONUSBACKSLASH;

  case VK_ATTN: return XM_SCANCODE_SYSREQ;
  case VK_CRSEL: return XM_SCANCODE_CRSEL;
  case VK_EXSEL: return XM_SCANCODE_EXSEL;
  case VK_OEM_CLEAR: return XM_SCANCODE_CLEAR;

  case VK_LAUNCH_APP1: return XM_SCANCODE_APP1;
  case VK_LAUNCH_APP2: return XM_SCANCODE_APP2;

  default: return XM_SCANCODE_UNKNOWN;
  }
}

static XM_Scancode
WindowsScanCodeToXMScanCode(LPARAM lParam, WPARAM wParam)
{
  XM_Scancode code;
  int nScanCode = (lParam >> 16) & 0xFF;
  int bIsExtended = (lParam & (1 << 24)) != 0;

  code = VKeytoScancode(wParam);

  if (code == XM_SCANCODE_UNKNOWN && nScanCode <= 127) {
    code = windows_scancode_table[nScanCode];

    if (bIsExtended) {
      switch (code) {
      case XM_SCANCODE_RETURN:
        code = XM_SCANCODE_KP_ENTER;
        break;
      case XM_SCANCODE_LALT:
        code = XM_SCANCODE_RALT;
        break;
      case XM_SCANCODE_LCTRL:
        code = XM_SCANCODE_RCTRL;
        break;
      case XM_SCANCODE_SLASH:
        code = XM_SCANCODE_KP_DIVIDE;
        break;
      case XM_SCANCODE_CAPSLOCK:
        code = XM_SCANCODE_KP_PLUS;
        break;
      default:
        break;
      }
    }
    else {
      switch (code) {
      case XM_SCANCODE_HOME:
        code = XM_SCANCODE_KP_7;
        break;
      case XM_SCANCODE_UP:
        code = XM_SCANCODE_KP_8;
        break;
      case XM_SCANCODE_PAGEUP:
        code = XM_SCANCODE_KP_9;
        break;
      case XM_SCANCODE_LEFT:
        code = XM_SCANCODE_KP_4;
        break;
      case XM_SCANCODE_RIGHT:
        code = XM_SCANCODE_KP_6;
        break;
      case XM_SCANCODE_END:
        code = XM_SCANCODE_KP_1;
        break;
      case XM_SCANCODE_DOWN:
        code = XM_SCANCODE_KP_2;
        break;
      case XM_SCANCODE_PAGEDOWN:
        code = XM_SCANCODE_KP_3;
        break;
      case XM_SCANCODE_INSERT:
        code = XM_SCANCODE_KP_0;
        break;
      case XM_SCANCODE_DELETE:
        code = XM_SCANCODE_KP_PERIOD;
        break;
      case XM_SCANCODE_PRINTSCREEN:
        code = XM_SCANCODE_KP_MULTIPLY;
        break;
      default:
        break;
      }
    }
  }
  return code;
}

static int
WIN_ConvertUTF32toUTF8(UINT32 codepoint, char * text)
{
  if (codepoint <= 0x7F) {
    text[0] = (char)codepoint;
    text[1] = '\0';
  }
  else if (codepoint <= 0x7FF) {
    text[0] = 0xC0 | (char)((codepoint >> 6) & 0x1F);
    text[1] = 0x80 | (char)(codepoint & 0x3F);
    text[2] = '\0';
  }
  else if (codepoint <= 0xFFFF) {
    text[0] = 0xE0 | (char)((codepoint >> 12) & 0x0F);
    text[1] = 0x80 | (char)((codepoint >> 6) & 0x3F);
    text[2] = 0x80 | (char)(codepoint & 0x3F);
    text[3] = '\0';
  }
  else if (codepoint <= 0x10FFFF) {
    text[0] = 0xF0 | (char)((codepoint >> 18) & 0x0F);
    text[1] = 0x80 | (char)((codepoint >> 12) & 0x3F);
    text[2] = 0x80 | (char)((codepoint >> 6) & 0x3F);
    text[3] = 0x80 | (char)(codepoint & 0x3F);
    text[4] = '\0';
  }
  else {
    return 0;
  }
  return 1;
}

static unsigned int next_move_ticket = 0;
LRESULT CALLBACK WinHost_MessagesHandler(HWND hWnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  char pc_hw_sync_fifo[32 + 1];
  unsigned int curr_ticket;
  curr_ticket = GetTickCount();
  switch (uMsg) {

  case WM_SYSCOMMAND:
    // disable screensaver and monitor powersave
    switch (wParam) {
    case SC_SCREENSAVE:
    case SC_MONITORPOWER:
      return 0;
    }
    break;

  case WM_CLOSE:
    PostQuitMessage(0);
    return 0;

  case WM_KEYDOWN:
  {
    if (wParam == VK_PROCESSKEY) {
      break;
    }
    XM_Scancode code = WindowsScanCodeToXMScanCode(lParam, wParam);
    if (code != XM_SCANCODE_UNKNOWN) {
      XM_KeyEventProc(XM_EVENT_KEYDOWN, code, (unsigned char)((lParam >> 30) & 0x01), (lParam & 0xffff) ? 1 : 0, curr_ticket);
    }
    return 0;
  }

  case WM_KEYUP:
  {
    XM_Scancode code = WindowsScanCodeToXMScanCode(lParam, wParam);
    if (code != XM_SCANCODE_UNKNOWN) {
      XM_KeyEventProc(XM_EVENT_KEYUP, code, (unsigned char)((lParam >> 30) & 0x01), (lParam & 0xffff) ? 1 : 0, curr_ticket);
    }
    return 0;

  }

  case WM_CHAR:
  {
    char text[5];
    if (WIN_ConvertUTF32toUTF8((UINT32)wParam, text)) {
      XM_TextInputEventProc(text, curr_ticket);
    }
  }
  break;


  case WM_GETMINMAXINFO:
    return 0;

  case WM_SIZE:
    return 0;

  case WM_LBUTTONDOWN:
    sprintf(pc_hw_sync_fifo, "##<TD %d %d %d>\r\n", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), curr_ticket);
    pc_hw_sync_write(pc_hw_sync_fifo, strlen(pc_hw_sync_fifo));
    XM_TpEventProc(XM_EVENT_TOUCHDOWN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), curr_ticket);
    next_move_ticket = GetTickCount() + 35;
    return 0;

  case WM_LBUTTONUP:
    sprintf(pc_hw_sync_fifo, "##<TU %d %d %d>\r\n", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), curr_ticket);
    pc_hw_sync_write(pc_hw_sync_fifo, strlen(pc_hw_sync_fifo));
    XM_TpEventProc(XM_EVENT_TOUCHUP, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), curr_ticket);
    next_move_ticket = GetTickCount() + 35;
    return 0;


  case WM_MOUSEMOVE:
    // 防止产生过多的MOVE事件
    if (curr_ticket >= next_move_ticket)
    {
      sprintf(pc_hw_sync_fifo, "##<TM %d %d %d>\r\n", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), curr_ticket);
      pc_hw_sync_write(pc_hw_sync_fifo, strlen(pc_hw_sync_fifo));
      XM_TpEventProc(XM_EVENT_TOUCHMOVE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), curr_ticket);
      next_move_ticket = GetTickCount() + 35;
    }
    return 0;
  }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


typedef enum MONITOR_DPI_TYPE {
  MDT_EFFECTIVE_DPI = 0,
  MDT_ANGULAR_DPI = 1,
  MDT_RAW_DPI = 2,
  MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;


typedef int (WINAPI *MySetProcessDpiAwareness)(int);
typedef HRESULT(WINAPI *MyGetDpiForMonitor)(HMONITOR, int, int*, int*);
static float WIN_GetWindowDpiRatio(void)
{
  float fPixel_ratio = 1.0f;
  int x = 0;
  int y = 0;
  const float nNormalPix = 96.0f;
  if (0)
  {
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
      x = GetDeviceCaps(hdc, LOGPIXELSX);//每英寸逻辑像素数 水平
      y = GetDeviceCaps(hdc, LOGPIXELSY);//每英寸逻辑像素数 垂直    
      ReleaseDC(NULL, hdc);
    }
  }
  else
  {
    HMODULE hModule = LoadLibraryA("shcore.dll");
    if (hModule)
    {
      MyGetDpiForMonitor  myGetDpi = (MyGetDpiForMonitor)GetProcAddress(hModule, "GetDpiForMonitor");
      MySetProcessDpiAwareness setDpiAwareness = (MySetProcessDpiAwareness)GetProcAddress(hModule, "SetProcessDpiAwareness");
      setDpiAwareness(2);
      HMONITOR hMonitor;
      POINT    pt;
      HRESULT  hr = E_FAIL;
      pt.x = 1;
      pt.y = 1;
      hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
      hr = myGetDpi(hMonitor, MDT_EFFECTIVE_DPI, &x, &y);
    }
    FreeLibrary(hModule);
  }
  fPixel_ratio = x / nNormalPix;
  return fPixel_ratio;
}

float XM_GetWindowDpiRatio(void)
{
  dpi_ratio = 1.0f;
  {
    dpi_ratio = WIN_GetWindowDpiRatio();
    dpi_ratio = dpi_ratio > 1.0f ? dpi_ratio : 1.0f;
  }
  return dpi_ratio;
}


static void windowDestroy(void)
{
  // release device context
  ReleaseDC(nativeWindow, deviceContext);
  // destroy window
  DestroyWindow(nativeWindow);
  // unregister class
  UnregisterClass(CLASS_NAME, applicationInstance);
}

void XM_WinHost_WindowDestroy(void)
{
  DeleteCriticalSection(&event_critical_section);
  CloseHandle(event_handle);
  windowDestroy();
}

static HWND WindowCreate(const char* title,
  const VGuint width,
  const VGuint height,
  const DWORD dwExStyle,
  const DWORD dwStyle)
{

  WNDCLASS wc;
  RECT rect;
  VGint x, y, w, h;

  // register window class
  wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
  wc.lpfnWndProc = WinHost_MessagesHandler;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;

  applicationInstance = GetModuleHandle(NULL);
  wc.hInstance = applicationInstance;
  wc.hIcon = NULL;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = CLASS_NAME;
  RegisterClass(&wc);

  // calculate window size
  rect.left = 0;
  rect.top = 0;
  rect.right = width;
  rect.bottom = height;
  AdjustWindowRect(&rect, dwStyle, 0);
  w = rect.right - rect.left;
  h = rect.bottom - rect.top;

#if  _DEBUG
  x = 0;
  y = 0;
#else
  // center window on the screen
  x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
  y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
#endif

  // create window
  return CreateWindowEx(dwExStyle, CLASS_NAME, title, dwStyle, x, y, w, h, NULL, NULL, applicationInstance, NULL);
}

void * XM_WinHost_WindowCreate(const char* title,
  const unsigned int width,
  const unsigned int height)
{

  // create window
  nativeWindow = WindowCreate(title, width, height, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, WS_TILEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
  if (!nativeWindow) {
    return 0;
  }

  init_pc_hw_sync_comm();

  // 禁用HDPI模式
  SetProcessDPIAware();

  // get window dc
  deviceContext = GetDC(nativeWindow);
  if (!deviceContext) {
    return 0;
  }
  VGuint i;
  for (i = 0; i < sizeof(BITMAPINFOHEADER) + 16; ++i)
  {
    vgSurfaceBitmapBuffer[i] = 0;
  }
  // create bitmap header
  vgSurfaceBitmapInfo = (BITMAPINFO *)&vgSurfaceBitmapBuffer;
  vgSurfaceBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  vgSurfaceBitmapInfo->bmiHeader.biPlanes = 1;
  vgSurfaceBitmapInfo->bmiHeader.biCompression = BI_BITFIELDS;


  // show window
  ShowWindow(nativeWindow, SW_NORMAL);
  SetForegroundWindow(nativeWindow);
  SetFocus(nativeWindow);

  event_handle = CreateSemaphore(NULL, 0, 0x7FFFFFFF, "EVENT_HANDLE");
  InitializeCriticalSection(&event_critical_section);
  eventBPos = eventEPos = 0;

  init_pc_autotest();

  return nativeWindow;
}

#include <stdio.h>
//BITMAPINFO vgSurfaceBitmapBuffer[16];
//BITMAPINFO* vgSurfaceBitmapInfo;
void XM_WinHost_WindowBuffersSwap(char *surface_pixels, unsigned int width, unsigned int height, unsigned int bpp)
{
  if (bpp == 32)
  {
    // ARGB channel ordering support
    //vgSurfaceBitmapInfo = vgSurfaceBitmapBuffer;
    //vgSurfaceBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    //vgSurfaceBitmapInfo->bmiHeader.biPlanes = 1;
    //vgSurfaceBitmapInfo->bmiHeader.biCompression = BI_BITFIELDS;
    vgSurfaceBitmapInfo->bmiHeader.biBitCount = 32;
    ((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[0] = 0x00FF0000;
    ((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[1] = 0x0000FF00;
    ((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[2] = 0x000000FF;
    //((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[0] = 0x000000FF;
    //((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[1] = 0x0000FF00;
    //((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[2] = 0x00FF0000;
    //((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[0] = 0x0000FF00;
    //((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[1] = 0x00FF0000;
    //((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[2] = 0xFF000000;
    //((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[0] = 0xFF000000;
    //((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[1] = 0x00FF0000;
    //((unsigned long *)vgSurfaceBitmapInfo->bmiColors)[2] = 0x0000FF00;
    vgSurfaceBitmapInfo->bmiHeader.biWidth = width;
    vgSurfaceBitmapInfo->bmiHeader.biHeight = height;
    SetDIBitsToDevice(deviceContext, 0, 0, width, height, 0, 0, 0, height, surface_pixels, vgSurfaceBitmapInfo, DIB_RGB_COLORS);

    //FILE *fp = fopen("\\1.bin", "wb");
    //fwrite(surface_pixels, 1, width * height * 4, fp);
    //fclose(fp);
  }

  //TextOut(deviceContext, 0, 0, "HELLO", 5);
}

void XM_GetWidowSize(int *w, int *h)
{
  RECT rect;
  GetClientRect(nativeWindow, &rect);
  *w = rect.right - rect.left;
  *h = rect.bottom - rect.top;
}

void XM_GetDrawableSize(int *w, int *h)
{

}

void dma_flush_range(unsigned int base, unsigned int last)
{
}
void dma_clean_range(unsigned int base, unsigned int last)
{
}

void dma_inv_range(unsigned int base, unsigned int last)
{
}

#ifdef NINE
HANDLE hAutotestThread;
#include <io.h>
#include <fcntl.h>
extern int nine_at_define(unsigned  char *script_data);

static int run_at_script(const char *runPath)
{
	int fd;
	size_t file_size = 0;
	unsigned char *script = NULL;
	do {
		fd = _open(runPath, _O_BINARY);
		if (fd < 0)
			return -1;
		file_size = _filelength(fd);
		script = (char *)calloc(file_size + 1, 1);
		if (script) {
			_read(fd, script, file_size);
		}
		_close(fd);
		if (script) {
			printf("run AT script file(%s) ...\n", runPath);
			nine_at_define((unsigned char *)script);
			printf("run AT script file(%s) finish\n", runPath);
			free(script);
		}

	} while (0);
	return 0;
}

static DWORD WINAPI pc_autotest_task(
  LPVOID lpParameter   // thread data
)
{
  char  runPath[MAX_PATH];
  memset(runPath, 0, sizeof(runPath));
  GetModuleFileName(NULL, runPath, MAX_PATH);
  char *ch = strrchr(runPath, '\\');
  *(ch + 1) = 0;
  strcat(ch, "autotest.at");

  run_at_script(runPath);

  return 0;

}
#endif

static void init_pc_autotest(void)
{
#ifdef NINE
  hAutotestThread = CreateThread(NULL, 0x10000, pc_autotest_task, NULL, 0, NULL);
#endif
}



#endif /* XM_WINDOWS_HOST */


