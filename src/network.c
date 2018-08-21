#include "dabai.h"

#define DBG_EN 1

int GetDeviceMACAddr(unsigned char *path, unsigned char *filebuf)
{
    unsigned char Strbuf[32];
    unsigned int offset = 0;
    unsigned char macaddr[17];
    FILE *fp;

    memset(filebuf, 0, FILESIZE);
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
                #if DBG_EN
                    printf("Network MAC Address is : %s\n", macaddr);
                #endif
                break;
            }
        }
        fclose(fp);
    }

    fp = fopen(path, "r+");
    if(fp != NULL)
    {
        sprintf(Strbuf, "  \"Network MAC Address\"");
        while(!feof(fp))
        {
            fgets(filebuf, FILESIZE, fp);
            #if DBG_EN
                printf("fgets filebuf : %s\n", filebuf);
            #endif
            if(strstr(filebuf, Strbuf) != NULL)
            {
              offset = ftell(fp) - strlen(filebuf);
              //memset(filebuf, 0, FILESIZE);
              sprintf(filebuf, "  \"Network MAC Address\" : \"%s\",\n", macaddr);
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
                printf("fputs filebuf : %s\n filebuf length : %d\n", filebuf, strlen(filebuf));
            #endif
            fclose(fp);
            return 1;
        }
    }
    return 0;
}
