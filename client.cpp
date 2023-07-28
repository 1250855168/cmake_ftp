
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "include/common.h"


//进程退出的标志位
// 1： 进程要退出 
// 0:  进程不退出 
int terminate = 0;

//本程序的用法说明
void usage(void)
{
    printf("本程序是一个 文件传输的 客户端程序\n");
    printf("本程序运行需要带两个参数： 一个服务器的Ip地址,第二个为服务器端口号\n");
    printf("如:  ftp_client 172.8.0.2  9999\n");
}

void sig_handler(int sig)
{
    switch (sig)
    {
    case SIGINT:
        terminate = 1;
        break;
    
    default:
        break;
    }
}


/*
    send_ftp_cmd: 构造一个ftp命令，并发送到套接字
    @sock: 套接字描述符
    @cmd: 命令号
    @arg_len: 命令参数长度
    @arg: 命令参数
    返回值:
        无。
*/
void send_ftp_cmd(int sock, cmd_no_t cmd, int arg_len, char* arg)
{   
    int i = 0, j;
    int ret;

    int raw_cmd_len = 1 + 4 + arg_len; //原始命令的长度(不包含帖头、帧发，没有转义之前)
    unsigned char* raw_cmd = (unsigned char*) malloc(raw_cmd_len);

    raw_cmd[0] = cmd;

    raw_cmd[1] = arg_len & 0xff;
    raw_cmd[2] = (arg_len >> 8) & 0xff;
    raw_cmd[3] = (arg_len >> 16) & 0xff;
    raw_cmd[4] = (arg_len >> 24) & 0xff;

    for (i = 5, j = 0; j <arg_len; i++, j++)
    {
        raw_cmd[i] = arg[j];
    }

    send_data(sock,(unsigned char *)raw_cmd, i);

    free(raw_cmd);
    
}



void handle_cmd_ls(int sock)
{
    int i, j,k;
    int ret;

    unsigned char raw[4096];
    unsigned char resp[8192];
    send_ftp_cmd(sock, CMD_LS, 0, NULL);

    
    while (!terminate)
    {
        // 1. 先接收 执行情况
        unsigned char resp[8192];
        ret = read_raw_data(sock,(unsigned char *) resp, 8192);
        if (ret < 0)
        {
            break;
        }

        if (resp[0] != CMD_LS  || resp[1] != NO_ERROR)
        {
            printf("ls执行出错\n");
            break;
        }

        //2. 接收 ls的执行结果
        ret = read_raw_data(sock,(unsigned char *)resp, 8192);
        if (ret > 0)
        {
            printf("%s\n", resp + 5);
        }

        break;
    }

}

void handle_cmd_get(int sock, char *filename)
{
    printf("用户想要从服务器上下载[%s]这个文件\n", filename);

    int i,j ,k;
    int arg_len = strlen(filename) + 1;
    int cmd_len = 1 + 4 + arg_len;
    unsigned char * cmd = (unsigned char *) malloc( cmd_len );

    cmd[0] = CMD_GET;
    
    cmd[1] = arg_len & 0xff;
    cmd[2] = (arg_len >> 8) & 0xff;
    cmd[3] = (arg_len >> 16) & 0xff;
    cmd[4] = (arg_len >> 24) & 0xff;

    strcpy((char *)(cmd + 5),filename);

    send_data(sock, (unsigned char *)cmd, cmd_len);

  
    // 1. 获取服务器 执行结果，成功还是不成功
    unsigned char resp[4];
    k = read_raw_data(sock,(unsigned char *)resp ,4);
    if (k != 2)
    {
        return;
    }
    if (resp[1] != NO_ERROR)
    {
        return;
    }

    printf("服务器回复get命令 OK!\n");
    //2. 
    k = read_raw_data(sock,(unsigned char *)resp, 4);
    if (k != 4)
    {
        return;
    }

    int file_len = resp[0] | (resp[1] << 8) | (resp[2] << 16) | (resp[3] << 24);
    int r = 0;

    printf("要下载的文件的大小为: %d\n", file_len);

    unsigned char *p = (unsigned char*) malloc(8192);
    int fd = open(filename, O_WRONLY | O_CREAT |O_TRUNC, 0664);

    while (r < file_len)
    {
        int ret = read_raw_data(sock, (unsigned char *)p, 8192);
        if (ret > 0)
        {
            write(fd, p, ret);
            r += ret;
        }
        else if (ret <= 0)
        {
            break;
        }
    }

    free(p);
    close(fd);

    printf("文件%s下载成功,get命令Over\n", filename);
}


int main(int argc, char *argv[])
{
    int sock;
    int ret;
    if (argc != 3)
    {
        usage();
        exit(0);
    }

    signal(SIGINT, sig_handler); //注册信号处理程序


    sock = connect_tcp_server(argv[1], atoi(argv[2]));
    if (sock == -1)
    {
        printf("failed to connect_tcp_server\n");
        return -1;
    }


    while (!terminate)
    {
        unsigned char cmd[256];
        printf("ftp_cmd $ ");
        fflush(stdout);



        fgets((char *)cmd, 256, stdin);
        if ( strlen((char *)cmd) > 0  && cmd[strlen((char *)cmd) - 1] == '\n')
            cmd[strlen((char *)cmd) - 1] = '\0';

        // printf("*****%s****\n", cmd);
        
        if (strcmp((char *)cmd, "ls") == 0)
        {
            // printf("用户输入了 ls 这个命令\n");
            handle_cmd_ls(sock);
        }
        else if (strncmp((char *)cmd, "get", 3) == 0)
        {
            // printf("用户输入了 get 这个命令\n");
            char *p = (char *)(cmd + 3);
            while (*p == ' ') p++;
            handle_cmd_get(sock, p);
        }
        else if (strncmp((char *)cmd, "put", 3) == 0)
        {
            printf("用户输入了 put 这个命令\n");
        }
        else if (strcmp((char *)cmd, "bye") == 0)
        {
            printf("用户输入了 bye 这个命令\n");
        }

        

    }

}