#include "dabai.h"

#define DBG_EN 1

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
void checkiOSMac(unsigned char *src)
{
    int length;
    int i,j;

    length = strlen(src);
    for(i=12,j=0;i>0;i--,j++)
    {
        src[j] = src[length - i];
        if(((j%2) == 0) && (j != 0))
          src[++j] = ':';
    }
    src[j] = '\0';
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

int WriteJsonVal(unsigned char *path, unsigned char *string, unsigned char *filebuf)
{

}

int RXCmdtoCHGDev(unsigned char *path, unsigned char *string, unsigned char *filebuf, unsigned char *checkbuf)
{
    FILE *fp;
    unsigned int length;

    memset(filebuf, 0, strlen(filebuf));
    fp = fopen(path, "r");
    if(fp != NULL)
    {
        while(!feof(fp))
        {
            fgets(filebuf, FILESIZE, fp);
            filebuf = strstr(filebuf, string);
            length = strlen(string);
            #if DBG_EN
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
            sprintf(filebuf, "    \"index\":\"%d\",\n", CDV->DevIdx);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"type\":\"%s\",\n", CDV->DevType);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"userId\":\"%s\",\n", CDV->DevUserId);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"account\":\"%s\",\n", CDV->DevAccount);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"mac\":\"%s\",\n", CDV->DevMac);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"StartTime\":\"%d\",\n", CDV->StartTime);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"CurrentTime\":\"%d\",\n", CDV->CurrentTime);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"FormatSTime\":\"%s\",\n", CDV->DevFormatSTime);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"FormatCTime\":\"%s\",\n", CDV->DevFormatCTime);
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"RemainT_Hr\":\"\",\n");
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"RemainT_Min\":\"\",\n");
            fputs(filebuf, fp);
            sprintf(filebuf, "    \"ChgMode\":\"\"\n");
            fputs(filebuf, fp);
            fputs(" },\n", fp);
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
    int i;
    time_t cur_time;
    struct tm *info;

    memset(filebuf, 0, strlen(filebuf));
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
        printf("Before ChargeDeviceCount is %d\n", *ChgDevCt);
    #endif
    if(i == (*ChgDevCt))    // no device mach within charge list or charge list is no device, and add the information into charge list
    {
        (CDV+i)->DevIdx = *ChgDevCt;
        RXCmdtoCHGDev(path, "\"type\":\"", filebuf, (CDV+i)->DevType);
        RXCmdtoCHGDev(path, "\"userId\":\"", filebuf, (CDV+i)->DevUserId);
        RXCmdtoCHGDev(path, "\"account\":\"", filebuf, (CDV+i)->DevAccount);
        RXCmdtoCHGDev(path, "\"mac\":\"", filebuf, (CDV+i)->DevMac);
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
            *ChgDevCt++;
    }
    else    // if device mach within charge list, update Time.
    {
        (CDV+i)->CurrentTime = cur_time;
        strftime((CDV+i)->DevFormatCTime, 64, "%x - %X", info);
    }
    #if DBG_EN
        printf("After ChargeDeviceCount is %d\n", *ChgDevCt);
    #endif
    WriteChgList(CHGLISTPATH, filebuf, CDV, *ChgDevCt);
}
