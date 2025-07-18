
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

typedef void (* bt_at_callback)(char * cAtStr);

int console_init(bt_at_callback cb);
void console_register_cb(void *ctx, bt_at_callback cb);
int console_send_atcmd(char * cAtCmd, unsigned short length);
int console_deinit(void);

#endif

