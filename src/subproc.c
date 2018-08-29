#include "dabai.h"

#define DBG_EN 0

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


/*check iOS system MAC address*/
void checkiOSMac(unsigned char *src, unsigned char *dest)
{
    int length;
    int i,j;
    unsigned char *buf;

    buf = strrchr(src, '-');
    printf("buf is %s\n", buf);
    for(i=0,j=0;i<12;i++,j++)
    {
        dest[j] = buf[i+1];
        if(i%2 == 1)
        {
            j++;
            if(i<11)
              dest[j] = ':';
        }
    }
    dest[j] = '\0';
}


void send_command(unsigned char *command, unsigned char *resulte, int resulte_length)
{
    FILE *fp;

    fp = popen(command, "r");
    if(resulte != NULL)
    {
        fgets(resulte, resulte_length, fp);
    }
    pclose(fp);
}

void ReadJsonVal(unsigned char *src, unsigned char *des)
{
    int i = 0;

    // check first " symbol
    while(src[i] != '\"')
    {
        des[i] = src[i];
        i++;
    }
    des[i] = '\0';
}

int RXCmdtoCHGDev(unsigned char *path, unsigned char *string, unsigned char *filebuf, unsigned char *checkbuf)
{
    FILE *fp;
    unsigned int length;

    memset(filebuf, 0, FILESIZE);
    fp = fopen(path, "r");
    if(fp != NULL)
    {
        while(!feof(fp))
        {
            fread(filebuf, 1, FILESIZE, fp);
            filebuf = strstr(filebuf, string);
            length = strlen(string);
            #if DBG_EN
                printf("string is %s\n", string);
                printf("filebuf is %s\n", filebuf);
            #endif
            if(filebuf != NULL)
            {
                ReadJsonVal(filebuf + length, checkbuf);
                break;
            }
        }
        #if DBG_EN
            printf("checkbuf is %s\n", checkbuf);
        #endif
        fclose(fp);
        return 1;
    }
    return 0;
}

int WriteChgList(unsigned char *path, unsigned char *filebuf, struct ClientDev *CDV, unsigned char ChgDevCt)
{
    FILE *fp;
    int i;

    fp = fopen(path, "w+");
    if(fp != NULL)
    {
        fputs("[\n", fp);
        for(i=0;i<ChgDevCt;i++)
        {
            fputs(" {\n", fp);
            sprintf(filebuf, "    \"index\":\"%d\",\n", (CDV+i)->DevIdx);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"type\":\"%s\",\n", (CDV+i)->DevType);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"userId\":\"%s\",\n", (CDV+i)->DevUserId);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"account\":\"%s\",\n", (CDV+i)->DevAccount);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"mac\":\"%s\",\n", (CDV+i)->DevMac);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"StartTime\":\"%d\",\n", (CDV+i)->StartTime);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"CurrentTime\":\"%d\",\n", (CDV+i)->CurrentTime);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"FormatSTime\":\"%s\",\n", (CDV+i)->DevFormatSTime);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"FormatCTime\":\"%s\",\n", (CDV+i)->DevFormatCTime);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"RemainT_Hr\":\"\",\n");
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"RemainT_Min\":\"\",\n");
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"ChgMode\":\"\"\n");
            fputs(filebuf, fp);
            if((ChgDevCt - i) == 1)
              fputs(" }\n", fp);
            else
              fputs("},\n", fp);
        }
        fputs("]\n", fp);
        fclose(fp);
    }
}

/*
  check account or uid from rx pair command.
    if account or uid has in Clien Device struct
      over write Client Device struct
    else
      add into client device struct
*/
int CheckCHGDevInfo(unsigned char *path, struct ClientDev *CDV, unsigned char *ChgDevCt, unsigned char *filebuf)
{
    unsigned char checkbuf[128];
    unsigned char Strbuf[64];
    unsigned char Macbuf[64];
    int i;
    time_t cur_time;
    struct tm *info;

    memset(filebuf, 0, FILESIZE);
    sprintf(Strbuf, "\"userId\":\"");
    RXCmdtoCHGDev(path, Strbuf, filebuf, checkbuf);
    time(&cur_time);
    info = localtime(&cur_time);
    for(i=0;i<(*ChgDevCt);i++)
    {
        if(strcmp(checkbuf, CDV->DevUserId) == 0)
        {
            printf("CDV->DevUserId is : %s\n", CDV->DevUserId);
            break;    // if the same userid in the charge list
        }
    }
    #if DBG_EN
        printf("Before ChargeDeviceCount is %d\n", ChargeDeviceCount);
        printf("Before *ChgDevCt is %d\n", *ChgDevCt);
    #endif
    if(i == (*ChgDevCt))    // no device mach within charge list or charge list is no device, and add the information into charge list
    {
        (CDV+i)->DevIdx = *ChgDevCt;
        RXCmdtoCHGDev(path, "\"type\":\"", filebuf, (CDV+i)->DevType);
        RXCmdtoCHGDev(path, "\"userId\":\"", filebuf, (CDV+i)->DevUserId);
        RXCmdtoCHGDev(path, "\"account\":\"", filebuf, (CDV+i)->DevAccount);
        if(strcmp((CDV+i)->DevType, "iOS") == 0)
        {
            RXCmdtoCHGDev(path, "\"mac\":\"", filebuf, Macbuf);
            #if DBG_EN
                printf("Macbuf is %s\n", Macbuf);
            #endif
            checkiOSMac(Macbuf, (CDV+i)->DevMac);
        }
        else if(strcmp((CDV+i)->DevType, "android") == 0)
        {
            RXCmdtoCHGDev(path, "\"mac\":\"", filebuf, (CDV+i)->DevMac);
        }
        (CDV+i)->StartTime = cur_time;
        (CDV+i)->CurrentTime = cur_time;
        strftime((CDV+i)->DevFormatSTime, 64, "%x - %X", info);
        strftime((CDV+i)->DevFormatCTime, 64, "%x - %X", info);
        #if DBG_EN
            printf("(CDV+%d)->DevIdx is : %d\n", i, (CDV+i)->DevIdx);
            printf("(CDV+%d)->DevType is : %s\n", i, (CDV+i)->DevType);
            printf("(CDV+%d)->DevUserId is : %s\n", i, (CDV+i)->DevUserId);
            printf("(CDV+%d)->DevAccount is : %s\n", i, (CDV+i)->DevAccount);
            printf("(CDV+%d)->DevMac is : %s\n", i, (CDV+i)->DevMac);
            printf("(CDV+%d)->StartTime is : %d\n", i ,(CDV+i)->StartTime);
            printf("(CDV+%d)->CurrentTime is : %d\n", i , (CDV+i)->CurrentTime);
            printf("(CDV+%d)->DevFormatSTime is : %s\n", i, (CDV+i)->DevFormatSTime);
            printf("(CDV+%d)->DevFormatCTime is : %s\n", i, (CDV+i)->DevFormatCTime);
        #endif
        if(*ChgDevCt < CHGDEVMAX)
            (*ChgDevCt)++;
    }
    else    // if device mach within charge list, update Time.
    {
        (CDV+i)->CurrentTime = cur_time;
        strftime((CDV+i)->DevFormatCTime, 64, "%x - %X", info);
    }
    #if DBG_EN
        printf("After ChargeDeviceCount is %d\n", ChargeDeviceCount);
        printf("After *ChgDevCt is %d\n", *ChgDevCt);
    #endif
    WriteChgList(CHGLISTPATH, filebuf, CDV, *ChgDevCt);
}
