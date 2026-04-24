#include "nocopyable.h"
#include "Channel.h"
#include "Poller.h"
#include <functional>
#include <mutex>

class EventLoop : nocopyable
{
public:
    using Functors = std::function<void()>;
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void wakeup();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
private:
    using ChannelList = std::vector<Channel*>;
    ChannelList activeChannels_;
    bool looping_;
    const pid_t threadid_;
    Channel *currentActiveChannel_;
    std::mutex mtx_;
    std::vector<Functors> pendingFunctors;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<Channel> wakeupChannel_;
    int wkupfd_;
};