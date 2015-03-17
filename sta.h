#ifndef __STA_H__
#define __STA_H__

int sta_init(void);
int handle_sta(void);
int sta_open(const char *ssid, const char *passwd);
static int sta_close(void);
int sta_state_machine(void);
int sta_connect_to_ap(char *ssid, char *passwd);

int sta_change_status(int is_up, net_para_st *np);

int sta_is_up(void);
char *sta_get_ip(void);

#endif
