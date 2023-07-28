
#include "common.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


extern int terminate;

void handle_cmd_ls(int conn) {
  unsigned char resp_ret[4] = {CMD_LS, NO_ERROR};

  char *filenames = get_filenames(FTP_ROOT_DIR);
  if (filenames == NULL) {
    resp_ret[1] = 0x11; // 回复 xx 错误

    send_data(conn,(unsigned char *) resp_ret, 2);

    return;
  }

  // 先回复 命令执行成功
  send_data(conn,(unsigned char *)  resp_ret, 2);

  int content_len = strlen(filenames) + 1;
  int raw_resp_len = 1 + 4 + content_len;
  char *resp = (char *)malloc(raw_resp_len);
  resp[0] = CMD_LS; // 命令号

  resp[1] = content_len & 0xff;
  resp[2] = (content_len >> 8) & 0xff;
  resp[3] = (content_len >> 16) & 0xff;
  resp[4] = (content_len >> 24) & 0xff;

  strcpy(resp + 5, filenames);

  // 再回复  结果内容
  send_data(conn, (unsigned char *)resp, raw_resp_len);

  free(resp);
  free(filenames);
}

void handle_cmd_get(int conn, char *filename) {
  unsigned char resp[4] = {CMD_GET, NO_ERROR};

  char file[512];
  sprintf(file, "%s/%s", FTP_ROOT_DIR, filename);

  int fd = open(file, O_RDONLY);
  if (fd == -1) {
    printf("open %s failed\n", file);
    resp[1] = 0x11;

    send_data(conn,(unsigned char *)  resp, 2);
    return;
  }

  // 1.
  send_data(conn,(unsigned char *)  resp, 2);

  // 2. 回复文件的长度
  struct stat st;
  stat(file, &st);
  // st.st_size;
  resp[0] = st.st_size & 0xff;
  resp[1] = (st.st_size >> 8) & 0xff;
  resp[2] = (st.st_size >> 16) & 0xff;
  resp[3] = (st.st_size >> 24) & 0xff;
  send_data(conn,(unsigned char *)  resp, 4);

  // 3. 发送文件内容
  while (1) {
    int ret;
    unsigned char buf[2048];
    ret = read(fd, buf, 2048);
    if (ret > 0) {
      send_data(conn,(unsigned char *)  buf, ret);
    } else if (ret < 0) {
      break;
    } else {
      break;
    }
  }

  close(fd);
}

void handle_cmd_put(int conn, char *filename) {
  // 允许发送上传信息
  unsigned char resp[4] = {CMD_GET, NO_ERROR};
  send_data(conn,(unsigned char *)  resp, 2);

  // 2.读取上传文件的大小
  int k = read_raw_data(conn,(unsigned char *)  resp, 4);
  if (k != 4) {
    return;
  }

  int file_len = resp[0] | (resp[1] << 8) | (resp[2] << 16) | (resp[3] << 24);
  int r = 0;

  printf("要上传的文件的大小为: %d\n", file_len);

  unsigned char *p = (unsigned char *)malloc(8192);
  int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);

  while (r < file_len) {
    int ret = read_raw_data(conn,(unsigned char *)  p, 8192);
    if (ret > 0) {
      write(fd, p, ret);
      r += ret;
    } else if (ret <= 0) {
      break;
    }
  }

  free(p);
  close(fd);

  printf("文件%s上传成功,put命令Over\n", filename);
}

void handle_cmd_bye(int conn) {
  unsigned char resp_ret[4] = {CMD_LS, NO_ERROR};
  send_data(conn, (unsigned char *) resp_ret, 2);

  // 2.接收y/n、
  unsigned char data[512];
  read_raw_data(conn,(unsigned char *)  data, 1);
  printf("%s\n", data);
  if (strcmp((const char *)data, "y") == 0) {
    close(conn);
    terminate = 1;
    printf("已断开连接\n");
  } else {
    printf("断开连接失败\n");
  }
}

void handle_cmd_play(int conn) {
  unsigned char resp_ret[4] = {CMD_LS, NO_ERROR};
  send_data(conn,(unsigned char *)  resp_ret, 2);

  int x;
  srandom(time(NULL)); // 设置随机数种子

  x = random() % 100 + 1;

  printf("---------本次的答案:%d-----------\n", x);
  printf("等待客户端发数据过来\n");

  while (!terminate) {

    unsigned char num[4];
    read_raw_data(conn,(unsigned char *)  num, 4);
    int client_x = (num[3] << 24) | (num[2] << 16) | (num[1] << 8) | num[0];
    printf("%d\n", client_x);
    unsigned char resp[5];
    for (int i = 0; i < 4; i++) {
      resp[i] = num[i];
    }
    if (client_x > x) {
      resp[4] = 1;
    } else if (client_x < x) {
      resp[4] = 2;
    } else {
      resp[4] = 0;
    }
    send_data(conn,(unsigned char *)  resp, 5);
  }
}
void handle_ftp_cmd(int conn, unsigned char *cmd, int len) {
  //
  int cmd_no = cmd[0];
  int arg_len = (cmd[1] | (cmd[2] << 8) | (cmd[3] << 16) | (cmd[4] << 24));

  if (len < 1 + 4 + arg_len) {
    return;
  }

  switch (cmd_no) {
  case CMD_LS:
    handle_cmd_ls(conn);
    break;
  case CMD_GET: {
    char *p = (char *)(cmd + 5);
    handle_cmd_get(conn, p);
    break;
  }
  case CMD_PUT: {
    char *p =(char *) (cmd + 5);
    handle_cmd_put(conn, p);
    break;
  }
  case CMD_BYE: {
    handle_cmd_bye(conn);
    break;
  }
  case CMD_PLAY:
    handle_cmd_play(conn);
  default:
    break;
  }
}

void handle_connection(int conn) {
  unsigned char ch;
  unsigned char cmd[512];
  unsigned char raw_cmd[4096]; // 转义后的命令
  int i, j, k;

  while (!terminate) {
    // 1. 接受一个完整的从client发过来的 ftp 命令
    k = read_raw_data(conn,(unsigned char *)  raw_cmd, 4096);

    // for debug
    printf("收到命令: ");
    for (j = 0; j < k; j++) {
      printf("%02x ", raw_cmd[j]);
    }
    printf("\n");

    // 2. 处理命令
    //  do it yourself...
    handle_ftp_cmd(conn, raw_cmd, k);
  }
}