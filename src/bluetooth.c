#include "dabai.h"

#define DBG_EN 1

int PinSetForBMModule(void)
{
    int ret;

	ret = GpioIOInitial(GPIO_SWBTN_NUM, DIRECT_OUT, 0);
	if(ret) {
		printf("Gpio SWBTN initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_WAKEUP_NUM, DIRECT_OUT, 0);
	if(ret) {
		printf("Gpio WAKEUP initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_RESET_NUM, DIRECT_OUT, 0);
	if(ret) {
		printf("Gpio RESET initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_P20_NUM, DIRECT_OUT, 0);
	if(ret) {
		printf("Gpio P20 initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_P24_NUM, DIRECT_OUT, 0);
	if(ret) {
		printf("Gpio P24 initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_EAN_NUM, DIRECT_OUT, 0);
	if(ret) {
		printf("Gpio EAN initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_P04_NUM, DIRECT_IN, 0);
	if(ret) {
		printf("Gpio P04 initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_P15_NUM, DIRECT_IN, 0);
	if(ret) {
		  printf("Gpio P15 initial faild!!! Error Code : %d\n", ret);
	}
}

int SetBMModuleMode(int OPmode)
{
    usleep(1000);
    SetGpioVal(GPIO_WAKEUP_NUM, 1);
    switch(OPmode)
    {
        case normal_mode:
        default:
            #ifdef BM78SPP05MC2
              SetGpioVal(GPIO_EAN_NUM, 0);    // for BM78SPPx5MC2
            #endif
            #ifdef BM78SPP05NC2
              SetGpioVal(GPIO_EAN_NUM, 1);    // for BM78SPPx5NC2
            #endif
            SetGpioVal(GPIO_P20_NUM, 1);
            SetGpioVal(GPIO_P24_NUM, 1);
            break;

        case WEE_mode:
            #ifdef BM78SPP05MC2
              SetGpioVal(GPIO_EAN_NUM, 0);    // for BM78SPPx5MC2
            #endif
            #ifdef BM78SPP05NC2
              SetGpioVal(GPIO_EAN_NUM, 1);    // for BM78SPPx5NC2
            #endif
            SetGpioVal(GPIO_P20_NUM, 0);
            SetGpioVal(GPIO_P24_NUM, 1);
            break;
    }
    SetGpioVal(GPIO_SWBTN_NUM, 1);
    usleep(40000);
    SetGpioVal(GPIO_RESET_NUM, 1);
    usleep(450000);
}

unsigned char CommandSetCheckSum(unsigned char *CommandBuf)
{
    unsigned int len, i;
    int CST = 0;
    unsigned char checksum;

    len = CommandBuf[2] + 2;
    for(i=0; i < len; i++)
    {
        CST -= CommandBuf[i+1];
    }
    checksum = CST & 0x00FF;
    return (checksum);
}

void BTModuleReset(void)
{
  SetGpioVal(GPIO_RESET_NUM, 0);
  usleep(150000);
  SetGpioVal(GPIO_RESET_NUM, 1);
  sleep(3);
}

int ReadEEpromCommand(unsigned char *commptr, unsigned int addr, unsigned char byte_num)
{
  *(commptr + 0) = 0x01;
  *(commptr + 1) = 0x29;
  *(commptr + 2) = 0xFC;
  *(commptr + 3) = 0x03;
  *(commptr + 4) = (unsigned char)(addr >> 8) & 0xFF;
  *(commptr + 5) = (unsigned char)addr & 0xFF;
  *(commptr + 6) = byte_num;
  return strlen(commptr);
}

int WriteEEpromCommand(unsigned char *commptr, unsigned int addr, unsigned char byte_num, unsigned char *writeptr)
{
  unsigned char i;

  *(commptr + 0) = 0x01;
  *(commptr + 1) = 0x27;
  *(commptr + 2) = 0xFC;
  *(commptr + 3) = byte_num + 3;
  *(commptr + 4) = (unsigned char)(addr >> 8) & 0xFF;
  *(commptr + 5) = (unsigned char)addr & 0xFF;
  *(commptr + 6) = byte_num;
  for(i=0;i<byte_num;i++)
  {
    *(commptr + i + 7) = *(writeptr + i);
  }
  return strlen(commptr);
}

int GetBTModuleInof(unsigned char *path, unsigned char *rxbuf, unsigned char *filebuf)
{
    int recct = 0, fd;
    unsigned char BTMBuf[5] = {0xAA, 0x00, 0x01, 0x01};
    unsigned char Strbuf[32] = "  \"Bluetooth MAC Address\"";
    unsigned int offset = 0;
    FILE *fp;
    struct json_object *json_file;
    unsigned char *BTAddrStr;

    memset(rxbuf, 0, UARTRXSIZE);
    memset(filebuf, 0, FILESIZE);
    fd = uart_initial(DEV_UART, BAUDRATE,  DATABIT, PARITY, STOPBIT);
    if(fd < 0)
        return -1;
    BTMBuf[4] = CommandSetCheckSum(BTMBuf);
    #if DBG_EN
        printf("Get BT Module Information Command Checksum : 0x%x\n", BTMBuf[4]);
    #endif
    uart_write(fd, BTMBuf, sizeof(BTMBuf));
    usleep(10000);
    recct = uart_read(fd, rxbuf);
    if(recct > 0)
    {
        #if DBG_EN
            int i;
            for(i=0;i<recct;i++)
              printf("rxbuf[%d](Hex) : %02x\n", i, rxbuf[i]);
            printf("GetBTModuleInof is receive status, count is %d\n", recct);
        #endif
        json_file = json_object_from_file(path);
        printf("josn file is %s\n", json_object_to_json_string(json_file));
        BTAddrStr = (unsigned char *)calloc(128, sizeof(unsigned char));

        sprintf(BTAddrStr, "%02x:%02x:%02x:%02x:%02x:%02x", rxbuf[recct - 2], rxbuf[recct - 3], rxbuf[recct - 4], rxbuf[recct - 5], rxbuf[recct - 6], (rxbuf[recct - 8] << 4) | rxbuf[recct - 7]);
        json_object_object_del(json_file, "Bluetooth MAC Address");
        json_object_object_add(json_file, "Bluetooth MAC Address", json_object_new_string(BTAddrStr));
        json_object_to_file(path, json_file);
        close(fd);
        usleep(10000);
        return 1;
    }
    close(fd);
    usleep(10000);
    return 0;
}

int GetBTModuleName(unsigned char *path, unsigned char *rxbuf, unsigned char *filebuf)
{
    int recct = 0, fd;
    unsigned char BTMBuf[5] = {0xAA, 0x00, 0x01, 0x07};
    unsigned char Strbuf[32] = "  \"Bluetooth Device Name\"";
    unsigned int offset = 0;
    unsigned char checksum;
    FILE *fp;
    struct json_object *json_file;
    unsigned char *BTNameStr;

restart:
    memset(rxbuf, 0, UARTRXSIZE);
    memset(filebuf, 0, FILESIZE);
    fd = uart_initial(DEV_UART, BAUDRATE,  DATABIT, PARITY, STOPBIT);
    if(fd < 0)
        return -1;
    BTMBuf[4] = CommandSetCheckSum(BTMBuf);
    #if DBG_EN
        printf("Get BT Module Name Command Checksum : 0x%x\n", BTMBuf[4]);
    #endif
    uart_write(fd, BTMBuf, sizeof(BTMBuf));
    usleep(10000);
    recct = uart_read(fd, rxbuf);
    if(recct > 0)
    {
        checksum = CommandSetCheckSum(rxbuf);
        #if DBG_EN
            int i;
            for(i=0;i<recct;i++)
              printf("rxbuf[%d](Hex) : %02x\n", i, rxbuf[i]);
            printf("GetBTModuleName is receive status, count is %d, checksum is %x\n", recct, checksum);
        #endif
        if(checksum != rxbuf[recct - 1])
        {
            printf("GetBTModuleName Receive data check sum error~~!!!\n");
            goto restart;
        }
        rxbuf[recct - 1] = '\0';
        json_file = json_object_from_file(path);
        printf("josn file is %s\n", json_object_to_json_string(json_file));
        BTNameStr = (unsigned char *)calloc(128, sizeof(unsigned char));

        sprintf(BTNameStr, "%s", rxbuf+6);
        json_object_object_del(json_file, "Bluetooth Device Name");
        json_object_object_add(json_file, "Bluetooth Device Name", json_object_new_string(BTNameStr));
        json_object_to_file(path, json_file);
        close(fd);
        usleep(10000);
        return 1;
    }
    close(fd);
    usleep(10000);
    return 0;
}
/*
int SetBTModuleName(char *path)
{
    int recct = 0, fd;
    unsigned char BTMBuf[] =
}
*/
int BTModuleLeaveConfigMode(unsigned char *rxbuf)
{
    int recct = 0, fd;
    unsigned char BTMBuf[6] = {0xAA, 0x00, 0x02, 0x52, 0x00};
    unsigned char checksum;

restart:
    memset(rxbuf, 0, UARTRXSIZE);
    fd = uart_initial(DEV_UART, BAUDRATE,  DATABIT, PARITY, STOPBIT);
    if(fd < 0)
        return -1;
    BTMBuf[5] = CommandSetCheckSum(BTMBuf);
    #if DBG_EN
        printf("Get BT Module Leave Config Mode Command Checksum : 0x%x\n", BTMBuf[5]);
    #endif
    uart_write(fd, BTMBuf, sizeof(BTMBuf));
    usleep(10000);
    recct = uart_read(fd, rxbuf);
    if(recct > 0)
    {
        checksum = CommandSetCheckSum(rxbuf);
        #if DBG_EN
            int i;
            for(i=0;i<recct;i++)
              printf("rxbuf[%d](Hex) : %02x\n", i, rxbuf[i]);
            printf("GetBTModule Leave Config Mode is receive status, count is %d, checksum is %x\n", recct, checksum);
        #endif
        if(checksum != rxbuf[recct - 1])
        {
            printf("GetBTModule Leave Config Mode receive data error~~!!!\n");
            goto restart;
        }
        if((rxbuf[0] == 0xAA) && (rxbuf[3] == 0x8F) && (rxbuf[4] == 0x00))
        {
            printf("BT Module Leave Config Mode !!!\n");
        }
        close(fd);
        usleep(10000);
        return 1;
    }
    close(fd);
    usleep(10000);
    return 0;
}

int BTModuleChgDevName(unsigned char *rxbuf, unsigned char *Name)
{
    unsigned char BTMBuf[36];
    unsigned short length, i;
    int recct, fd;
    unsigned char checksum;

restart:
    memset(rxbuf, 0, UARTRXSIZE);
    memset(BTMBuf, 0, 36);
    fd = uart_initial(DEV_UART, BAUDRATE,  DATABIT, PARITY, STOPBIT);
    if(fd < 0)
        return -1;
    length = strlen(Name);
    if(length > 30)
      length = 30;
    #if DBG_EN
        printf("length is %d\n", length);
    #endif
    BTMBuf[0] = 0xAA;
    BTMBuf[1] = ((length + 2) >> 8);
    BTMBuf[2] = (length + 2) & 0xFF;
    BTMBuf[3] = 0x08;
    BTMBuf[4] = 0x01;       // 0: not write into eeprom / 1: write into eeprom
    for(i=0;i<length;i++)
    {
        BTMBuf[i+5] = Name[i];
    }
    BTMBuf[length+5] = CommandSetCheckSum(BTMBuf);
    #if DBG_EN
        printf("Name is %s\n", Name);
        for(i=0;i<(length+6);i++)
            printf("BTMBuf[%d] is %x\n", i, BTMBuf[i]);
    #endif
    uart_write(fd, BTMBuf, length+6);
    usleep(100000);
    recct = uart_read(fd, rxbuf);
    if(recct > 0)
    {
        checksum = CommandSetCheckSum(rxbuf);
        #if DBG_EN
            for(i=0;i<recct;i++)
              printf("rxbuf is : %x\n", rxbuf[i]);
            printf("BTModuleChgDevName is receive status, count is %d, checksum is %x\n", recct);
        #endif
        if(checksum != rxbuf[recct - 1])
        {
            printf("BTModuleChgDevName recive data error~~!!!\n");
            goto restart;
        }
        if((rxbuf[0] == 0xAA) && (rxbuf[3] == 0x80) && (rxbuf[4] == 0x08))
        {
            #if DBG_EN
                printf("Bluetooth Device Name Change success~~!!!\n");
            #endif
            usleep(500000);
            SetGpioVal(GPIO_RESET_NUM, 0);
            usleep(1000);
            SetGpioVal(GPIO_RESET_NUM, 1);
            memset(rxbuf, 0, strlen(rxbuf));
            while(1)
            {
                recct = uart_read(fd, rxbuf);
                if(recct > 0)
                {
                    if((rxbuf[0] == 0xAA) && (rxbuf[3] == 0x8F) && (rxbuf[4] == 0x01))
                    {
                        #if DBG_EN
                            printf("Bluetooth Device Restart~~!!!\n");
                        #endif
                        memset(rxbuf, 0, strlen(rxbuf));
                        break;
                    }
                }
            }
        }
    }
    close(fd);
}

int BTTransferUart(int fd, unsigned char *path, unsigned char *rxbuf, unsigned char *filebuf)
{
    int recct = 0, idx = 0, i = 0;;
    unsigned char *checkstr;
    FILE *fp_uart, *fpimg;
    unsigned char *imgbuf;
    struct json_object *jobj;

    memset(rxbuf, 0, UARTRXSIZE);
    memset(filebuf, 0, FILESIZE);
    checkstr = (unsigned char *)calloc(16, sizeof(unsigned char));
    recct = uart_read(fd, rxbuf);
    if(recct > 0)
    {
        #if DBG_EN
            printf("rxbuf is : %s\n", rxbuf);
            printf("BTTransferUart is receive status, count is %d\n", recct);
        #endif
        jobj = json_tokener_parse(rxbuf);
        if(is_error(jobj))
        {
          fp_uart = fopen("/DaBai/ErrorCmd.txt", "w");
          if(fp_uart != NULL)
          {
            fwrite(rxbuf, 1, recct, fp_uart);
            fclose(fp_uart);
          }
          #if DBG_EN
              printf("rxbuf format is error, not mach json file format\n");
          #endif
          return -1;
        }
        printf("rxbuf to jobj string %s\n", json_object_to_json_string(jobj));
        idx = json_object_get_int(json_object_object_get(jobj, "index"));
        #if DBG_EN
            printf("json-c idx : %d\n", idx);
        #endif
        if(idx == 0 || idx == NULL)
        {
          #if DBG_EN
              printf("json-c idx : %d\n", idx);
          #endif
          return -1;
        }
        fp_uart = fopen(path, "w");
        if(fp_uart != NULL)
        {
            #if DBG_EN
                printf("write path %s\n", path);
            #endif
            fwrite(json_object_to_json_string(jobj), 1, strlen(json_object_to_json_string(jobj)), fp_uart);
            //fwrite("\n\0", 1, 1, fp_uart);
            fclose(fp_uart);
        }
        json_object_put(jobj);      // if new json object, After used must be free json object
        if(idx == 2)
        {
          fp_uart = fopen(path, "r");
          if(fp_uart != NULL)
          {
              #if DBG_EN
                  printf("Open RxCommTmp file\n");
              #endif
              while(!feof(fp_uart))
              {
                  fread(filebuf, 1, FILESIZE, fp_uart);
                  jobj = json_tokener_parse(filebuf);
                  printf("filebuf to jobj string %s\n", json_object_to_json_string(jobj));
                  checkstr = json_object_get_string(json_object_object_get(jobj, "type"));
                  imgbuf = (unsigned char *)calloc(IMAGESIZE, sizeof(unsigned char));
                  fpimg = fopen(SENDTOPHONEPATH, "r");       // feedback to Device
                  if(fpimg != NULL)
                  {
                    while(!feof(fpimg))
                    {
                        uart_write(fd, imgbuf, fread(imgbuf, 1, FILESIZE, fpimg));
                    }
                    if(strcmp(checkstr, "iOS") == 0)
                    {
                        uart_write(fd, IOSSUFFIX, strlen(IOSSUFFIX));
                    }
                    fclose(fpimg);
                    free(imgbuf);
                    json_object_put(jobj);      // if new json object, After used must be free json object
                    break;
                  }
              }
              fclose(fp_uart);
          }
        }
        return idx;
    }
    return 0;
}

int ChangBTName(unsigned char *path, unsigned char *rxbuf, unsigned char *filebuf)
{
    unsigned char *LocalBTMac, *CommBTMac;
    unsigned char *NewBTName;
    int recct = 0;
    FILE *fp;
    int fd;
    struct json_object *json_file;

    LocalBTMac = (unsigned char *)calloc(20, sizeof(unsigned char));
    CommBTMac = (unsigned char *)calloc(20, sizeof(unsigned char));
    NewBTName = (unsigned char *)calloc(128, sizeof(unsigned char));
    memset(filebuf, 0, FILESIZE);
    memset(rxbuf, 0, UARTRXSIZE);
    // Read Local Bluetooth MAC address //
    json_file = json_object_from_file("/DaBai/HostDeviceInfo.json");
    #if DBG_EN
        printf("josn file is %s\n", json_object_to_json_string(json_file));
    #endif
    LocalBTMac = json_object_get_string(json_object_object_get(json_file, "Bluetooth MAC Address"));
    memset(filebuf, 0, FILESIZE);

    // Read Command Bluetooth MAC address from Uart interface //
    json_file = json_object_from_file(path);
    #if DBG_EN
        printf("josn file is %s\n", json_object_to_json_string(json_file));
    #endif
    CommBTMac = json_object_get_string(json_object_object_get(json_file, "BTMac"));
    NewBTName = json_object_get_string(json_object_object_get(json_file, "BTName"));

    // compare Mac address //
    // if the same
    // receive %s Receive ChangBTName Command~~!!
    // and reset Bluetooth device to be change Bluetooth device name
    if(strcmp(LocalBTMac, CommBTMac) == 0)
    {
        fd = uart_initial(DEV_UART, BAUDRATE,  DATABIT, PARITY, STOPBIT);
        if(fd < 0)
            return -1;
        sprintf(rxbuf, "%s Receive ChangBTName Command~~!!\n", LocalBTMac);
        uart_write(fd, rxbuf, strlen(rxbuf));
        #if DBG_EN
            printf("Uart Write buf is %s\n", rxbuf);
        #endif
        SetGpioVal(GPIO_RESET_NUM, 0);
        usleep(1000);
        SetGpioVal(GPIO_RESET_NUM, 1);
        memset(rxbuf, 0, UARTRXSIZE);
        while(1)
        {
            recct = uart_read(fd, rxbuf);
            if(recct > 0)
            {
                if((rxbuf[0] == 0xAA) && (rxbuf[3] == 0x8F) && (rxbuf[4] == 0x01))
                {
                    #if DBG_EN
                        printf("Bluetooth Device Restart for Change DevName~~!!!\n");
                    #endif
                    memset(rxbuf, 0, strlen(rxbuf));
                    break;
                }
            }
        }
        close(fd);
        BTModuleChgDevName(rxbuf, NewBTName);
        GetBTModuleName(BTMIDULEPATH, rxbuf, filebuf);
        BTModuleLeaveConfigMode(rxbuf);    // BT Module Leave Config Mode, into Normal Mode
    }
    free(LocalBTMac);
    free(CommBTMac);
    free(NewBTName);
}
