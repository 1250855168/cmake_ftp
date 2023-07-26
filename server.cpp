#include "Server.h"
#include "EventLoop.h"

int terminate = 0;

int main() {
  EventLoop *loop = new EventLoop();
  Server *server = new Server(loop);
  loop->loop();
  return 0;
}
