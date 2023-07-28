

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

/*
    create_listen_sockct:用于TCP服务端，创建一个“监听套接字”
    @ip: 服务器的ip地址
    @port:端口号
    返回值：
        成功，返回一个监听套接字
        失败，返回-1
*/
int create_listen_sockct(const char* ip,  short port)
{
    int sock;
    int ret;
    //1.创建一个套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket error");
        return -1;
    }

    //2.

    int on = 1;
    ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,  (const void*) &on, sizeof(on));
    if (ret == -1)
    {
        perror("failed to setsockopt");
        return -1;
    }

    //设置选项，允许重用端口
    on = 1;
    ret = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,  (const void*) &on, sizeof(on));
    if (ret == -1)
    {
        perror("failed to setsockopt");
        return -1;
    }


    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(ip);

    ret = bind(sock, (struct sockaddr*)&sa, sizeof(sa));
    if (ret == -1)
    {
        perror("bind error");
        return -1;
    }


    //3.
    listen(sock, 10);


    return sock;


}


/*
    connect_tcp_server: 创建一个TCP的套接字，并去连接服务器
    @serv_ip: tcp服务器的ip地址
    @port: tcp服务器的端口号
    返回值：
        成功，返回连接套接字
        失败 返回-1
*/
int connect_tcp_server(char* serv_ip, short port)
{
    int sock;
    int ret;
    //1.创建一个套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket error");
        return -1;
    }


    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(serv_ip);

    ret = connect(sock, (struct sockaddr*)&sa, sizeof(sa));
    if (ret == -1)
    {
        perror("failed to connect");
        return -1;
    }

    return sock;
}



/*
    get_filenames: 获取一个目录下面所有文件的文件名
    @dir: 目录名
    返回值：
        该目录下面所有文件(包括目录)文件名(以空格分开)字符串。
*/
char* get_filenames(char* dir)
{
        //strcpy(p, "1.c 2.txt 3.bmp");

    DIR *dp;//指向一个打开的目录的DIR结构体。

    dp = opendir(dir);
    if (dp == NULL)
    {
        perror("opendir error");
        return NULL;
    }

    char *p =(char *)malloc(4096);
    struct  dirent *dirp;
    int r = 0;
    while (dirp = readdir(dp))
    {
        //printf("%s/%s\n", argv[1], dirp->d_name);
        if((strcmp(dirp->d_name,".")) == 0 ||
            (strcmp(dirp->d_name,"..")) == 0)
            continue;

        r += sprintf(p + r, "%s ", dirp->d_name);
    }   


    closedir(dp);

    return p;
}



/*
    send_data: 发送一个完整的数据包(命令回复)
        （1） 加上帧头(0xc0) 加上帧尾(0xc0)
         (2) 转义
                0xc0 ---> 0xdb 0xdc
                0xdb ---> 0xdb 0xdd
    @conn: 连接套接字
    @raw_data: 原始的数据包(不包括帧头和帧尾，中间还可能有0xc0需要转义)
    @len: 原始的回复数据的长度
    返回值：
        成功返回实际发送的字节数
        失败返回-1
*/
int send_data(int conn,unsigned char* raw_data, int len)
{
    int i,j;
    int ret;
    int send_len = 2 + 2*len;
    unsigned char* send_dat= (unsigned char*) malloc(send_len);

    i = 0;
    send_dat[i++] = 0xc0;
    for (j = 0; j < len; j++)
    {
        if (raw_data[j] == 0xc0)
        {
            send_dat[i++] = 0xdb;
            send_dat[i++] = 0xdc;
        }
        else if (raw_data[j] == 0xdb)
        {
            send_dat[i++] = 0xdb;
            send_dat[i++] = 0xdd;
        }
        else 
        {
            send_dat[i++] = raw_data[j];
        }
    }
    send_dat[i++] = 0xc0;


    // for debug
    printf("send data: ");
    for (j = 0; j < i; j++)
    {
        printf("%02x ", send_dat[j]);
    }
    printf("\n");


    ret = send(conn, send_dat, i, 0);
    if (ret == -1)
    {
        perror("send error");
    }


    free(send_dat);

    return ret;
}


/*
    read_raw_data: 从套接字中，接收一个完整的 命令或数据包 
            0xc0 ......... 0xc0
            完整的命令或数据包：
                (1) 去掉帧头(0xc0) 去掉帧尾(0xc0)
                (2) 转义
                    0xdb 0xdc  ---> 0xc0
                    0xdb 0xdd  ---> 0xdb
    @sock: 套接字描述符
    @data: 指向的空间，用来保存获取的原始的数据或命令包
    @max_len: data指向的空间最多能容纳多少个字节
    返回值：
        成功 返回实际接收到的命令或数据包的字节数
        失败 返回-1;
*/
int read_raw_data(int sock,unsigned char* data, int max_len)
{
    int i,j,k;
    unsigned char ch;

    unsigned char * p = (unsigned char*) malloc( 8192);

    //1. 接受一个完整的 数据包
    do
    {
        read(sock, &ch, 1);
    } while (ch != 0xc0);
    
    while (ch == 0xc0)
    {
        read(sock, &ch, 1);
    }

    i = 0;
    while (ch != 0xc0)
    {
        p[i++] = ch;
        read(sock,&ch, 1);
    }

    //2. 转义:
    //   0xdb 0xdc ---> 0xc0
    //   0xdb 0xdd ---> 0xdb
    for (j = 0, k = 0; j < i; j++)
    {
        if (p[j] == 0xdb && j != i-1)
        {
            if (p[j+1] == 0xdc)
            {
                data[k++] = 0xc0;
                j++;
            }
            else if (p[j+1] == 0xdd)
            {
                data[k++] = 0xdb;
                j++;
            }
            else 
            {
                data[k++] = p[j];
            }
        }
        else 
        {
            data[k++] = p[j];
        }
    }

    free(p);


    return k;
    
}
