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
    //struct json_object *jobj_list;
    struct json_object *jobj[CHGDEVMAX];
    struct json_object *data;

    //jobj_list = json_object_new_object();
    data = json_object_new_array();
    #if DBG_EN
        printf("into WriteChgList~~~!!\nChgDevCt is %d\n", ChgDevCt);
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
      json_object_array_add(data, jobj[i]);
      /*
      #if DBG_EN
          printf("at for loop data : %s\n", json_object_to_json_string(data));
      #endif
      */
    }
    //json_object_object_add(jobj_list, "ChgList", data);
    filebuf = json_object_to_json_string(data);
    #if DBG_EN
        printf("json file : %s\n", filebuf);
    #endif
    for(i=0;i<ChgDevCt;i++)
      json_object_put(jobj[i]);
    json_object_put(data);
    //json_object_put(jobj_list);
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
    // First check userID and Demolist's userID, if there are the same. it is a demo account.
    // if there is not the same, check it with server. if there are the same, it is a registed account.
    // otherwise it is a not faild account.
    if(CheckDemoID("DaBai/RxCommTmp.txt", filebuf) == 0)
    {
      if(CheckServerID("DaBai/RxCommTmp.txt", filebuf) == 0)
        return -1;
    }
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

int GetChgDevFromFile(unsigned char *path, struct ClientDev *CDV)
{
  unsigned char *Onlinebuf;
  unsigned char *onlinestr[4];
  FILE *fp_online;
  struct json_object *jobj_online, *jobj_value;
  int arraylen = 0, i;

  Onlinebuf = (unsigned char *)calloc(FILESIZE, sizeof(unsigned char));
  for(i=0;i<4;i++)
  {
    onlinestr[i] = (unsigned char *)calloc(FILESIZE, sizeof(unsigned char));
  }
  fp_online = fopen(path, "r");
  if(fp_online != NULL)
  {
    while(!feof(fp_online))
    {
      fread(Onlinebuf, 1, FILESIZE, fp_online);
    }
    fclose(fp_online);
  }
  else
  {
    // not onlinechglist file, return 0~
    return 0;
  }
  printf("onlinebuf is %s\n", Onlinebuf);
  jobj_online = json_tokener_parse(Onlinebuf);
  arraylen = json_object_array_length(jobj_online);
  printf("arraylen is %d\n", arraylen);
  if(arraylen > 0)
  {
    for(i=0;i<arraylen;i++)
    {
      jobj_value = json_object_array_get_idx(jobj_online, i);
      (CDV+i)->DevIdx = json_object_get_int(json_object_object_get(jobj_value, "index"));
      sprintf((CDV+i)->DevType, "%s", json_object_get_string(json_object_object_get(jobj_value, "type")));
      sprintf((CDV+i)->DevUserId, "%s", json_object_get_string(json_object_object_get(jobj_value, "userId")));
      sprintf((CDV+i)->DevAccount, "%s", json_object_get_string(json_object_object_get(jobj_value, "account")));
      sprintf((CDV+i)->DevMac, "%s", json_object_get_string(json_object_object_get(jobj_value, "mac")));
      (CDV+i)->StartTime = json_object_get_int(json_object_object_get(jobj_value, "StartTime"));
      (CDV+i)->CurrentTime = json_object_get_int(json_object_object_get(jobj_value, "CurrentTime"));
      sprintf((CDV+i)->DevFormatSTime, "%s", json_object_get_string(json_object_object_get(jobj_value, "FormatSTime")));
      sprintf((CDV+i)->DevFormatCTime, "%s", json_object_get_string(json_object_object_get(jobj_value, "FormatCTime")));
      (CDV+i)->RemainT_Hr = json_object_get_int(json_object_object_get(jobj_value, "RemainT_Hr"));
      (CDV+i)->RemainT_Min = json_object_get_int(json_object_object_get(jobj_value, "RemainT_Min"));
      (CDV+i)->ChgMode = json_object_get_int(json_object_object_get(jobj_value, "ChgMode"));
      onlinestr[i] = json_object_get_string(jobj_value);
      #if DBG_EN
          printf("list[%d] is %s\n", i, onlinestr[i]);
      #endif
    }
    return arraylen;
  }
  else
  {
    // not anymore charge device in the list file!
    return 0;
  }
}

