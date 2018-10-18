// add test program by ian at 2018-04-25
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>      /*檔控制定義*/
#include <termios.h>    /*PPSIX 終端控制定義*/
#include <sys/time.h>
#include "dabai.h"

#define DBG_EN 0

int uart_initial(char *dev, int baudrate, int bits, int parity, int stopbits)
{
    int fd;
    unsigned char membuf[64];
    int reg;

    fd = open_uart(dev);
    if(fd > 0)
    {
        set_speed(fd, baudrate);
        set_Parity(fd, bits, stopbits, parity);

        // count 115200
        // 40000000 / 115200 = 437 --> (12 * 28)
        // sample = 28
        // {DLM:DLL} = 12

        // count 57600
        // 40000000 / 57600 = 694 --> (24 * 28)
        // sample = 28
        // {DLM:DLL} = 24

        // set 0x10000c0c bit7 to 1
        send_command("devmem 0x10000c0c", membuf, sizeof(membuf));
        #if DBG_EN
          printf("SET DLAB to 1 0x10000c0c value is %s\n", membuf);
        #endif
        reg = strtoul(membuf + 2, NULL, 16);
        reg |= 0x00000080;
        #if DBG_EN
          printf("SET DLAB to 1 0x10000c0c value is 0x%x\n", reg);
        #endif
        sprintf(membuf, "devmem 0x10000c0c 32 0x%08x", reg);
        send_command(membuf, NULL, 0);

        // write value 12 to DLL register
        send_command("devmem 0x10000c00 32 0x0000000c", NULL, 0);

        // set 0x10000c0c bit7 to 0
        send_command("devmem 0x10000c0c", membuf, sizeof(membuf));
        reg = strtoul(membuf + 2, NULL, 16);
        reg &= 0xFFFFFF7F;
        #if DBG_EN
          printf("SET DLAB to 0 0x10000c0c value is 0x%x\n", reg);
        #endif
        sprintf(membuf, "devmem 0x10000c0c 32 0x%08x", reg);
        send_command(membuf, NULL, 0);

        // write value 28 to sample count register
        send_command("devmem 0x10000c28 32 0x0000001c", NULL, 0);
        // write value 14 to sample point register (sapmle point is half of sample count)
        if(BTEEPROM_MODE == 1)
          send_command("devmem 0x10000c2c 32 0x0000000e", NULL, 0);         // baudrate 115200
        else
          send_command("devmem 0x10000c2c 32 0x00000018", NULL, 0);         // baudrate 57600
        return fd;
    }
    return -1;
}

int open_uart(char *dev)
{
    int fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd)
        return fd;
    else
    {
        perror("Can't Open Serial Port\n");
        return -1;
    }
}

/**
*@brief  設置串口通信速率
*@param  fd     類型 int  打開串口的文件控制碼
*@param  speed  類型 int  串口速度
*@return  void
*/
int speed_arr[] = {B1000000, B921600, B576000, B460800, B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200};
int name_arr[] = {1000000, 921600, 576000, 460800, 230400, 115200, 57600, 38400,  19200,  9600,  4800,  2400,  1200};
int set_speed(int fd, int speed)
{
    int i;
    int status;
    struct termios Opt;
    tcgetattr(fd, &Opt);
    for(i= 0;i < (sizeof(speed_arr) / sizeof(int)); i++)
    {
         if(speed == name_arr[i])
         {
             tcflush(fd, TCIOFLUSH);
             cfsetispeed(&Opt, speed_arr[i]);
             cfsetospeed(&Opt, speed_arr[i]);
             status = tcsetattr(fd, TCSANOW, &Opt);
             if(status != 0)
             {
                      perror("tcsetattr fd1");
                      return -1;
             }
             tcflush(fd,TCIOFLUSH);
         }
    }
    return 0;
}

