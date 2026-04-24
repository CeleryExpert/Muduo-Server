#include "Channel.h"
#include <poll.h>
class Timestamp;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd) : loop_(loop),
                                            fd_(fd),
                                            event_(0),
                                            revent_(0),
                                            tied_(false),
                                            eventHandling_(false),
                                            addedtoLoop_(false),
                                            index_(-1)
{
}

Channel::~Channel()
{
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    if (!tied_)
    {
        tie_ = obj;
        tied_ = true;
    }
}
void Channel::handleEvent(const Timestamp &recievetime)
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock();
        if (guard)
        {
            HandleEventsWithGuard(recievetime);
        }
    }
    else
    {
        HandleEventsWithGuard(recievetime);
    }
}

void Channel::HandleEventsWithGuard(const Timestamp &recievetime)
{
    eventHandling_ = true;

    // todo

    eventHandling_ = false;
}

void Channel::update()
{
    addedtoLoop_ = true;
    // loop_->updateChannel(this);
}
