// add test program by ian at 2018-04-25
#include "dabai.h"

/* read string */
void readjasonvalue(char src[], char des[])
{
    int i = 0, j = 0;

    // check first " symbol
    while(src[i] != '\"')
    {
        i++;
    }
    i++;
    while(src[j + i] != '\"')
    {
        des[j] = src[j + i];
        j++;
    }
    des[j] = '\0';
}

/*check iOS system MAC address*/
void checkiOSMac(char src[])
{
    int length;
    int i,j;

    length = strlen(src);
    for(j=0;j<12;j++)
    {
        src[j] = src[length - 12 + j];
    }
    src[j] = '\0';
}

/*将字符串s中出现的字符c删除*/
void squeeze(char s[],int c)
{
    int i,j;

    for (i = 0, j = 0; s[i] != '\0'; i++)
    {
        if (s[i] != c)
        {
            s[j++] = s[i];
        }
    }
    s[j] = '\0';    //这一条语句千万不能忘记，字符串的结束标记
}

// client device information write into online list
int write_online_list(struct ClientDev client)
{
    FILE *fp;
    char OldStr[512], NewStr[512];
    int length;
    struct tm *info;

    /* read current time */
    time(&client.CurrentTime);
    info = localtime(&client.CurrentTime);
    strftime(client.DevFormatCTime, 64, "%x - %X", info);
    /* read current time */

    /* ----------------- */
    // if client Start time is zero, the client devive was first connect to the host.
    // will be write current time to start time.
    /* ----------------- */
    if(client.StartTime == 0)
    {
        client.StartTime = client.CurrentTime;
        info = localtime(&client.CurrentTime);
        strftime(client.DevFormatSTime, 64, "%x - %X", info);
    }
    fp = fopen("DaBai/Onlist.txt", "a+");
    if(fp)
    {
        sprintf(NewStr, "%s-%s-%s-%s %s[%ld] %s[%ld]\n", client.DevType, client.DevMac, client.DevUserId, client.DevAccount, client.DevFormatSTime, client.StartTime, client.DevFormatCTime, client.CurrentTime);
        length = strlen(client.DevType) + strlen(client.DevMac) + strlen(client.DevUserId) + strlen(client.DevAccount);
        while(!feof(fp))
        {
            // check device at list already, if device has at list return otherwise write into onlist.txt
            if(fgets(OldStr, sizeof(OldStr), fp) != NULL)
            {
                if(strncmp(OldStr, NewStr, length) == 0)
                {
                    fclose(fp);
                    return 1;
                }
            }
        }
        fprintf(fp, NewStr);
        fclose(fp);
        return 0;
    }
    else
    {
        return -1;
    }
}

