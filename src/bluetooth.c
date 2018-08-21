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

int GetBTModuleInof(unsigned char *path, unsigned char *rxbuf, unsigned char *filebuf)
{
    int recct = 0, fd;
    unsigned char BTMBuf[5] = {0xAA, 0x00, 0x01, 0x01};
    unsigned char Strbuf[32] = "  \"Bluetooth MAC Address\"";
    unsigned int offset = 0;
    FILE *fp;

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
        fp = fopen(path, "r+");
        if(fp != NULL)
        {
            while(!feof(fp))
            {
                fgets(filebuf, FILESIZE, fp);
                #if DBG_EN
                    printf("fgets filebuf : %s\n", filebuf);
                #endif
                if(strstr(filebuf, Strbuf) != NULL)
                {
                  offset = ftell(fp) - strlen(filebuf);
                  sprintf(filebuf, "  \"Bluetooth MAC Address\" : \"%02x:%02x:%02x:%02x:%02x:%02x\",\n", rxbuf[recct - 2], rxbuf[recct - 3], rxbuf[recct - 4], rxbuf[recct - 5], rxbuf[recct - 6], (rxbuf[recct - 8] << 4) | rxbuf[recct - 7]);
                  fread(filebuf + (strlen(filebuf)), 1, FILESIZE - (strlen(filebuf)), fp);
                  break;
                }
            }
            fclose(fp);
            if(offset != 0)
            {
                fp = fopen(path, "r+");
                fseek(fp, offset, SEEK_SET);
                fputs(filebuf, fp);
                #if DBG_EN
                    printf("fputs filebuf : %s\n", filebuf);
                #endif
                fclose(fp);
            }
        }
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
    FILE *fp;

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
        #if DBG_EN
            int i;
            for(i=0;i<recct;i++)
              printf("rxbuf[%d](Hex) : %02x\n", i, rxbuf[i]);
            printf("GetBTModuleName is receive status, count is %d\n", recct);
        #endif
        fp = fopen(path, "r+");
        if(fp != NULL)
        {
            while(!feof(fp))
            {
                fgets(filebuf, FILESIZE, fp);
                #if DBG_EN
                    printf("fgets filebuf : %s\n", filebuf);
                #endif
                if(strstr(filebuf, Strbuf) != NULL)
                {
                  offset = ftell(fp) - strlen(filebuf);
                  sprintf(filebuf, "  \"Bluetooth Device Name\" : \"%s\"\n", (rxbuf + 6));
                  fread(filebuf + (strlen(filebuf)), 1, FILESIZE - (strlen(filebuf)), fp);
                  break;
                }
            }
            fclose(fp);
            if(offset != 0)
            {
                fp = fopen(path, "r+");
                fseek(fp, offset, SEEK_SET);
                fputs(filebuf, fp);
                #if DBG_EN
                    printf("fputs filebuf : %s\n", filebuf);
                #endif
                fclose(fp);
            }
        }
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
        #if DBG_EN
            int i;
            for(i=0;i<recct;i++)
              printf("rxbuf[%d](Hex) : %02x\n", i, rxbuf[i]);
            printf("GetBTModule Leave Config Mode is receive status, count is %d\n", recct);
        #endif
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

int BTTransferUart(int fd, unsigned char *path, unsigned char *rxbuf, unsigned char *filebuf)
{
    int recct = 0, idx = 0, i = 0;;
    unsigned char checkstr[16];
    FILE *fp, *fpimg;
    unsigned char *imgbuf;

    memset(rxbuf, 0, UARTRXSIZE);
    memset(filebuf, 0, FILESIZE);
    recct = uart_read(fd, rxbuf);
    if(recct > 0)
    {
        #if DBG_EN
            printf("rxbuf is : %s\n", rxbuf);
            printf("BTTransferUart is receive status, count is %d\n", recct);
        #endif
        fp = fopen(path, "w");
        if(fp != NULL)
        {
            fwrite(rxbuf, 1, strlen(rxbuf), fp);
            fwrite("\n\0", 1, 1, fp);
            fclose(fp);
        }
        // check index
        sprintf(checkstr, "\"index\":\"");
        rxbuf = strstr(rxbuf, checkstr);
        if(rxbuf != NULL)
        {
            idx = strtoul(rxbuf + (strlen(checkstr)), NULL, 10);
        }
        else
        {
            sprintf(checkstr, "\"index\" : \"");
            rxbuf = strstr(rxbuf, checkstr);
            if(rxbuf != NULL)
            {
                idx = strtoul(rxbuf + (strlen(checkstr)), NULL, 10);
            }
        }
        #if DBG_EN
            printf("BTTransferUart rxbuf : %s\n", rxbuf);
            printf("idx : %d\n", idx);
        #endif
        fp = fopen(path, "r");
        if(fp != NULL)
        {
            printf("Open RxCommTmp file\n");
            while(!feof(fp))
            {
                //fgets(filebuf, UARTRXSIZE, fp);
                fread(filebuf, 1, FILESIZE, fp);
                filebuf = strstr(filebuf, "\"type\":\"");
                if(filebuf != NULL)
                {
                    ReadJsonVal(filebuf + strlen("\"type\":\""), checkstr);
                    imgbuf = (unsigned char *)calloc(FILESIZE, sizeof(unsigned char));
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
                        break;
                    }
                }
            }
            fclose(fp);
        }
        return idx;
    }
    return 0;
}
