#ifndef _WDT_H
#define _WDT_H

#ifdef __cplusplus
extern "C" {
#endif

int wdt_set_heartbeat(unsigned int timeout);
void wdt_stop(void);
void wdt_start(void);
void ark_wdt_keepalive(void);
int wdt_init(void);
void wdt_cpu_reboot(void);


#ifdef __cplusplus
}
#endif

#endif
