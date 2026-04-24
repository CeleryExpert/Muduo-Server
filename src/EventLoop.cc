#include "Eventloop.h"

EventLoop::EventLoop(){

}
~EventLoop();

void loop();
void quit();

void wakeup();
void updateChannel(Channel *channel);
void removeChannel(Channel *channel);