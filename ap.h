#ifndef __AP_H__
#define __AP_H__

int ap_init(void);

int ap_open(void);
int ap_close(void);

int ap_state_machine(void);

char *ap_get_ip(void);
int ap_is_up(void);
void ap_change_status(int is_up, net_para_st *np);

#endif
