
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

extern int terminate;

void handle_cmd_ls(int conn) {

  printf("%s  %s__FUNCTION__", __FILE__, __FUNCTION__);
  unsigned char resp_ret[4] = {CMD_LS, NO_ERROR};

  char *filenames = get_filenames((char *)FTP_ROOT_DIR);
  if (filenames == NULL) {
    resp_ret[1] = 0x11; // 回复 xx 错误

    send_data(conn, (char *)resp_ret, 2);

    return;
  }

  printf("%s  %s__FUNCTION__", __FILE__, __FUNCTION__);
  // 先回复 命令执行成功
  send_data(conn, (char *)resp_ret, 2);

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
  send_data(conn, resp, raw_resp_len);

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

    send_data(conn, (char *)resp, 2);
    return;
  }

  // 1.
  send_data(conn, (char *)resp, 2);

  // 2. 回复文件的长度
  struct stat st;
  stat(file, &st);
  // st.st_size;
  resp[0] = st.st_size & 0xff;
  resp[1] = (st.st_size >> 8) & 0xff;
  resp[2] = (st.st_size >> 16) & 0xff;
  resp[3] = (st.st_size >> 24) & 0xff;
  send_data(conn, (char *)resp, 4);

  // 3. 发送文件内容
  while (1) {
    int ret;
    unsigned char buf[2048];
    ret = read(fd, buf, 2048);
    if (ret > 0) {
      send_data(conn, (char *)buf, ret);
    } else if (ret < 0) {
      break;
    } else {
      break;
    }
  }

  close(fd);
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
    printf("%d  %s\n", __LINE__, __FUNCTION__);
    handle_cmd_ls(conn);
    break;
  case CMD_GET: {
    char *p = (char *)(cmd + 5);
    handle_cmd_get(conn, p);
    break;
  }
  case CMD_PUT:
    break;
  case CMD_BYE:
    break;
  default:
    break;
  }
}

void handle_connection(int conn) {
  unsigned char ch;
  unsigned char cmd[512];
  unsigned char raw_cmd[4096]; // 转义后的命令
  int i, j, k;
  //
  while (!terminate) {
    // 1. 接受一个完整的从client发过来的 ftp 命令
    k = read_raw_data(conn, (char *)raw_cmd, 4096);

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