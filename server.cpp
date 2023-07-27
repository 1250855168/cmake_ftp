#include "Server.h"
#include "EventLoop.h"

int terminate = 0;

int main() {
  EventLoop *loop = new EventLoop();

  Server *server = Server::getInstance(loop);

  loop->loop();

  Server::destoryInstance();

  return 0;
}
