# Muduo 开发指导文档（关键对象 + 参数 + 函数）

## 1. 总体关系图
1. `TcpServer` 持有 `Acceptor`、`EventLoopThreadPool`、连接表。
2. `Acceptor` 监听 `listenfd`，新连接到来后回调 `TcpServer::newConnection`。
3. 每个连接创建一个 `TcpConnection`，内部绑定一个 `Channel`。
4. 每个线程一个 `EventLoop`，内部用 `Poller(epoll)` 等待事件。
5. 数据收发走 `Buffer`，定时任务走 `TimerQueue`，日志走 `Logger`。

---

## 2. EventLoop
职责：线程内事件循环、事件分发、跨线程任务执行。

常见关键成员（你开发时重点关注）：
1. `bool looping_`：是否处于循环中。
2. `bool quit_`：是否退出循环。
3. `std::thread::id threadId_`：所属线程 id。
4. `Poller* poller_`：多路复用器。
5. `TimerQueue* timerQueue_`：定时器管理。
6. `int wakeupFd_`：跨线程唤醒 fd（通常 eventfd）。
7. `std::vector<Channel*> activeChannels_`：本轮活跃事件集合。

关键函数（按用途）：
1. `void loop();`  
2. `void quit();`  
3. `Timestamp pollReturnTime() const;`  
4. `void runInLoop(Functor cb);`  
5. `void queueInLoop(Functor cb);`  
6. `TimerId runAt(Timestamp time, TimerCallback cb);`  
7. `TimerId runAfter(double delay, TimerCallback cb);`  
8. `TimerId runEvery(double interval, TimerCallback cb);`  
9. `void updateChannel(Channel* channel);`  
10. `void removeChannel(Channel* channel);`

线程规则：
1. `runInLoop`：如果当前就是 loop 线程，立即执行。
2. `queueInLoop`：无论在哪个线程都入队，随后唤醒 loop 线程执行。

---

## 3. Poller（Linux 常用 EPollPoller）
职责：封装 `epoll_wait/epoll_ctl`，维护 fd 到 `Channel` 的映射。

常见关键成员：
1. `int epollfd_`：epoll 实例 fd。
2. `std::vector<epoll_event> events_`：内核返回的活跃事件数组。
3. `ChannelMap channels_`：`fd -> Channel*` 映射。

关键函数：
1. `Timestamp poll(int timeoutMs, ChannelList* activeChannels);`
2. `void updateChannel(Channel* channel);`
3. `void removeChannel(Channel* channel);`

开发关注参数：
1. `timeoutMs`：建议统一配置，比如 10000ms。
2. 活跃数组容量 `events_.size()`：需要动态扩容，避免高峰丢活跃事件。

---

## 4. Channel
职责：把一个 fd 的“关注事件”和“回调函数”绑定起来。

常见关键成员：
1. `int fd_`：关联 fd。
2. `int events_`：关注的事件位（`EPOLLIN/EPOLLOUT` 等）。
3. `int revents_`：本轮实际触发事件位。
4. `int index_`：在 Poller 中状态（new/added/deleted）。
5. 回调：
   - `ReadEventCallback readCallback_`
   - `EventCallback writeCallback_`
   - `EventCallback closeCallback_`
   - `EventCallback errorCallback_`

关键函数：
1. `void handleEvent(Timestamp receiveTime);`
2. `void setReadCallback(ReadEventCallback cb);`
3. `void setWriteCallback(EventCallback cb);`
4. `void setCloseCallback(EventCallback cb);`
5. `void setErrorCallback(EventCallback cb);`
6. `void enableReading();`
7. `void enableWriting();`
8. `void disableWriting();`
9. `void disableAll();`
10. `bool isWriting() const;`

事件参数要点：
1. 读：`EPOLLIN | EPOLLPRI`。
2. 写：`EPOLLOUT`。
3. 错误：`EPOLLERR`。
4. 挂断：`EPOLLHUP`。

---

## 5. Acceptor
职责：监听新连接，处理 `accept`，把 `connfd` 交给上层。

常见关键成员：
1. `EventLoop* loop_`：所属 loop（通常 base loop）。
2. `Socket acceptSocket_`：监听 socket。
3. `Channel acceptChannel_`：监听 fd 对应 channel。
4. `NewConnectionCallback newConnectionCallback_`。
5. `bool listenning_`。

关键函数：
1. `void listen();`
2. `void handleRead();`
3. `void setNewConnectionCallback(const NewConnectionCallback& cb);`

关键参数建议：
1. `listen backlog`：结合系统 `somaxconn` 调整。
2. 是否启用 `reuseport`：多进程/多实例场景再开。

---

## 6. TcpServer
职责：服务总控，管理线程池、连接生命周期、业务回调挂载。

常见关键成员：
1. `EventLoop* loop_`：base loop。
2. `std::unique_ptr<Acceptor> acceptor_`。
3. `std::shared_ptr<EventLoopThreadPool> threadPool_`。
4. `ConnectionMap connections_`：`name -> TcpConnectionPtr`。
5. 回调：
   - `ConnectionCallback connectionCallback_`
   - `MessageCallback messageCallback_`
   - `WriteCompleteCallback writeCompleteCallback_`
6. `int nextConnId_`：连接 id 生成。

关键函数：
1. `void setThreadNum(int numThreads);`
2. `void setConnectionCallback(ConnectionCallback cb);`
3. `void setMessageCallback(MessageCallback cb);`
4. `void setWriteCompleteCallback(WriteCompleteCallback cb);`
5. `void start();`
6. `void newConnection(int sockfd, const InetAddress& peerAddr);`
7. `void removeConnection(const TcpConnectionPtr& conn);`