int CheckDemoID(unsigned char *path, unsigned char *filebuf)
{
    FILE *fp;
    struct json_object *jobj_Rx, *jobj_Demo, *jobj_id, *jobj_value;
    unsigned char *RxIdbuf, *DemoIdbuf;
    int arraylen, i;

    //Demobuf = (unsigned char *)calloc(FILESIZE, sizeof(unsigned char));
    RxIdbuf = (unsigned char *)calloc(64, sizeof(unsigned char));
    DemoIdbuf = (unsigned char *)calloc(64, sizeof(unsigned char));
    memset(filebuf, 0, FILESIZE);
    fp = fopen(path, "r");
    if(fp != NULL)
    {
      while(!feof(fp))
      {
        fread(filebuf, 1, FILESIZE, fp);
      }
      fclose(fp);
    }
    jobj_Rx = json_tokener_parse(filebuf);
    RxIdbuf = json_object_get_string(json_object_object_get(jobj_Rx, "userId"));

    memset(filebuf, 0, FILESIZE);
    fp = fopen(DEMOLISTPATH, "r");
    if(fp != NULL)
    {
      while(!feof(fp))
      {
        fread(filebuf, 1, FILESIZE, fp);
      }
      fclose(fp);
    }
    else
    {
      return 0;
    }
    jobj_Demo = json_tokener_parse(filebuf);
    arraylen = json_object_array_length(jobj_Demo);
    #if DBG_EN
        printf("arraylen is %d\n", arraylen);
    #endif
    for(i=0;i<arraylen;i++)
    {
      jobj_value = json_object_array_get_idx(jobj_Demo, i);
      DemoIdbuf = json_object_get_string(jobj_value);
      #if DBG_EN
          printf("DemoIdbuf is %s, RxIdbuf is %s\n", DemoIdbuf, RxIdbuf);
      #endif
      if(strcmp(RxIdbuf, DemoIdbuf) == 0)
      {
        printf("There will be set wireless enable~~!!");
        return 1;
      }
    }
    free(RxIdbuf);
    free(DemoIdbuf);
    return 0;
}

int CheckServerID(unsigned char *path, unsigned char *filebuf)
{
  return 0;
}

int UpdateChgDevice(struct ClientDev *CDV, struct ClientDev *Tmp, unsigned char *filebuf)
{
  int i,j=0;
  time_t cur_time;

  for(i=0;i<CHGDEVMAX;i++)
  {
    time(&cur_time);
    if((cur_time - (CDV+i)->CurrentTime) < 10)
    {
      (Tmp+j)->DevIdx = (CDV+i)->DevIdx;
      sprintf((Tmp+j)->DevType, "%s", (CDV+i)->DevType);
      sprintf((Tmp+j)->DevUserId, "%s", (CDV+i)->DevUserId);
      sprintf((Tmp+j)->DevAccount, "%s", (CDV+i)->DevAccount);
      sprintf((Tmp+j)->DevMac, "%s", (CDV+i)->DevMac);
      (Tmp+j)->StartTime = (CDV+i)->StartTime;
      (Tmp+j)->CurrentTime = (CDV+i)->CurrentTime;
      sprintf((Tmp+j)->DevFormatSTime, "%s", (CDV+i)->DevFormatSTime);
      sprintf((Tmp+j)->DevFormatCTime, "%s", (CDV+i)->DevFormatCTime);
      (Tmp+j)->RemainT_Hr = (CDV+i)->RemainT_Hr;
      (Tmp+j)->RemainT_Min = (CDV+i)->RemainT_Min;
      (Tmp+j)->ChgMode = (CDV+i)->ChgMode;
      j++;
    }
  }
  for(i=0;i<CHGDEVMAX;i++)
    memset(CDV+i, 0, sizeof(struct ClientDev));
  for(i=0;i<j;i++)
  {
    (CDV+i)->DevIdx = i;
    sprintf((CDV+i)->DevType, "%s", (Tmp+i)->DevType);
    sprintf((CDV+i)->DevUserId, "%s", (Tmp+i)->DevUserId);
    sprintf((CDV+i)->DevAccount, "%s", (Tmp+i)->DevAccount);
    sprintf((CDV+i)->DevMac, "%s", (Tmp+i)->DevMac);
    (CDV+i)->StartTime = (Tmp+i)->StartTime;
    (CDV+i)->CurrentTime = (Tmp+i)->CurrentTime;
    sprintf((CDV+i)->DevFormatSTime, "%s", (Tmp+i)->DevFormatSTime);
    sprintf((CDV+i)->DevFormatCTime, "%s", (Tmp+i)->DevFormatCTime);
    (CDV+i)->RemainT_Hr = (Tmp+i)->RemainT_Hr;
    (CDV+i)->RemainT_Min = (Tmp+i)->RemainT_Min;
    (CDV+i)->ChgMode = (Tmp+i)->ChgMode;
  }
  for(i=0;i<CHGDEVMAX;i++)
    memset(Tmp+i, 0, sizeof(struct ClientDev));
  #if DBG_EN
      printf("After update CHG List, CHG count is %d(j)\n", j);
  #endif
  WriteChgList(CHGLISTPATH, filebuf, CDV, j);
  return j;
}
