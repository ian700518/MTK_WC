// add test program by ian at 2018-04-25
#include "dabai.h"

const unsigned int BaudRateList[10] = {115200, 57600, 38400, 28800, 19200, 14400, 9600, 4800, 2400, 1000000};
const unsigned char BRSValue[10] = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x11};
const unsigned char defeep[8] = {0x00, 0x00, 0x00, 0x1B, 0x49, 0x10, 0x11, 0x33};

int main(int argc, char *argv[])
{
  int fd, recctmain = 0, BTIdx = 0;
  unsigned char *rxbuf;
  unsigned char *filebuf;
  int reg, perr, BRC = 0;
  time_t Current_sec;
  time_t Present_sec;
  unsigned char BTCheckFlag = 0;
  pthread_t thrid;
  struct SocketPara *Sockarg;
  int eeprom_brc, i, j;
  unsigned char membuf[64];
  unsigned int start_addr = 0;
  unsigned char GetEEprom[7];
  unsigned char SetEEprom[40];
  unsigned char SetEEVal[32];
  unsigned int SetEELen;

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
  send_command("devmem 0x1000003C", membuf, sizeof(membuf));
  reg = strtoul(membuf + 2, NULL, 16);
  reg |= 0x001E000F;
  sprintf(membuf, "devmem 0x1000003c 32 0x%08x", reg);
  send_command(membuf, NULL, 0);
  memset(membuf, 0, sizeof(membuf));
  reg = 0;

  // add check argv, if argv[1] is eeprom. It will be set baudrate value, and baudrate value from argv[2]
  DBGMSG("argc is %d, argv is \"", argc);
  for(i=0;i<argc;i++)
  {
    if(i < argc - 1)
      DBGMSG("%s ", argv[i]);
    else
      DBGMSG("%s\"\n", argv[i]);
  }
  send_command("devmem 0x10000c24 32 0x00000003", NULL, 0);        // Set Uart0 HighSpeed to 3, and real uart0 baudrate will used 115200
  PinSetForBMModule();
  fd = uart_initial(DEV_UART, BAUDRATE, DATABIT, PARITY, STOPBIT);
  if(fd < 0)
      return -1;
  if(argc > 1)
  {
    BTEEPROM_MODE = 1;
    DBGMSG("BT Module into EEprom Mode~~!!\ncommand is \"");
    for(i=0;i<argc;i++)
    {
      if(i < (argc - 1))
        DBGMSG("%s ", argv[i]);
      else
        DBGMSG("%s\"\n", argv[i]);
    }
    memset(SetEEVal, 0, sizeof(SetEEVal));
    memset(GetEEprom, 0, sizeof(GetEEprom));
    memset(SetEEprom, 0, sizeof(SetEEprom));
    SetEELen = 0;
    SetBMModuleMode(WEE_mode);
    if(strcmp("baudrate", argv[1]) == 0)    // Set BaudRate command
    {
      eeprom_brc = atoi(argv[2]);
      // get set value from BRSValue list match baudrate list
      for(i=0;i<10;i++)
      {
        if(eeprom_brc == BaudRateList[i])
        {
          BRC = BRSValue[i];
          DBGMSG("BRC is %d\n", BRC);
          break;
        }
      }
      ReadEEpromCommand(GetEEprom, 0x0031, 1);
      // first read eeprom buf for baudrate
      uart_write(fd, GetEEprom, sizeof(GetEEprom));
      DBGMSG("Send read orginal eeprom value command [");
      for(i=0;i<sizeof(GetEEprom);i++)
      {
        if(i < (sizeof(GetEEprom) - 1))
          DBGMSG("0x%02x ", GetEEprom[i]);
        else
          DBGMSG("0x%02x]\n", GetEEprom[i]);
      }
      recctmain = uart_read(fd, rxbuf);
      // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x00 0x31 0x01 0x04
      if(recctmain >= 11)
      {
        DBGMSG("Receive read orginal eeprom value command [");
        for(i=0;i<recctmain;i++)
          DBGMSG("0x%02x ", rxbuf[i]);
        DBGMSG("]\n");
        if((rxbuf[0] == 0x04) && (rxbuf[3] == 0x01) && (rxbuf[4] == 0x29) && (rxbuf[5] == 0xfc) && (rxbuf[6] == 0x00))
        {
          if((rxbuf[10] != BRC) && BRC != 0)
          {
            SetEEVal[0] = BRC;
            SetEELen = WriteEEpromCommand(SetEEprom, 0x0031, 1, SetEEVal);
            //MadeSBRComm(SetBRComm, BRC);
            DBGMSG("Send Write eeprom value command [");
            for(i=0;i<SetEELen;i++)
            {
              if(i < (SetEELen - 1))
                DBGMSG("0x%02x ", SetEEprom[i]);
              else
                DBGMSG("0x%02x]\n", SetEEprom[i]);
            }
            BTModuleReset();
            uart_write(fd, SetEEprom, SetEELen);
            memset(rxbuf, 0, strlen(rxbuf));
            recctmain = uart_read(fd, rxbuf);
            // rxbuf --> 0x04 0x0e 0x04 0x01 0x27 0xfc 0x00 (Success)
            if(recctmain >= 7)
            {
              DBGMSG("Receive Write eeprom value command [");
              for(i=0;i<recctmain;i++)
                DBGMSG("0x%02x ", rxbuf[i]);
              DBGMSG("]\n");
              if((rxbuf[0] == 0x04) && (rxbuf[3] == 0x01) && (rxbuf[4] == 0x27) && (rxbuf[6] == 0x00))
              {
                DBGMSG("set eeprom baudrate val success~~!!!\n \
                        ====================================\n \
                        == Reseting Bluetooth Module ~~!! ==\n \
                        ====================================\n");

                BTModuleReset();
                ReadEEpromCommand(GetEEprom, 0x0031, 1);
                DBGMSG("After Setting~~~~~\nSend Read eeprom value command [");
                for(i=0;i<sizeof(GetEEprom);i++)
                {
                  if(i < (sizeof(GetEEprom) - 1))
                    DBGMSG("0x%02x ", GetEEprom[i]);
                  else
                    DBGMSG("0x%02x]\n", GetEEprom[i]);
                }
                uart_write(fd, GetEEprom, sizeof(GetEEprom));
                recctmain = uart_read(fd, rxbuf);
                // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x00 0x31 0x01 0x00
                if(recctmain >= 1)
                {
                  DBGMSG("After Setting~~~~~\nReceive read eeprom value command [");
                  for(i=0;i<recctmain;i++)
                    DBGMSG("0x%02x ", rxbuf[i]);
                  DBGMSG("]\n");
                  close(fd);
                  exit(0);
                }
              }
              else
              {
                DBGMSG("Read baudrate from eeprom, and receive data is not correct~!");
                close(fd);
                exit(0);
              }
            }
            else
            {
              DBGMSG("After Setting Baudrate command, there is not receive any data !!!!!!!!!!\n");
              close(fd);
              exit(0);
            }
          }
          else
          {
            DBGMSG("The New Baurdate Value is the same with EEProm Value !!!!!!!!!!\n");
            close(fd);
            exit(0);
          }
        }
      }
      else
      {
        DBGMSG("There is not receive any data~~~~~!!!!!");
        close(fd);
        exit(0);
      }
    }
    else if(strcmp("eeprom", argv[1]) == 0)
    {
      if(atoi(argv[2]) <= 0x4FF)
        ReadEEpromCommand(GetEEprom, atoi(argv[2]), 0x10);
      else
        ReadEEpromCommand(GetEEprom, 0x0000, 0x10);
      uart_write(fd, GetEEprom, sizeof(GetEEprom));
      DBGMSG("Send Geteeprom command : [");
      for(i=0;i<sizeof(GetEEprom);i++)
      {
        if(i < (sizeof(GetEEprom) - 1))
          DBGMSG("0x%02x ", GetEEprom[i]);
        else
          DBGMSG("0x%02x]\n", GetEEprom[i]);
      }
      recctmain = uart_read(fd, rxbuf);
      if(recctmain >= 11)
      {
        DBGMSG("EEprom value is [");
        for(i=10,j=0;i<recctmain;i++,j++)
        {
          if(i < (recctmain - 1))
          {
            if(j==8)
              DBGMSG("\t");
            DBGMSG("%02x ", rxbuf[i]);
          }
          else
          {
            DBGMSG("%02x]\n", rxbuf[i]);
          }
        }
      }
      BTModuleReset();
      close(fd);
      exit(0);
    }
    else if(strcmp("WREE", argv[1]) == 0)
    {
      if(atoi(argv[2]) == 0x50)
      {
        SetEELen = WriteEEpromCommand(SetEEprom, 0x0050, 8, defeep);
        //MadeSBRComm(SetBRComm, BRC);
        DBGMSG("Send Write eeprom value command [");
        for(i=0;i<SetEELen;i++)
        {
          if(i < (SetEELen - 1))
            DBGMSG("0x%02x ", SetEEprom[i]);
          else
            DBGMSG("0x%02x]\n", SetEEprom[i]);
        }
        BTModuleReset();
        uart_write(fd, GetEEprom, SetEELen);
        memset(rxbuf, 0, strlen(rxbuf));
        recctmain = uart_read(fd, rxbuf);
        // rxbuf --> 0x04 0x0e 0x04 0x01 0x27 0xfc 0x00 (Success)
        if(recctmain >= 7)
        {
          DBGMSG("Receive Write eeprom value command [");
          for(i=0;i<recctmain;i++)
            DBGMSG("0x%02x ", rxbuf[i]);
          DBGMSG("]\n");
          BTModuleReset();
          ReadEEpromCommand(GetEEprom, atoi(argv[2]), 0x10);
          uart_write(fd, GetEEprom, sizeof(GetEEprom));
          DBGMSG("Send Geteeprom command : [");
          for(i=0;i<sizeof(GetEEprom);i++)
          {
            if(i < (sizeof(GetEEprom) - 1))
              DBGMSG("0x%02x ", GetEEprom[i]);
            else
              DBGMSG("0x%02x]\n", GetEEprom[i]);
          }
          recctmain = uart_read(fd, rxbuf);
          if(recctmain >= 11)
          {
            DBGMSG("EEprom value is [");
            for(i=10,j=0;i<recctmain;i++,j++)
            {
              if(i < (recctmain - 1))
              {
                if(j==8)
                  DBGMSG("\t");
                DBGMSG("%02x ", rxbuf[i]);
              }
              else
              {
                DBGMSG("%02x]\n", rxbuf[i]);
              }
            }
          }
        }
      }
      BTModuleReset();
      close(fd);
      exit(0);
    }
    else
    {
      // get device name
      i = 0;
      while(1)
      {
        switch(i)
        {
          case 0:
            ReadEEpromCommand(GetEEprom, 0x000B, 0x20);     // Get Device Name
            break;
          case 1:
            ReadEEpromCommand(GetEEprom, 0x0000, 0x06);     // Get Device MAC address
            break;
          case 2:
            ReadEEpromCommand(GetEEprom, 0x038B, 0x01);     // Get Device Operation pattern delay time
            break;
        }
        uart_write(fd, GetEEprom, sizeof(GetEEprom));
        DBGMSG("Send read Device Name value command [");
        for(i=0;i<sizeof(GetEEprom);i++)
        {
          if(i < (sizeof(GetEEprom) - 1))
            DBGMSG("0x%02x ", GetEEprom[i]);
          else
            DBGMSG("0x%02x]\n", GetEEprom[i]);
        }
        recctmain = uart_read(fd, rxbuf);
        if(recctmain >= 11)
        {
          // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x00 0x0B 0x20 0x04 ... ... ... ...
          DBGMSG("Receive read Device Name value command [");
          for(i=0;i<recctmain;i++)
            DBGMSG("0x%02x ", rxbuf[i]);
          DBGMSG("]\n");
        }
        sleep(5);
        if(i>=4)
          break;
        else
          i++;
      }
      close(fd);
      exit(0);
    }
  }
  else
  {
    BTEEPROM_MODE = 0;
    DBGMSG("BT Module into Normal Mode~~!!\n command is \"%s\", parameter is %d\n", argv[0], argc);
    fd = uart_initial(DEV_UART, BAUDRATE, DATABIT, PARITY, STOPBIT);
    if(fd < 0)
    {
      DBGMSG("Uart open error~~~!!!\n");
      return -1;
    }
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
        for(i=0;i<recctmain;i++)
          DBGMSG("rxbuf[%d] is : 0x%x\n", i, rxbuf[i]);
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
    SetBMModuleMode(normal_mode);
    GetDeviceMACAddr(BTMIDULEPATH, filebuf);   // Get Device Network MAC address
    GpioPinMode();
    DBGMSG("Host Device Data finishi~~~!!!\n");
    //GetBTConfigFlag = 1;
    fd = uart_initial(DEV_UART, BAUDRATE, DATABIT, PARITY, STOPBIT);
    if(fd < 0)
    {
      DBGMSG("Uart open error~~~!!!\n");
      return -1;
    }
    DBGMSG("Dabai process starting~~~!!!\n");

    /*
      first :  clear ChargeDevice array.
      second : if has any charge device information in the file, write it into ChargeDevice array.
    */
    ChargeDeviceCount = 0;
    for(i=0;i<CHGDEVMAX;i++)
    {
      memset(ChargeDevice+i, 0, sizeof(struct ClientDev));
    }
    ChargeDeviceCount = GetChgDevFromFile("/DaBai/OnlineChgList.json", ChargeDevice);

    Sockarg = (struct SocketPara *)malloc(sizeof(struct SocketPara));
    Sockarg->Addr = "192.168.100.100";
    Sockarg->Port = 12345;

    DBGMSG("before pthread_create~~~~~~~\n");
    pthread_create(&thrid, NULL, SockConnProcess, (void *)Sockarg);

    // for test chage list
    /*
    #if 0
    for(i=0;i<CHGDEVMAX;i++)
    {
      time(&Current_sec);
      ChargeDevice[i].CurrentTime = Current_sec;
    }
    #endif
    // for test chage list
    */
    time(&Current_sec);
    Present_sec = Current_sec;
    while(1)
    {
      time(&Current_sec);
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
              DBGMSG("Uart open error~~~!!!\n");
              //return -1;
          }
          break;

        case -1:
          DBGMSG("Uart receive format is error!!!\n");
          break;

        default:
          DBGMSG("Uart not receive any data\n");
          break;
      }
      // for test charge List
      /*
      #if 0
        for(i=0;i<ChargeDeviceCount;i++)
        {
          time(&Current_sec);
          if((Current_sec - Present_sec) < 20)
          {
            if((strcmp((ChargeDevice+i)->DevUserId, "Demo001") == 0) || (strcmp((ChargeDevice+i)->DevUserId, "Demo002") == 0) || (strcmp((ChargeDevice+i)->DevUserId, "Demo005") == 0))
            {
              DBGMSG("DevUserId is %s\n", (ChargeDevice+i)->DevUserId);
              ChargeDevice[i].CurrentTime = Current_sec;
            }
          }
          else if((Current_sec - Present_sec) >= 20 && (Current_sec - Present_sec) < 40)
          {
            if((strcmp((ChargeDevice+i)->DevUserId, "Demo002") == 0) || (strcmp((ChargeDevice+i)->DevUserId, "Demo005") == 0))
            {
              DBGMSG("DevUserId is %s\n", (ChargeDevice+i)->DevUserId);
              ChargeDevice[i].CurrentTime = Current_sec;
            }
          }
          else if((Current_sec - Present_sec) >= 40 && (Current_sec - Present_sec) < 60)
          {
            if((strcmp((ChargeDevice+i)->DevUserId, "Demo005") == 0))
            {
              DBGMSG("DevUserId is %s\n", (ChargeDevice+i)->DevUserId);
              ChargeDevice[i].CurrentTime = Current_sec;
            }
          }
        }
      #endif
      // for test charge List
      */
      if((Current_sec - Present_sec) >= CHKCHGLISTDLY)
      {
        Present_sec = Current_sec;
        ChargeDeviceCount = UpdateChgDevice(ChargeDevice, CDVTemp, filebuf);
        if(ChargeDeviceCount == 0)
          SetGpioVal(GPIO_WCEN_NUM, 0);
        DBGMSG("ChargeDeviceCount is %d\n", ChargeDeviceCount);
      }

      /*
        if the system has the "Demolist.json" file, check RxCommTemp.txt command.
        if the parameter "userId" which in the RxCommTmp.txt include "Demolist.json" userId.
        there will be enable wireless charge function
      */
      if(ChargeDeviceCount != 0 && (GetGpioVal(GPIO_WCEN_NUM) == 0))
      {
        SetGpioVal(GPIO_WCEN_NUM, 1);
      }
      usleep(100000);
    }
  }
  close(fd);
  exit(0);
}