关键参数建议：
1. `numThreads`：CPU 密集低，I/O 密集可适当增加。
2. 连接命名：`ip:port#id`，便于日志追踪。

---

## 7. TcpConnection
职责：单连接状态管理、读写回调、输入输出缓冲管理。

常见关键成员：
1. `EventLoop* loop_`：归属 I/O 线程。
2. `std::string name_`：连接名。
3. `StateE state_`：`kConnecting/kConnected/kDisconnecting/kDisconnected`。
4. `Socket socket_`。
5. `Channel channel_`。
6. `Buffer inputBuffer_`。
7. `Buffer outputBuffer_`。
8. `InetAddress localAddr_`、`peerAddr_`。

关键函数：
1. `void send(const StringPiece& message);`
2. `void shutdown();`
3. `void forceClose();`
4. `void setTcpNoDelay(bool on);`
5. `void connectEstablished();`
6. `void connectDestroyed();`
7. `void handleRead(Timestamp receiveTime);`
8. `void handleWrite();`
9. `void handleClose();`
10. `void handleError();`

开发参数要点：
1. 高水位阈值：`highWaterMark`（例如 64MB）。
2. 输出缓冲上限：超限后限流或断连。
3. `TcpNoDelay`：低延迟场景可开启。

---

## 8. Buffer
职责：高效字节缓冲，支撑粘包拆包和零碎写入。

常见关键成员：
1. `std::vector<char> buffer_`。
2. `size_t readerIndex_`。
3. `size_t writerIndex_`。

关键函数：
1. `size_t readableBytes() const;`
2. `size_t writableBytes() const;`
3. `const char* peek() const;`
4. `void retrieve(size_t len);`
5. `void retrieveAll();`
6. `std::string retrieveAsString(size_t len);`
7. `std::string retrieveAllAsString();`
8. `void append(const char* data, size_t len);`
9. `ssize_t readFd(int fd, int* savedErrno);`
10. `ssize_t writeFd(int fd, int* savedErrno);`

协议参数建议（你实现业务协议时）：
1. 包头固定长度（例如 4 字节长度）。
2. `maxFrameLen`（例如 1MB）防攻击。
3. 不完整包不处理，等下次数据到达。

---

## 9. EventLoopThreadPool
职责：管理多个 I/O 线程和它们各自的 `EventLoop`。

常见关键成员：
1. `EventLoop* baseLoop_`。
2. `std::string name_`。
3. `bool started_`。
4. `int numThreads_`。
5. `int next_`：轮询下标。
6. `std::vector<std::unique_ptr<EventLoopThread>> threads_`。
7. `std::vector<EventLoop*> loops_`。

关键函数：
1. `void setThreadNum(int numThreads);`
2. `void start(const ThreadInitCallback& cb = ThreadInitCallback());`
3. `EventLoop* getNextLoop();`
4. `std::vector<EventLoop*> getAllLoops();`

参数建议：
1. `numThreads=0` 表示单线程模式（仅 base loop）。
2. 初学调试建议 1~2 线程，排障简单。

---

## 10. TimerQueue
职责：管理定时器，支持一次性和周期任务。

常见关键成员：
1. `int timerfd_`：内核定时器 fd。
2. `Channel timerfdChannel_`。
3. `TimerList timers_`：按过期时间排序。
4. `ActiveTimerSet activeTimers_`。

关键函数：
1. `TimerId addTimer(TimerCallback cb, Timestamp when, double interval);`
2. `void cancel(TimerId timerId);`
3. `void handleRead();`
4. `std::vector<Entry> getExpired(Timestamp now);`
5. `void reset(const std::vector<Entry>& expired, Timestamp now);`

参数建议：
1. 心跳扫描周期：1~5 秒。
2. 空闲踢线阈值：30~300 秒按业务定。

---

## 11. Logger
职责：统一日志输出、级别控制、故障定位。

常见能力参数：
1. 日志级别：`TRACE/DEBUG/INFO/WARN/ERROR/FATAL`。
2. 输出方式：stdout、文件、异步落盘。
3. 格式字段：时间、线程 id、连接 id、请求 id、源文件行号。

关键接口（按 Muduo 常见用法）：
1. `Logger::setLogLevel(Logger::LogLevel level);`
2. `Logger::setOutput(OutputFunc out);`
3. `Logger::setFlush(FlushFunc flush);`
4. 宏：`LOG_INFO`、`LOG_WARN`、`LOG_ERROR`、`LOG_FATAL`。

日志字段建议至少包含：
1. `connName`
2. `peerIp:peerPort`
3. `requestId`
4. `cmd`
5. `costMs`

---

## 12. 一次完整调用链（你开发时对照）
1. `TcpServer::start()`
2. `Acceptor::listen()`
3. `EventLoop::loop() -> Poller::poll()`
4. 新连接：`Acceptor::handleRead() -> TcpServer::newConnection()`
5. `TcpConnection::connectEstablished()`
6. 收包：`Channel::handleEvent() -> TcpConnection::handleRead()`
7. 解包：`Buffer` 读取 + 业务协议解析
8. 回包：`TcpConnection::send() -> handleWrite()`
9. 定时：`TimerQueue` 周期任务触发
10. 断连：`TcpConnection::handleClose() -> TcpServer::removeConnection()`

---

## 13. 开发落地最小检查清单
1. `onConnection/onMessage/onWriteComplete` 三个回调都已绑定。
2. 协议有长度校验与 `maxFrameLen` 限制。
3. 输出缓冲有高水位处理策略。
4. 定时器已做空闲连接清理。
5. 日志能定位到具体连接和请求。
