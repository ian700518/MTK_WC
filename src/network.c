#include "dabai.h"

int GetDeviceMACAddr(unsigned char *path, unsigned char *filebuf)
{
  unsigned char Strbuf[32];
  unsigned int offset = 0;
  unsigned char macaddr[18];
  FILE *fp;
  struct json_object *json_file;
  unsigned char *EthAddrStr;

  memset(filebuf, 0, FILESIZE);
  memset(macaddr, 0, 18);
  fp = fopen("/etc/config/network", "r");
  if(fp != NULL)
  {
    sprintf(Strbuf, "option macaddr \'");
    while(!feof(fp))
    {
      fgets(filebuf, FILESIZE, fp);
      if(strstr(filebuf, Strbuf) != NULL)
      {
        filebuf = strchr(filebuf, '\'');
        strncpy(macaddr, filebuf + 1, 17);
        DBGNET("Network MAC Address is : %s\n", macaddr);
        break;
      }
    }
    fclose(fp);
  }
  json_file = json_object_from_file(path);
  DBGNET("josn file is %s\n", json_object_to_json_string(json_file));
  EthAddrStr = (unsigned char *)calloc(128, sizeof(unsigned char));

  sprintf(EthAddrStr, "%s", macaddr);
  json_object_object_del(json_file, "Network MAC Address");
  json_object_object_add(json_file, "Network MAC Address", json_object_new_string(EthAddrStr));
  json_object_to_file(path, json_file);
  return 0;
}
