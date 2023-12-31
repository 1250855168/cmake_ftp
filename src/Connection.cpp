/******************************
 *   author: yuesong-feng
 *
 *
 *
 ******************************/
#include "Connection.h"
#include "Buffer.h"
#include "Channel.h"
#include "Socket.h"
#include "common.h"
#include "handler.h"
#include "util.h"
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Connection::Connection(EventLoop *_loop, Socket *_sock)
    : loop(_loop), sock(_sock), channel(nullptr), inBuffer(new std::string()),
      readBuffer(nullptr) {
  channel = new Channel(loop, sock->getFd());
  channel->enableRead();
  channel->useET();
  std::function<void()> cb = std::bind(&Connection::echo, this, sock->getFd());
  channel->setReadCallback(cb);
  channel->setUseThreadPool(true);
  readBuffer = new Buffer();
}

Connection::~Connection() {
  delete channel;
  delete sock;
  delete readBuffer;
}

void Connection::setDeleteConnectionCallback(std::function<void(int)> _cb) {
  deleteConnectionCallback = _cb;
}

void Connection::echo(int sockfd) {

  unsigned char ch;
  unsigned char cmd[512];
  unsigned char raw_cmd[4096]; // 转义后的命令
  int i, j;

  while (
      true) { // 由于使用非阻塞IO，读取客户端buffer，一次读取buf大小数据，直到全部读取完毕

    // ssize_t bytes_read = read(sockfd, buf, sizeof(buf));

    printf("%d %s\n", __LINE__, __FUNCTION__);

    ssize_t bytes_read = read_raw_data(sockfd, (unsigned char *)raw_cmd, 4096);

    printf("收到命令: ");

    for (j = 0; j < bytes_read; j++) {
      printf("%02x ", raw_cmd[j]);
    }
    printf("\n");

    if (bytes_read > 0) {

      // 进行数据包的发送
      handle_ftp_cmd(sockfd, (unsigned char *)raw_cmd, bytes_read);

      break;

    } else if (bytes_read == -1 && errno == EINTR) { // 客户端正常中断、继续读取
      printf("continue reading\n");
      continue;
    } else if (bytes_read == -1 &&
               ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
      // 非阻塞IO，这个条件表示数据全部读取完毕
      // printf("message from client fd %d: %s\n", sockfd, readBuffer->c_str());

      //  errif(write(sockfd, readBuffer->c_str(), readBuffer->size()) == -1,
      //  "socket write error");

      // send(sockfd);

      readBuffer->clear();
      break;
    } else if (bytes_read == 0) { // EOF，客户端断开连接
      printf("EOF, client fd %d disconnected\n", sockfd);
      deleteConnectionCallback(sockfd); // 多线程会有bug
      break;
    } else {
      printf("Connection reset by peer\n");
      deleteConnectionCallback(sockfd); // 会有bug，注释后单线程无bug
      break;
    }
  }
}

void Connection::send(int sockfd) {
  char buf[readBuffer->size()];
  strcpy(buf, readBuffer->c_str());
  int data_size = readBuffer->size();
  int data_left = data_size;
  while (data_left > 0) {

    // 需要对发送的东西进行解析

    ssize_t bytes_write = write(sockfd, buf + data_size - data_left, data_left);
    if (bytes_write == -1 && errno == EAGAIN) {
      break;
    }
    data_left -= bytes_write;
  }
}