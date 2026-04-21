#ifndef X_MUDUO_SRC_SOCKET_H_
#define X_MUDUO_SRC_SOCKET_H_

#include <netinet/in.h>

class Socket {
 public:
  explicit Socket();
  explicit Socket(int sockfd);
  ~Socket();

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  int fd() const;

  bool bindAddress(const sockaddr_in& addr) const;
  bool listen(int backlog = 128) const;
  int accept(sockaddr_in* peerAddr = nullptr) const;

  bool shutdownWrite() const;

  bool setReuseAddr(bool on) const;
  bool setReusePort(bool on) const;
  bool setKeepAlive(bool on) const;
  bool setTcpNoDelay(bool on) const;

 private:
  int sockfd_;
};

#endif  // X_MUDUO_SRC_SOCKET_H_
