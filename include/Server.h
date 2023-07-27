/******************************
 *   author: yuesong-feng
 *
 *
 *
 ******************************/
#pragma once
#include <map>
#include <mutex>
class EventLoop;
class Socket;
class Acceptor;
class Connection;

class Server {
private:
  static Server *instance;
  EventLoop *loop;
  Acceptor *acceptor;
  std::map<int, Connection *> connections;
  Server(EventLoop *);
  ~Server();
  static std::mutex mutex;

public:
  static Server *getInstance(EventLoop *loop) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!instance) {
      instance = new Server(loop);
    }
    return instance;
  }

  static void destoryInstance() {
    if (!instance) {
      delete instance;
      instance = nullptr;
    }
  }

  void handleReadEvent(int);
  void newConnection(Socket *sock);
  void deleteConnection(int sockfd);
};