/**
*@brief   設置串口資料位元，停止位元和效驗位
*@param  fd     類型  int  打開的串口文件控制碼
*@param  databits 類型  int 資料位元   取值 為 7 或者8
*@param  stopbits 類型  int 停止位   取值為 1 或者2
*@param  parity  類型  int  效驗類型 取值為N,E,O,,S
*/
int set_Parity(int fd, int databits, int stopbits, int parity)
{
        struct termios options;
        if(tcgetattr(fd,&options) != 0)
        {
            perror("SetupSerial 1");
            return -1;
        }
        options.c_cflag &= ~CSIZE;
        switch(databits) /*設置數據位元數*/
        {
            case 7:
                options.c_cflag |= CS7;
                break;
            case 8:
                options.c_cflag |= CS8;
                break;
            default:
                fprintf(stderr,"Unsupported data size\n");
                return -1;
        }
        switch(parity)
        {
            case 'n':
            case 'N':
                options.c_cflag &= ~PARENB;   /* Clear parity enable */
                options.c_iflag &= ~INPCK;     /* Enable parity checking */
                break;
            case 'o':
            case 'O':
                options.c_cflag |= (PARODD | PARENB); /* 設置為奇效驗*/
                options.c_iflag |= INPCK;             /* Disnable parity checking */
                break;
            case 'e':
            case 'E':
                options.c_cflag |= PARENB;     /* Enable parity */
                options.c_cflag &= ~PARODD;   /* 轉換為偶效驗*/
                options.c_iflag |= INPCK;       /* Disnable parity checking */
                break;
            case 'S':
            case 's':  /*as no parity*/
                options.c_cflag &= ~PARENB;
                options.c_cflag &= ~CSTOPB;break;
            default:
                     fprintf(stderr,"Unsupported parity\n");
                     return -1;
        }
        /* 設置停止位*/
        switch(stopbits)
        {
            case 1:
                options.c_cflag &= ~CSTOPB;
                break;
            case 2:
                options.c_cflag |= CSTOPB;
                break;
            default:
                fprintf(stderr,"Unsupported stop bits\n");
                return -1;
        }
        /* Set input parity option */
        if(parity != 'n')
            options.c_iflag |= INPCK;
        options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
        options.c_oflag  &= ~OPOST;   /*Output*/
        tcflush(fd,TCIFLUSH);
        options.c_cc[VTIME] = 0; /* 設置超時15 seconds*/
        options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
        if(tcsetattr(fd,TCSANOW,&options) != 0)
        {
            perror("SetupSerial 3");
            return -1;
        }
        return 0;
}

int uart_read(int fd, unsigned char *buf)
{
    struct timeval timeout;
    fd_set readfd;
    int readct = 0, nread = 0, ret = 0, i = 0;

    while(1)
    {
        FD_ZERO(&readfd);
        FD_SET(fd, &readfd);
        timeout.tv_sec = 0;
        timeout.tv_usec = 30000;
        ret = select(fd + 1, &readfd, NULL, NULL, &timeout);
        if(ret < 0)
        {
            printf("select function error\n");
            ret = -1;
            break;
        }
        else if(ret == 0)
        {
            if(readct > 0)
            {
                #if DBG_EN
                    for(i=0;i<readct;i++)
                      printf("Read buf[%d] = 0x%02x\n", i, buf[i]);
                    printf("tty receive data finish!!!\n");
                #endif
                ret = readct;
                break;
            }
            else
            {
                ret = -1;
                break;
            }
        }
        else
        {
            if(FD_ISSET(fd, &readfd))
            {
                while((nread = read(fd, buf + readct, UARTRXSIZE)) > 0)
                {
                    readct += nread;
                }
            }
        }
    }
    return ret;
}

int uart_write(int fd, unsigned char *buf, unsigned long length)
{
    int nwrite;
    unsigned long i;

    #if DBG_EN
      for(i = 0; i < length; i++)
        printf("Write buf[%d] = 0x%02x\n", i, buf[i]);
    #endif
    nwrite = write(fd, buf, length);
    //printf("uart write buf : %s length : %d\n", buf, length);
    usleep(length * RW_BYTETIME);
    if(nwrite == length)
        return nwrite;
    return -1;
}
