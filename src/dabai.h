#ifndef __DABAI_H_
#define __DABAI_H_

#include <stdio.h>      /*標準輸入輸出定義*/
#include <stdlib.h>     /*標準函數庫定義*/
#include <unistd.h>     /*Unix 標準函數定義*/
#include <errno.h>      /*錯誤號定義*/
#include <memory.h>
#include <time.h>

#define BM78SPP05MC2 1
//#define BM78SPP05NC2 1			// define Bluetooth module

#define DEV_UART "/dev/ttyS0"
#define BAUDRATE 57600
#define DATABIT 8
#define PARITY 'N'
#define STOPBIT 1
#define DABAI_PASSWD "lingshi508"
#define DEFENCRYPTION "psk"
#define IOSSUFFIX "AAEnd"
//#define FILETOTTYBYTE 4096
//#define SENDTOCLIENTFILE "/DaBai/ToClient.json"
#define RW_BYTETIME ((10/(57600/10000))+1)*100
#define CHGDEVMAX 4		// Maxmum Charge Device...
#define BTCONFPATH "/DaBai/btconf"
#define SENDTOPHONEPATH "/DaBai/7688.png"
#define UARTRXSIZE 1024
#define UARTTXSIZE 1024
#define FILESIZE 4096
#define BTMIDULEPATH "/DaBai/HostDeviceInfo.json"
#define CHGLISTPATH "/DaBai/OnlineChgList.json"

#define DIRECT_OUT 1
#define DIRECT_IN 0

#define GPIOCTRL_0 0x10000600
#define GPIOCTRL_1 0x10000604
#define GPIOCTRL_2 0x10000608

#define GPIODATA_0 0x10000620
#define GPIODATA_1 0x10000624
#define GPIODATA_2 0x10000628

#define GPIO_USB1PWR_NUM 4
#define GPIO_USB2PWR_NUM 5
#define GPIO_LANLINK_NUM 0
#define GPIO_LANACTIVE_NUM 15
#define GPIO_WCEN_NUM 18
#define GPIO_WCRDY_NUM 19
#define GPIO_SWBTN_NUM 14
#define GPIO_WAKEUP_NUM 2
#define GPIO_RESET_NUM 3
#define GPIO_P04_NUM 45
#define GPIO_P15_NUM 1
#define GPIO_P20_NUM 46
#define GPIO_P24_NUM 16
#define GPIO_EAN_NUM 17

enum
{
    normal_mode=0,
    WEE_mode,
};

enum {
	ERR_GPIO_MODE = 99,
	ERR_GPIO_VAL = 98,
	ERR_GPIO_RANGE = 97,
	OK_GPIO = 0,
};

struct ClientDev
{
    unsigned int DevIdx;
    char DevType[16];
    char DevMac[18];
    char DevAccount[64];
    char DevUserId[64];
		time_t StartTime;
		time_t CurrentTime;
		char DevFormatSTime[64];
		char DevFormatCTime[64];
    unsigned char ChgMode;    // 0:Not charge , 1:Wireless , 2:USB1 , 3:USB2
    unsigned int RemainT_Hr;
    unsigned int RemainT_Min;
};

enum
{
    //Err_none=0,
    Err_Profile=-1,
    Err_Index,
    Err_Ssid,
    Err_Password,
    Err_Encrytpion,
    Err_Dbstoreid,
    Err_Type,
    Err_Userid,
    Err_Account,
    Err_Mac,
};

struct ClientDev *ChargeDevice[CHGDEVMAX];
char StaSsid[128];
char StaPassword[64];
char StaEncryption[16];
char DBStoreId[128];
int CmdIndex;
int TypeIdx;
unsigned char ChargeDeviceCount;
unsigned char GetBTConfigFlag;
unsigned char BTIntoConfigMode;
unsigned char ChangBTModuleNameFlag;

// define at gpio.c
unsigned GetGpioVal(int gpio_num);
int SetGpioVal(int gpio_num, int val);
void GpioPinMode(void);

// define at uart.c
int uart_initial(char *dev, int baudrate, int bits, int parity, int stopbits);
int uart_read(int fd, unsigned char *buf);
int uart_write(int fd, unsigned char *buf, unsigned long length);

// define at network.c
int GetDeviceMACAddr(unsigned char *path, unsigned char *filebuf);

// define at bluetooth.c
int PinSetForBMModule(void);
int SetBMModuleMode(int OPmode);
int GetBTModuleInof(unsigned char *path, unsigned char *rxbuf, unsigned char *filebuf);
int GetBTModuleName(unsigned char *path, unsigned char *rxbuf, unsigned char *filebuf);
int BTModuleLeaveConfigMode(unsigned char *rxbuf);
int BTTransferUart(int fd, unsigned char *path, unsigned char *rxbuf);

// define at subporc.c
void send_command(unsigned char *command, unsigned char *resulte, int resulte_length);
int CheckCHGDevInfo(unsigned char *path, struct ClientDev *CDV, unsigned char *ChgDevCt, unsigned char *filebuf);

#endif /* !__DABAI_H_ */
