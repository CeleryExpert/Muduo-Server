#include "Socket.h"

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

bool setSocketOpt(int sockfd, int level, int optname, bool on) {
  const int opt = on ? 1 : 0;
  return ::setsockopt(sockfd, level, optname, &opt, sizeof(opt)) == 0;
}

}  // namespace

Socket::Socket() : sockfd_(::socket(AF_INET, SOCK_STREAM, 0)) {}

Socket::Socket(int sockfd) : sockfd_(sockfd) {}

Socket::~Socket() {
  if (sockfd_ >= 0) {
    ::close(sockfd_);
  }
}

int Socket::fd() const {
  return sockfd_;
}

bool Socket::bindAddress(const sockaddr_in& addr) const {
  return ::bind(sockfd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == 0;
}

bool Socket::listen(int backlog) const {
  return ::listen(sockfd_, backlog) == 0;
}

int Socket::accept(sockaddr_in* peerAddr) const {
  sockaddr_in addr {};
  socklen_t len = sizeof(addr);
  int connfd = ::accept(sockfd_, reinterpret_cast<sockaddr*>(&addr), &len);
  if (connfd >= 0 && peerAddr != nullptr) {
    *peerAddr = addr;
  }
  return connfd;
}

bool Socket::shutdownWrite() const {
  return ::shutdown(sockfd_, SHUT_WR) == 0;
}

bool Socket::setReuseAddr(bool on) const {
  return setSocketOpt(sockfd_, SOL_SOCKET, SO_REUSEADDR, on);
}

bool Socket::setReusePort(bool on) const {
#ifdef SO_REUSEPORT
  return setSocketOpt(sockfd_, SOL_SOCKET, SO_REUSEPORT, on);
#else
  (void)on;
  return false;
#endif
}

bool Socket::setKeepAlive(bool on) const {
  return setSocketOpt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, on);
}

bool Socket::setTcpNoDelay(bool on) const {
  return setSocketOpt(sockfd_, IPPROTO_TCP, TCP_NODELAY, on);
}
