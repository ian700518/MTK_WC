// add test program by ian at 2018-04-25
#include "dabai.h"

const unsigned int BaudRateList[10] = {115200, 57600, 38400, 28800, 19200, 14400, 9600, 4800, 2400, 1000000};
const unsigned char BRSValue[10] = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x11};

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
    reg |= 0x000E000F;
    sprintf(membuf, "devmem 0x1000003c 32 0x%08x", reg);
    send_command(membuf, NULL, 0);
    memset(membuf, 0, sizeof(membuf));
    reg = 0;

    // add check argv, if argv[1] is eeprom. It will be set baudrate value, and baudrate value from argv[2]
    printf("argc is %d, argv is %s %s %s\n", argc, argv[0], argv[1], argv[2]);
    send_command("devmem 0x10000c24 32 0x00000003", NULL, 0);        // Set Uart0 HighSpeed to 1, and real uart0 baudrate will used 115200
    PinSetForBMModule();
    if(argc > 1)
    {
      BTEEPROM_MODE = 1;
      printf("BT Module into EEprom Mode~~!!\ncommand is \"");
      fd = uart_initial(DEV_UART, BAUDRATE, DATABIT, PARITY, STOPBIT);
      if(fd < 0)
          return -1;
      for(i=0;i<argc;i++)
      {
        if(i < (argc - 1))
          printf("%s ", argv[i]);
        else
          printf("%s\"\n", argv[i]);
      }
      memset(SetEEVal, 0, sizeof(SetEEVal));
      memset(GetEEprom, 0, sizeof(GetEEprom));
      memset(SetEEprom, 0, sizeof(SetEEprom));
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
            printf("BRC is %d\n", BRC);
            break;
          }
        }
        ReadEEpromCommand(GetEEprom, 0x0031, 1);
        // first read eeprom buf for baudrate
        uart_write(fd, GetEEprom, sizeof(GetEEprom));
        printf("Send read orginal eeprom value command [");
        for(i=0;i<sizeof(GetEEprom);i++)
        {
          if(i < (sizeof(GetEEprom) - 1))
            printf("0x%02x ", GetEEprom[i]);
          else
            printf("0x%02x]\n", GetEEprom[i]);
        }
        recctmain = uart_read(fd, rxbuf);
        // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x00 0x31 0x01 0x04
        if(recctmain >= 11)
        {
          printf("Receive read orginal eeprom value command [");
          for(i=0;i<recctmain;i++)
            printf("0x%02x ", rxbuf[i]);
          printf("]\n");
          if((rxbuf[0] == 0x04) && (rxbuf[3] == 0x01) && (rxbuf[4] == 0x29) && (rxbuf[5] == 0xfc) && (rxbuf[6] == 0x00))
          {
            if((rxbuf[10] != BRC) && BRC != 0)
            {
              SetEEVal[0] = BRC;
              WriteEEpromCommand(SetEEprom, 0x0031, 1, SetEEVal);
              //MadeSBRComm(SetBRComm, BRC);
              printf("Send Write eeprom value command [");
              for(i=0;i<sizeof(SetEEprom);i++)
              {
                if(i < (sizeof(SetEEprom) - 1))
                  printf("0x%02x ", SetEEprom[i]);
                else
                  printf("0x%02x]\n", SetEEprom[i]);
              }
              BTModuleReset();
              uart_write(fd, SetEEprom, sizeof(SetEEprom));
              memset(rxbuf, 0, strlen(rxbuf));
              recctmain = uart_read(fd, rxbuf);
              // rxbuf --> 0x04 0x0e 0x04 0x01 0x27 0xfc 0x00 (Success)
              if(recctmain >= 7)
              {
                printf("Receive Write eeprom value command [");
                for(i=0;i<recctmain;i++)
                  printf("0x%02x ", rxbuf[i]);
                printf("]\n");
                if((rxbuf[0] == 0x04) && (rxbuf[3] == 0x01) && (rxbuf[4] == 0x27) && (rxbuf[6] == 0x00))
                {
                  printf("set eeprom baudrate val success~~!!!\n \
                          ####################################\n \
                          Reseting Bluetooth Module ~~!!!\n \
                          ####################################\n");


                  BTModuleReset();
                  ReadEEpromCommand(GetEEprom, 0x0031, 1);
                  printf("After Setting~~~~~\nSend Read eeprom value command [");
                  for(i=0;i<sizeof(GetEEprom);i++)
                  {
                    if(i < (sizeof(GetEEprom) - 1))
                      printf("0x%02x ", GetEEprom[i]);
                    else
                      printf("0x%02x]\n", GetEEprom[i]);
                  }
                  uart_write(fd, GetEEprom, sizeof(GetEEprom));
                  recctmain = uart_read(fd, rxbuf);
                  // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x00 0x31 0x01 0x00
                  if(recctmain >= 1)
                  {
                    printf("After Setting~~~~~\nReceive read eeprom value command [");
                    for(i=0;i<recctmain;i++)
                      printf("0x%02x ", rxbuf[i]);
                    printf("]\n");
                    exit(0);
                  }
                }
                else
                {
                  printf("Read baudrate from eeprom, and receive data is not correct~!");
                  exit(0);
                }
              }
              else
              {
                printf("After Setting Baudrate command, there is not receive any data !!!!!!!!!!\n");
                exit(0);
              }
            }
            else
            {
              printf("The New Baurdate Value is the same with EEProm Value !!!!!!!!!!\n");
              exit(0);
            }
          }
        }
        else
        {
          printf("There is not receive any data~~~~~!!!!!");
          exit(0);
        }
      }
      else if(strcmp("operation", argv[1]) == 0)
      {
        // get Pattern mode
        //eeprom_op = atoi(argv[2]);
        ReadEEpromCommand(GetEEprom, 0x01B1, 1);
        uart_write(fd, GetEEprom, sizeof(GetEEprom));
        printf("Send read Operation Pattern value command [");
        for(i=0;i<sizeof(GetEEprom);i++)
        {
          if(i < (sizeof(GetEEprom) - 1))
            printf("0x%02x ", GetEEprom[i]);
          else
            printf("0x%02x]\n", GetEEprom[i]);
        }
        recctmain = uart_read(fd, rxbuf);
        if(recctmain >= 11)
        {
          // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x01 0xB1 0x01 0x04
          printf("Receive read Operation Pattern value command [");
          for(i=0;i<recctmain;i++)
            printf("0x%02x ", rxbuf[i]);
          printf("]\n");
          if(rxbuf[10] & 0x04)
            printf("Operation Pattern is Manual Mode\n");
          else
            printf("Operation Pattern is Auto Mode\n");
          // write setting value to eeprom
          if(((rxbuf[10] & 0x04) >> 2) != (atoi(argv[2])))
          {
            if(atoi(argv[2]) == 0)
              SetEEVal[0] = rxbuf[10] & 0xFB;
              //MadeSOPComm(SetOPComm, (rxbuf[10] & 0xFB));
            else
              SetEEVal[0] = rxbuf[10] | 0x04;
              //MadeSOPComm(SetOPComm, (rxbuf[10] | 0x04));
            WriteEEpromCommand(SetEEprom, 0x01B1, 1, SetEEVal);
            printf("Send Write eeprom value command [");
            for(i=0;i<sizeof(SetEEprom);i++)
            {
              if(i < (sizeof(SetEEprom) - 1))
                printf("0x%02x ", SetEEprom[i]);
              else
                printf("0x%02x]\n", SetEEprom[i]);
            }
            BTModuleReset();
            uart_write(fd, SetEEprom, sizeof(SetEEprom));
            memset(rxbuf, 0, strlen(rxbuf));
            recctmain = uart_read(fd, rxbuf);
            // rxbuf --> 0x04 0x0e 0x04 0x01 0x27 0xfc 0x00 (Success)
            if(recctmain >= 7)
            {
              printf("Receive Operation Pattern value command [");
              for(i=0;i<recctmain;i++)
                printf("0x%02x ", rxbuf[i]);
              printf("]\n");
              if((rxbuf[0] == 0x04) && (rxbuf[3] == 0x01) && (rxbuf[4] == 0x27) && (rxbuf[6] == 0x00))
              {
                printf("set eeprom Operation Pattern val success~~!!!\n \
                        ####################################\n \
                        Reseting Bluetooth Module ~~!!!\n \
                        ####################################\n");
                BTModuleReset();
                printf("After Setting~~~~~\nSend Read Operation Pattern command [");
                ReadEEpromCommand(GetEEprom, 0x01B1, 1);
                for(i=0;i<sizeof(GetEEprom);i++)
                {
                  if(i < (sizeof(GetEEprom) - 1))
                    printf("0x%02x ", GetEEprom[i]);
                  else
                    printf("0x%02x]\n", GetEEprom[i]);
                }
                uart_write(fd, GetEEprom, sizeof(GetEEprom));
                recctmain = uart_read(fd, rxbuf);
                //printf("After setting //read eeprom recive count %d\n", recctmain);
                // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x00 0x31 0x01 0x00
                if(recctmain >= 1)
                {
                  printf("After Setting~~~~~\nReceive read Operation Pattern command [");
                  for(i=0;i<recctmain;i++)
                    printf("0x%02x ", rxbuf[i]);
                  printf("]\n");
                  exit(0);
                }
              }
              else
              {
                printf("Read operation pattern from eeprom, and receive data is not correct~!");
                exit(0);
              }
            }
            else
            {
              printf("After Setting Operation pattern command, there is not receive any data !!!!!!!!!!\n");
              exit(0);
            }
          }
          else
          {
            printf("The New Operation pattern Value is the same with EEProm Value !!!!!!!!!!\n");
            exit(0);
          }
        }
        else
        {
          printf("There is not receive any data~~~~~!!!!!");
          exit(0);
        }
      }
      else if(strcmp("pdly", argv[1]) == 0)
      {
        // get Pattern delay time
        ReadEEpromCommand(GetEEprom, 0x038B, 1);
        uart_write(fd, GetEEprom, sizeof(GetEEprom));
        printf("Send read Operation Pattern Delay Time value command [");
        for(i=0;i<sizeof(GetEEprom);i++)
        {
          if(i < (sizeof(GetEEprom) - 1))
            printf("0x%02x ", GetEEprom[i]);
          else
            printf("0x%02x]\n", GetEEprom[i]);
        }
        recctmain = uart_read(fd, rxbuf);
        if(recctmain >= 11)
        {
          // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x01 0xB1 0x01 0x04
          printf("Receive read Operation Pattern Delay Time value command [");
          for(i=0;i<recctmain;i++)
            printf("0x%02x ", rxbuf[i]);
          printf("]\n");
          printf("Operation Pattern Delay Time is %02x\n", rxbuf[10]);
          // write setting value to eeprom
          if(rxbuf[10] != (atoi(argv[2])))
          {
            SetEEVal[0] = atoi(argv[2]);
            WriteEEpromCommand(SetEEprom, 0x038B, 1, SetEEVal);
            //MadeSOPTComm(SetOPTComm, (atoi(argv[2])));
            printf("Send Write Opteration Pattern Delay Time value command [");
            for(i=0;i<sizeof(SetEEprom);i++)
            {
              if(i < (sizeof(SetEEprom) - 1))
                printf("0x%02x ", SetEEprom[i]);
              else
                printf("0x%02x]\n", SetEEprom[i]);
            }
            BTModuleReset();
            uart_write(fd, SetEEprom, sizeof(SetEEprom));
            memset(rxbuf, 0, strlen(rxbuf));
            recctmain = uart_read(fd, rxbuf);
            // rxbuf --> 0x04 0x0e 0x04 0x01 0x27 0xfc 0x00 (Success)
            if(recctmain >= 7)
            {
              printf("Receive Write Operation Pattern Delay Time value command [");
              for(i=0;i<recctmain;i++)
                printf("0x%02x ", rxbuf[i]);
              printf("]\n");
              if((rxbuf[0] == 0x04) && (rxbuf[3] == 0x01) && (rxbuf[4] == 0x27) && (rxbuf[6] == 0x00))
              {
                printf("set eeprom Operation Pattern Delay Time val success~~!!!\n \
                        ####################################\n \
                        Reseting Bluetooth Module ~~!!!\n \
                        ####################################\n");
                BTModuleReset();
                ReadEEpromCommand(GetEEprom, 0x038B, 1);
                printf("After Setting~~~~~\nSend Read Operation Pattern Delay Time command [");
                for(i=0;i<sizeof(GetEEprom);i++)
                {
                  if(i < (sizeof(GetEEprom) - 1))
                    printf("0x%02x ", GetEEprom[i]);
                  else
                    printf("0x%02x]\n", GetEEprom[i]);
                }
                uart_write(fd, GetEEprom, sizeof(GetEEprom));
                recctmain = uart_read(fd, rxbuf);
                //printf("After setting //read eeprom recive count %d\n", recctmain);
                // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x00 0x31 0x01 0x00
                if(recctmain >= 1)
                {
                  printf("After Setting~~~~~\nReceive read Operation Pattern Delay Time command [");
                  for(i=0;i<recctmain;i++)
                    printf("0x%02x ", rxbuf[i]);
                  printf("]\n");
                  exit(0);
                }
              }
              else
              {
                printf("Read operation pattern from eeprom, and receive data is not correct~!");
                exit(0);
              }
            }
            else
            {
              printf("After Setting Operation pattern command, there is not receive any data !!!!!!!!!!\n");
              exit(0);
            }
          }
          else
          {
            printf("The New Operation pattern Delay Time Value is the same with EEProm Value !!!!!!!!!!\n");
            exit(0);
          }
        }
        else
        {
          printf("There is not receive any data~~~~~!!!!!");
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
        printf("Send Geteeprom command : [");
        for(i=0;i<sizeof(GetEEprom);i++)
        {
          if(i < (sizeof(GetEEprom) - 1))
            printf("0x%02x ", GetEEprom[i]);
          else
            printf("0x%02x]\n", GetEEprom[i]);
        }
        recctmain = uart_read(fd, rxbuf);
        if(recctmain >= 11)
        {
          printf("EEprom value is [");
          for(i=10,j=0;i<recctmain;i++,j++)
          {
            if(i < (recctmain - 1))
            {
              if(j==8)
                printf("\t");
              printf("%02x ", rxbuf[i]);
            }
            else
            {
              printf("%02x]\n", rxbuf[i]);
            }
          }
        }
        BTModuleReset();
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
          printf("Send read Device Name value command [");
          for(i=0;i<sizeof(GetEEprom);i++)
          {
            if(i < (sizeof(GetEEprom) - 1))
              printf("0x%02x ", GetEEprom[i]);
            else
              printf("0x%02x]\n", GetEEprom[i]);
          }
          recctmain = uart_read(fd, rxbuf);
          if(recctmain >= 11)
          {
            // rxbuf --> 0x04 0x0e 0x08 0x01 0x29 0xfc 0x00 0x00 0x0B 0x20 0x04 ... ... ... ...
            printf("Receive read Device Name value command [");
            for(i=0;i<recctmain;i++)
              printf("0x%02x ", rxbuf[i]);
            printf("]\n");
          }
          sleep(5);
          if(i>=4)
            break;
          else
            i++;
        }
        exit(0);
      }
    }
    else
    {
      BTEEPROM_MODE = 1;
      printf("BT Module into Normal Mode~~!!\n command is \"%s\", parameter is %d\n", argv[0], argc);
      PinSetForBMModule();
      ////GetBTConfigFlag = 0;
      ////BTIntoConfigMode = 0;
      ////ChangBTModuleNameFlag = 0;
      // detect BT module and read bt module mac address
      ////while(BTIntoConfigMode == 0)
      ////{
      ////  recctmain = uart_read(fd, rxbuf);
      ////  if(recctmain > 0)
      ////  {
      ////    for(i=0;i<recctmain;i++)
      ////      printf("rxbuf[%d] is : 0x%x\n", i, rxbuf[i]);
      ////    if((rxbuf[0] == 0xAA) && (rxbuf[3] == 0x8F) && (rxbuf[4] == 0x01))
      ////    {
      ////        memset(rxbuf, 0, strlen(rxbuf));
      ////        BTIntoConfigMode = 1;
      ////    }
      ////  }
      ////}
      ////close(fd);
      GetBTModuleName(BTMIDULEPATH, rxbuf, filebuf);    // Get BT Module Name
      GetBTModuleInof(BTMIDULEPATH, rxbuf, filebuf);    // Get BT Module Information like MAC address
      BTModuleLeaveConfigMode(rxbuf);    // BT Module Leave Config Mode, into Normal Mode
      SetBMModuleMode(normal_mode);
      GetDeviceMACAddr(BTMIDULEPATH, filebuf);   // Get Device Network MAC address
      GpioPinMode();
      printf("Host Device Data finishi~~~!!!\n");
      time(&Current_sec);
      Present_sec = Current_sec;
      //GetBTConfigFlag = 1;
      fd = uart_initial(DEV_UART, BAUDRATE, DATABIT, PARITY, STOPBIT);
      if(fd < 0)
      {
          printf("Uart open error~~~!!!\n");
          return -1;
      }
      printf("Dabai process starting~~~!!!\n");

      Sockarg = (struct SocketPara *)malloc(sizeof(struct SocketPara));
      Sockarg->Addr = "192.168.100.100";
      Sockarg->Port = 12345;

      printf("before pthread_create~~~~~~~\n");
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
                printf("Uart receive format is error\n");
                break;
          }
          sleep(1);
      }
    }
    exit(0);
}
