// add test program by ian at 2018-04-25
#include "dabai.h"


void test_pthread1(void)
{
  printf("This is pthread test 1 function~~~!!!\n");
}

void test_pthread2(void)
{
  printf("This is pthread test 2 function~~~!!!\n");
}

int main()
{
    int fd, recctmain = 0, BTIdx = 0;
    unsigned char *rxbuf;
    unsigned char *filebuf;
    int reg, perr;
    time_t Current_sec;
    time_t Present_sec;
    unsigned char BTCheckFlag = 0;
    pthread_t thrid;
    struct SocketPara *Sockarg;

    rxbuf = (unsigned char *)calloc(UARTRXSIZE, sizeof(unsigned char));
    filebuf = (unsigned char *)calloc(FILESIZE, sizeof(unsigned char));
    // set pinmux register
    // set i2c pin to gpio
    send_command("mt7688_pinmux set i2c gpio", NULL, 0);
    // set uart1 pin to gpio
    send_command("mt7688_pinmux set uart1 gpio", NULL, 0);
    // set pwm0 pin to gpio
    send_command("mt7688_pinmux set pwm0 gpio", NULL, 0);
    // set pwm1 pin to gpio
    send_command("mt7688_pinmux set pwm1 gpio", NULL, 0);

    // set AGPIO_CFG
    send_command("devmem 0x1000003C", rxbuf, sizeof(rxbuf));
    reg = strtoul(rxbuf + 2, NULL, 16);
    reg |= 0x000E000F;
    sprintf(rxbuf, "devmem 0x1000003c 32 0x%08x", reg);
    send_command(rxbuf, NULL, 0);
    memset(rxbuf, 0, strlen(rxbuf));

    fd = uart_initial(DEV_UART, BAUDRATE, DATABIT, PARITY, STOPBIT);
    if(fd < 0)
        return -1;
    PinSetForBMModule();
    SetBMModuleMode(normal_mode);
    GetBTConfigFlag = 0;
    BTIntoConfigMode = 0;
    ChangBTModuleNameFlag = 0;
    // detect BT module and read bt module mac address
    while(BTIntoConfigMode == 0)
    {
        recctmain = uart_read(fd, rxbuf);
        if(recctmain > 0)
        {
            if((rxbuf[0] == 0xAA) && (rxbuf[3] == 0x8F) && (rxbuf[4] == 0x01))
            {
                memset(rxbuf, 0, strlen(rxbuf));
                BTIntoConfigMode = 1;
            }
        }
    }
    close(fd);
    GetBTModuleName(BTMIDULEPATH, rxbuf, filebuf);    // Get BT Module Name
    GetBTModuleInof(BTMIDULEPATH, rxbuf, filebuf);    // Get BT Module Information like MAC address
    BTModuleLeaveConfigMode(rxbuf);    // BT Module Leave Config Mode, into Normal Mode
    GetDeviceMACAddr(BTMIDULEPATH, filebuf);   // Get Device Network MAC address
    GpioPinMode();
    printf("Host Device Data finishi~~~!!!\n");
    time(&Current_sec);
    Present_sec = Current_sec;
    GetBTConfigFlag = 1;
    fd = uart_initial(DEV_UART, BAUDRATE, DATABIT, PARITY, STOPBIT);
    if(fd < 0)
    {
        printf("Uart open error~~~!!!\n");
        return -1;
    }
    printf("Dabai process starting~~~!!!\n");

    Sockarg = (struct SocketPara *)malloc(sizeof(struct SocketPara));
    Sockarg->Addr = "www.ian.com";
    Sockarg->Port = 12345;
    Sockarg->NameAddr = 1;

    pthread_create(&thrid, NULL, SockConnProcess, (void *)Sockarg);
    while(1)
    {
        switch(BTTransferUart(fd, "DaBai/RxCommTmp.txt", rxbuf, filebuf))
        {
            case 1:     // Set Device Network Information
              //BTIdx = 0;
              break;

            case 2:     // Devive Send Account and Devive information to check will be charged
              CheckCHGDevInfo("DaBai/RxCommTmp.txt", ChargeDevice, &ChargeDeviceCount, filebuf);
              //BTIdx = 0;
              break;

            case 3:     // Set Bluetooth Device Name
              close(fd);
              ChangBTName("DaBai/RxCommTmp.txt", rxbuf, filebuf);
              //BTIdx = 0;
              fd = uart_initial(DEV_UART, BAUDRATE, DATABIT, PARITY, STOPBIT);
              if(fd < 0)
              {
                  printf("Uart open error~~~!!!\n");
                  //return -1;
              }
              break;

            default:
              break;
        }
        sleep(1);
    }

    exit(0);
}
