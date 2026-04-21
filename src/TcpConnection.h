#ifndef X_MUDUO_SRC_TCP_CONNECTION_H_
#define X_MUDUO_SRC_TCP_CONNECTION_H_

#include <netinet/in.h>

#include <functional>
#include <memory>
#include <string>

class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
 public:
  enum StateE {
    kConnecting = 0,
    kConnected,
    kDisconnecting,
    kDisconnected
  };

  using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
  // Message callback can parse/consume inputBuffer in place.
  using MessageCallback = std::function<void(const TcpConnectionPtr&, std::string&)>;
  using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
  using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
  using ErrorCallback = std::function<void(const TcpConnectionPtr&, int)>;

  TcpConnection(int sockfd, const std::string& name, const sockaddr_in& localAddr,
                const sockaddr_in& peerAddr);
  ~TcpConnection();

  TcpConnection(const TcpConnection&) = delete;
  TcpConnection& operator=(const TcpConnection&) = delete;

  int fd() const;
  const std::string& name() const;
  StateE state() const;
  bool connected() const;

  std::string localIpPort() const;
  std::string peerIpPort() const;

  // Optional socket behavior.
  bool setTcpNoDelay(bool on) const;

  // Callback setters.
  void setConnectionCallback(ConnectionCallback cb);
  void setMessageCallback(MessageCallback cb);
  void setCloseCallback(CloseCallback cb);
  void setWriteCompleteCallback(WriteCompleteCallback cb);
  void setErrorCallback(ErrorCallback cb);

  // Lifecycle hooks.
  void connectEstablished();
  void connectDestroyed();

  // User actions.
  void send(const std::string& data);
  void shutdown();
  void forceClose();
  void setHighWaterMark(size_t bytes);

  // Event handlers (to be called when readable/writable/error).
  void handleRead();
  void handleWrite();
  void handleError();

 private:
  void closeSocket();
  ssize_t writeRaw(const char* data, size_t len);
  void setState(StateE s);

  int sockfd_;
  std::string name_;
  sockaddr_in localAddr_;
  sockaddr_in peerAddr_;
  StateE state_;
  bool socketClosed_;

  // Simple buffers for this stage. You can replace with Buffer later.
  std::string inputBuffer_;
  std::string outputBuffer_;
  size_t highWaterMark_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  CloseCallback closeCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  ErrorCallback errorCallback_;
};

#endif  // X_MUDUO_SRC_TCP_CONNECTION_H_
