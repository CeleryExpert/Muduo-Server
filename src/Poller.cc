#include "Poller.h"
#include "logger.h"
#include "Channel.h"

Poller::Poller(EventLoop *ownloop, int eventsize = 16) : ownloop_(ownloop), events_(eventsize)
{
    epollfd_ = ::epoll_create1(EPOLL_CLOEXEC);
}
Poller::~Poller()
{
    ::close(epollfd_);
}
void Poller::poll(Poller::ChannelList *activeChannels)
{
    // ::epoll_wait(epollfd_,events_.data(),static_cast<int>(event_.size()),0);
    int eventnum = ::epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), 0);
    if (eventnum > 0)
    {
        fillActiveChannels(eventnum, activeChannels);
        if (eventnum == static_cast<int>(events_.size()))
        {
            events_.resize(eventnum * 2);
        }
    }
    else if (eventnum == 0)
    {
        LOG_INFO << "NO ACTIVE CHANNELS";
    }
}
void Poller::fillActiveChannels(int eventnum, Poller::ChannelList *activeChannels)
{
    for (int i = 0; i < eventnum; i++)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->setRevents(events_[i].events);
        activeChannels->push_back(channel);
    }
}
void Poller::updateChannel(Channel *channel){
    int fd = channel->getFd();
    auto it = channelmap_.find(fd);
    

}
void removeChannel(Channel *channel){

}