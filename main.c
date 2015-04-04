#include "main.h"
#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "sys_control.h"
#include "sta.h"
#include "ap.h"

#define PRODUCT_INFO  "Wifi Infrared Controller V0.1"

#define MODULE "[callback] "

static const char *secure_type[8] = {
	"Open system",
	"WEP",
	"WPA TKIP",
	"WPA AES",
	"WPA2 TKIP",
	"WPA2 AES",
	"WPA2 MIXED",
	"Auto"
};

#ifdef CONFIG_LOCAL_IR
extern void ir_test_menu(void);
#endif

/**
 *  Call backs
 * */
void system_version(char *str, int len)
{
	snprintf(str, len, "%s", PRODUCT_INFO);
}

void userWatchDog(void)
{
}

void ApListCallback(ScanResult *pApList)
{
	int i;

	printf(MODULE "Find %d APs: \r\n", pApList->ApNum);

	for (i=0;i<pApList->ApNum;i++) {
		printf(MODULE "    SSID: %s, Signal: %d%%\r\n",
				pApList->ApList[i].ssid, pApList->ApList[i].ApPower);
	}
}

void NetCallback(net_para_st *pnet)
{
	printf(MODULE "Station mode: IP address: %s \r\n", pnet->ip);
	printf(MODULE "Station mode: NetMask address: %s \r\n", pnet->mask);
	printf(MODULE "Station mode: Gateway address: %s \r\n", pnet->gate);
	printf(MODULE "Station mode: DNS server address: %s \r\n", pnet->dns);
	printf(MODULE "Station mode: MAC address: %s \r\n", pnet->mac);
}

/*
 * Callback, return connected AP info
 * */
void connected_ap_info(apinfo_adv_t *ap_info, char *key, int key_len)
{
	/*Update fastlink record*/
	char *key_str;

	key_str = calloc(100, 1);
	if (!key_str) {
		printf(MODULE "Fail to alloc space for key_str\r\n");
		return;
	}
	memcpy(key_str, key, key_len + 1);

	printf(MODULE "Connected to SSID: <%s>\r\n", ap_info->ssid);
	printf(MODULE "Channel: %d\r\n", ap_info->channel);
	printf(MODULE "Security: %s\r\n", secure_type[ap_info->security]);
	printf(MODULE "Key or PMK: %s\r\n", key_str);

	free(key_str);
}

void RptConfigmodeRslt(network_InitTypeDef_st *nwkpara)
{
	if(nwkpara == NULL)
		printf(MODULE "Configuration failed\r\n");
	else
		printf(MODULE "Configuration is successful, SSID:%s, Key:%s\r\n",
			nwkpara->wifi_ssid,
			nwkpara->wifi_key);
}

void Button_irq_handler(void *arg)
{
	printf(MODULE "Start scanning by user...\r\n");
	mxchipStartScan();
}

/*
 * StartNetwork() callback
 * */
void WifiStatusHandler(int event)
{
	net_para_st para;

	switch (event) {
	case MXCHIP_WIFI_UP:
		getNetPara(&para, Station);
		sta_change_status(1, &para);
		break;

	case MXCHIP_WIFI_DOWN:
		sta_change_status(0, NULL);
		break;

	case MXCHIP_UAP_UP:
		getNetPara(&para, Soft_AP);
		ap_change_status(1, &para);
		break;

	case MXCHIP_UAP_DOWN:
		ap_change_status(0, NULL);
		break;

	default:
		break;
	}
}

static void init_lib(void)
{
	lib_config_t config;

	config.tcp_buf_dynamic = mxEnable;
	config.tcp_max_connection_num = 12;
	config.tcp_rx_size = RX_BUF_SIZE;
	config.tcp_tx_size = TX_BUF_SIZE;
	config.hw_watchdog = 0;
	config.wifi_channel = WIFI_CHANNEL_1_13;

	lib_config(&config);
}

int main(void)
{
	init_lib();
	mxchipInit();

	UART_Init();

#ifdef CONFIG_LOCAL_IR
        ir_test_menu();
#endif
	Button_Init();

	printf("\r\n");
	printf("============================================\r\n");
	printf("  %s\r\n", PRODUCT_INFO);
	printf("  mxchipWNet library version: %s\r\n", system_lib_version());
	printf("============================================\r\n");

	init_sys();

	while(1){
		mxchipTick();

		sys_control();
	}
}
