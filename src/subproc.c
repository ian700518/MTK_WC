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

int WriteChgList(unsigned char *path, unsigned char *filebuf, struct ClientDev *CDV, unsigned char ChgDevCt)
{
    FILE *fp;
    int i, length = 0;
    struct json_object *jobj[CHGDEVMAX];
    struct json_object *data;

    data = json_object_new_array();
    #if DBG_EN
        printf("into WriteChgList~~~!!\n");
    #endif
    for(i=0; i<ChgDevCt; i++)
    {

      jobj[i] = json_object_new_object();
      json_object_object_add(jobj[i], "index", json_object_new_int(CDV[i].DevIdx));
      json_object_object_add(jobj[i], "type", json_object_new_string((CDV+i)->DevType));
      json_object_object_add(jobj[i], "userId", json_object_new_string((CDV+i)->DevUserId));
      json_object_object_add(jobj[i], "account", json_object_new_string((CDV+i)->DevAccount));
      json_object_object_add(jobj[i], "mac", json_object_new_string((CDV+i)->DevMac));
      json_object_object_add(jobj[i], "StartTime", json_object_new_int(CDV[i].StartTime));
      json_object_object_add(jobj[i], "CurrentTime", json_object_new_int(CDV[i].CurrentTime));
      json_object_object_add(jobj[i], "FormatSTime", json_object_new_string((CDV+i)->DevFormatSTime));
      json_object_object_add(jobj[i], "FormatCTime", json_object_new_string((CDV+i)->DevFormatCTime));
      json_object_object_add(jobj[i], "RemainT_Hr", json_object_new_int(CDV[i].RemainT_Hr));
      json_object_object_add(jobj[i], "RemainT_Min", json_object_new_int(CDV[i].RemainT_Min));
      json_object_object_add(jobj[i], "ChgMode", json_object_new_int(CDV[i].ChgMode));
      #if DBG_EN
          printf("at for loop jobj : %s\nat for loop data : %s\n", json_object_to_json_string(jobj[i]), json_object_to_json_string(data));
      #endif
      json_object_array_add(data, jobj[i]);
    }
    //jobj = json_tokener_parse(filebuf);
    filebuf = json_object_to_json_string(data);
    #if DBG_EN
        printf("json file : %s\n", filebuf);
    #endif
    for(i=0;i<ChgDevCt;i++)
      json_object_put(jobj[i]);
    json_object_put(data);
    fp = fopen(path, "w+");
    if(fp != NULL)
    {
      fwrite(filebuf, strlen(filebuf), 1, fp);
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
    unsigned char *checkbuf;
    //unsigned char Strbuf[64];
    unsigned char *Macbuf;
    int i;
    time_t cur_time;
    struct tm *info;
    struct json_object *jobj;
    FILE *fp_check;

    memset(filebuf, 0, FILESIZE);
    checkbuf = (unsigned char *)calloc(128, sizeof(unsigned char));
    Macbuf = (unsigned char *)calloc(64, sizeof(unsigned char));
    fp_check = fopen(path, "r");
    if(fp_check != NULL)
    {
      while(!feof(fp_check))
      {
        fread(filebuf, 1, FILESIZE, fp_check);
      }
      fclose(fp_check);
    }
    jobj = json_tokener_parse(filebuf);
    checkbuf = json_object_get_string(json_object_object_get(jobj, "userId"));
    #if DBG_EN
        printf("filebuf to jobj string %s\n", json_object_to_json_string(jobj));
        printf("json-c checkbuf : %d\n", checkbuf);
    #endif
    time(&cur_time);
    info = localtime(&cur_time);
    for(i=0;i<(*ChgDevCt);i++)
    {
        if(strcmp(checkbuf, (CDV+i)->DevUserId) == 0)
        {
            #if DBG_EN
                printf("CDV->DevUserId is : %s\n", (CDV+i)->DevUserId);
            #endif
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
        sprintf((CDV+i)->DevType, "%s", json_object_get_string(json_object_object_get(jobj, "type")));
        sprintf((CDV+i)->DevUserId, "%s", json_object_get_string(json_object_object_get(jobj, "userId")));
        sprintf((CDV+i)->DevAccount, "%s", json_object_get_string(json_object_object_get(jobj, "account")));
        if(strcmp((CDV+i)->DevType, "iOS") == 0)
        {
          Macbuf = json_object_get_string(json_object_object_get(jobj, "mac"));
          #if DBG_EN
              printf("Macbuf is %s\n", Macbuf);
          #endif
          checkiOSMac(Macbuf, (CDV+i)->DevMac);
        }
        else if(strcmp((CDV+i)->DevType, "android") == 0)
        {
          sprintf((CDV+i)->DevMac, "%s", json_object_get_string(json_object_object_get(jobj, "mac")));
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
    json_object_put(jobj);      // if new json object, After used must be free json object
    WriteChgList(CHGLISTPATH, filebuf, CDV, *ChgDevCt);
}
