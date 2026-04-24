#pragma once
#include "nocopyable.h"
#include <sys/epoll.h>
#include <map>
#include <vector>
#include <unistd.h>

class Channel;
class EventLoop;

class Poller : nocopyable
{
public:
    using ChannelList = std::vector<Channel *>;
    Poller(EventLoop *ownloop, int);
    ~Poller();
    void poll(ChannelList *activeChannels);
    void updateChannel(Channel *);
    void removeChannel(Channel *);
    void fillActiveChannels(int, ChannelList *);
private:
    std::map<int, Channel*> channelmap_;
    EventLoop *ownloop_;
    std::vector<struct epoll_event> events_;
    int epollfd_;
};