int check_data_format(char *path, int bytenum, char *buf2)
{
    char *tmp, *tmp1, ucicommand[64];
    int  i;
    int type = 0, file_length;
    struct ClientDev CDev;
    FILE *fp;
    char buf[512];
    char stringnum[3];


    fp = fopen(path, "r");
    fseek(fp, 0, SEEK_END); //定位到文件末
    file_length = ftell(fp);
    rewind(fp);
    while(!feof(fp))
    {
        fread(buf, 1, file_length, fp);
    }
    fclose(fp);
    printf("buf : %s\n", buf);
    // check data profile : first char is "{" and last char is "}"
    if((strchr(buf, '{') == NULL) || (strrchr(buf, '}') == NULL))
    {
        //printf("Data profile is serror\n");
        //sprintf(buf2, "Data profile is error\n");
        return Err_Profile;
    }

    // check data is include index value
    //printf("buf : %s\n", buf);
    if((tmp = strstr(buf, "index")) != NULL)
    {
        //strncpy(stringnum, tmp + strlen("index\""), 2);
        //printf("tmp : %s\n", tmp);
        //printf("tmp : %s\n", tmp + strlen("index\""));
        readjasonvalue(tmp + strlen("index\""), stringnum);
        //printf("stringnum = %s\n", stringnum);
        //CmdIndex = atoi(tmp + strlen("index\":\""));
        CmdIndex = strtol(stringnum, NULL, 10);
        printf("CmdIndex : %d\n", CmdIndex);
    }
    else
    {
        printf("Error : %d\n", Err_Index);
        return Err_Index;
    }

    switch(CmdIndex)
    {
        // set Host parameter. like ssid / store id ...
        case 1:
            // check SSID
            if((tmp = strstr(buf, "SSID")) != NULL)
            {
                //printf("SSID buf : %s\nstrlen is %d\n", tmp, strlen("SSID\":\""));
                readjasonvalue(tmp + strlen("SSID\""), StaSsid);
                printf("SSID : %s\n", StaSsid);
            }
            else
            {
                return Err_Ssid;
                printf("Error : %d\n", Err_Ssid);
            }

            // check Passord
            if((tmp = strstr(buf, "PASSWORD")) != NULL)
            {
                //printf("PASSWORD buf : %s\nstrlen is %d\n", tmp, strlen("PASSWORD\":\""));
                readjasonvalue(tmp + strlen("PASSWORD\""), StaPassword);
                printf("PASSWORD : %s\n", StaPassword);
            }
            else
            {
                return Err_Password;
                printf("Error : %d\n", Err_Password);
            }

            // check encryption
            if((tmp = strstr(buf, "ENCRYPTION")) != NULL)
            {
                //printf("ENCRYPTION buf : %s\nstrlen is %d\n", tmp, strlen("ENCRYPTION\":\""));
                readjasonvalue(tmp + strlen("ENCRYPTION\""), StaEncryption);
                printf("ENCRYPTION : %s\n", StaEncryption);
            }
            else
            {
                return Err_Encrytpion;
                printf("Error : %d\n", Err_Encrytpion);
            }

            // check store Id
            if((tmp = strstr(buf, "StoreId")) != NULL)
            {
                //printf("StoreId buf : %s\nstrlen is %d\n", tmp, strlen("StoreId\":\""));
                readjasonvalue(tmp + strlen("StoreId\""), DBStoreId);
                printf("StoreId : %s\n", DBStoreId);
            }
            else
            {
                return Err_Dbstoreid;
                printf("Error : %d\n", Err_Dbstoreid);
            }
            // set Host ssid for ap mode
            sprintf(ucicommand, "uci set wireless.ap.ssid=\"DaBai_%s\"", DBStoreId);
            send_command(ucicommand, NULL, 0);

            // set Store wifi ssid to Host device
            sprintf(ucicommand, "uci set wireless.sta.ssid=\"%s\"", StaSsid);
            send_command(ucicommand, NULL, 0);

            // set Store wifi ssid to Host device
            sprintf(ucicommand, "uci set wireless.sta.key=\"%s\"", StaPassword);
            send_command(ucicommand, NULL, 0);

            // set Store wifi ssid to Host device
            sprintf(ucicommand, "uci set wireless.sta.encryption=\"%s\"", StaEncryption);
            send_command(ucicommand, NULL, 0);

            // set Host device to apsta mode
            send_command("wifi_mode apsta", NULL, 0);

            break;

        // device like phone / powerbank ... send device information to host.
        case 2:
            // cheeck commnad include device type information
            if((tmp = strstr(buf, "type")) != NULL)
            {
                readjasonvalue(tmp + strlen("type\""), CDev.DevType);
                printf("DevType : %s\n", CDev.DevType);
            }
            else
            {
                return Err_Type;
                printf("Error : %d\n", Err_Type);
            }
            if(strcmp(CDev.DevType, "android") == 0)
                TypeIdx = 1;
            else if(strcmp(CDev.DevType, "iOS") == 0)
                TypeIdx = 2;

            // check command include UserId
            if((tmp = strstr(buf, "userId")) != NULL)
            {
                readjasonvalue(tmp + strlen("userId\""), CDev.DevUserId);
                printf("DevUserId : %s\n", CDev.DevUserId);
            }
            else
            {
                return Err_Userid;
                printf("Error : %d\n", Err_Userid);
            }

            // check command include account
            if((tmp = strstr(buf, "account")) != NULL)
            {
                readjasonvalue(tmp + strlen("account\""), CDev.DevAccount);
                printf("DevAccount : %s\n", CDev.DevAccount);
            }
            else
            {
                return Err_Account;
                printf("Error : %d\n", Err_Account);
            }

            // check command include device mac address
            if((tmp = strstr(buf, "mac")) != NULL)
            {
                readjasonvalue(tmp + strlen("mac\""), CDev.DevMac);
                if(TypeIdx == 1)    // android
                {
                    squeeze(CDev.DevMac, ':');
                }
                else if(TypeIdx == 2)
                {
                    checkiOSMac(CDev.DevMac);
                }
                printf("DevMac : %s\n", CDev.DevMac);
            }
            else
            {
                return Err_Mac;
                printf("Error : %d\n", Err_Mac);
            }
            write_online_list(CDev);
            //create_device_json(CDev);
            break;
    }
    sprintf(buf2, "Receive Data is OK\n");
    //return Err_none;
    return 0;
}

int main()
{
    int fd, recctmain = 0, BTIdx = 0;
    unsigned char *rxbuf;
    unsigned char *filebuf;
    //unsigned char *commandbuf;
    //unsigned char buf2[64];
    int reg;
    //int i, reg, data_length;
    //unsigned char *jpgbuf;
    //char prebuf[8] = "StartAA";
    //unsigned char suffixbuf[5] = "AAEnd";
    time_t Current_sec;
    time_t Present_sec;
    unsigned char BTCheckFlag = 0;

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
    while(1)
    {
        BTIdx = BTTransferUart(fd, "DaBai/RxCommTmp.txt", rxbuf);
        switch(BTIdx)
        {
            case 1:     // Set Device Network Information
              BTIdx = 0;
              break;

            case 2:     // Devive Send Account and Devive information to check will be charged
              CheckCHGDevInfo("DaBai/RxCommTmp.txt", ChargeDevice, &ChargeDeviceCount, filebuf);
              BTIdx = 0;
              break;

            case 3:     // Set Bluetooth Device Name
              BTIdx= 0;
              break;

            default:
              break;
        }
    }
    return 0;
}
