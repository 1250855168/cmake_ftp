
#ifndef __COMMON_H__
#define __COMMON_H__

#define FTP_ROOT_DIR "/mnt/hgfs/gxiang/7.test/7-project/1-ftp_server"
#define FTP_PUT_DIR "/mnt/hgfs/gxiang/7.test/7-project/2-ftp_client"

typedef enum cmd_no {
  CMD_LS = 0x01,
  CMD_GET = 0x02,
  CMD_PUT = 0x03,
  CMD_BYE = 0x04,
  CMD_PLAY = 0x05,

  CMD_UNKNOWN = 0Xff
} cmd_no_t;

typedef enum errno_t {
  NO_ERROR = 0x00,

  //...
} errno_t;

/*
    create_listen_sockct:用于TCP服务端，创建一个“监听套接字”
    @ip: 服务器的ip地址
    @port:端口号
    返回值：
        成功，返回一个监听套接字
        失败，返回-1
*/
int create_listen_sockct(const char *ip, short port);

/*
    connect_tcp_server: 创建一个TCP的套接字，并去连接服务器
    @serv_ip: tcp服务器的ip地址
    @port: tcp服务器的端口号
    返回值：
        成功，返回连接套接字
        失败 返回-1
*/
int connect_tcp_server(char *serv_ip, short port);

/*
    get_filenames: 获取一个目录下面所有文件的文件名
    @dir: 目录名
    返回值：
        该目录下面所有文件(包括目录)文件名(以空格分开)字符串。
*/
char *get_filenames(char *dir);

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
int send_data(int conn, unsigned char *raw_data, int len);

/*
    read_raw_data: 从套接字中，接收一个完整的 命令或数据包
    @sock: 套接字描述符
    @data: 指向的空间，用来保存获取的原始的数据或命令包
    @max_len: data指向的空间最多能容纳多少个字节
    返回值：
        成功 返回实际接收到的命令或数据包的字节数
        失败 返回-1;
*/
int read_raw_data(int sock, unsigned char *data, int max_len);
#endif