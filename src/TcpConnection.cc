#include "TcpConnection.h"

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "logger.h"

namespace {

std::string toIpPort(const sockaddr_in& addr) {
  char ip[INET_ADDRSTRLEN] = {0};
  if (::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip)) == nullptr) {
    return "0.0.0.0:0";
  }
  return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

}  // namespace

TcpConnection::TcpConnection(int sockfd, const std::string& name, const sockaddr_in& localAddr,
                             const sockaddr_in& peerAddr)
    : sockfd_(sockfd),
      name_(name),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      state_(kConnecting),
      socketClosed_(false),
      highWaterMark_(64 * 1024 * 1024) {}

TcpConnection::~TcpConnection() {
  closeSocket();
}

int TcpConnection::fd() const {
  return sockfd_;
}

const std::string& TcpConnection::name() const {
  return name_;
}

TcpConnection::StateE TcpConnection::state() const {
  return state_;
}

bool TcpConnection::connected() const {
  return state_ == kConnected;
}

std::string TcpConnection::localIpPort() const {
  return toIpPort(localAddr_);
}

std::string TcpConnection::peerIpPort() const {
  return toIpPort(peerAddr_);
}

bool TcpConnection::setTcpNoDelay(bool on) const {
  const int opt = on ? 1 : 0;
  return ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == 0;
}

void TcpConnection::setConnectionCallback(ConnectionCallback cb) {
  connectionCallback_ = std::move(cb);
}

void TcpConnection::setMessageCallback(MessageCallback cb) {
  messageCallback_ = std::move(cb);
}

void TcpConnection::setCloseCallback(CloseCallback cb) {
  closeCallback_ = std::move(cb);
}

void TcpConnection::setWriteCompleteCallback(WriteCompleteCallback cb) {
  writeCompleteCallback_ = std::move(cb);
}

void TcpConnection::setErrorCallback(ErrorCallback cb) {
  errorCallback_ = std::move(cb);
}

void TcpConnection::connectEstablished() {
  setState(kConnected);
  if (connectionCallback_) {
    connectionCallback_(shared_from_this());
  }
}

void TcpConnection::connectDestroyed() {
  if (state_ == kConnected || state_ == kDisconnecting) {
    setState(kDisconnected);
    if (connectionCallback_) {
      connectionCallback_(shared_from_this());
    }
  }
  closeSocket();
}

void TcpConnection::send(const std::string& data) {
  if (state_ != kConnected && state_ != kDisconnecting) {
    return;
  }
  if (data.empty()) {
    return;
  }

  // Fast path: if no pending output, try sending immediately.
  if (outputBuffer_.empty()) {
    const ssize_t n = writeRaw(data.data(), data.size());
    if (n >= 0) {
      if (static_cast<size_t>(n) == data.size()) {
        if (writeCompleteCallback_) {
          writeCompleteCallback_(shared_from_this());
        }
        return;
      }
      outputBuffer_.append(data.data() + n, data.size() - static_cast<size_t>(n));
    } else {
      // Error path: for EAGAIN/EWOULDBLOCK we queue all data and wait writable.
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        outputBuffer_.append(data);
      } else {
        handleError();
        return;
      }
    }
  } else {
    outputBuffer_.append(data);
  }

  if (outputBuffer_.size() >= highWaterMark_) {
    LOG_WARN << "high watermark reached, conn=" << name_ << " bytes=" << outputBuffer_.size();
  }
}

void TcpConnection::shutdown() {
  if (state_ == kConnected) {
    setState(kDisconnecting);
    if (outputBuffer_.empty()) {
      ::shutdown(sockfd_, SHUT_WR);
    }
  }
}

void TcpConnection::forceClose() {
  if (state_ == kConnected || state_ == kDisconnecting) {
    setState(kDisconnected);
    closeSocket();
    if (closeCallback_) {
      closeCallback_(shared_from_this());
    }
  }
}

void TcpConnection::setHighWaterMark(size_t bytes) {
  highWaterMark_ = bytes;
}

void TcpConnection::handleRead() {
  if (state_ != kConnected && state_ != kDisconnecting) {
    return;
  }

  char buf[4096];
  for (;;) {
    const ssize_t n = ::read(sockfd_, buf, sizeof(buf));
    if (n > 0) {
      inputBuffer_.append(buf, static_cast<size_t>(n));
      continue;
    }

    if (n == 0) {
      // Peer closed gracefully.
      setState(kDisconnected);
      closeSocket();
      if (closeCallback_) {
        closeCallback_(shared_from_this());
      }
      return;
    }

    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }
    handleError();
    return;
  }

  if (!inputBuffer_.empty() && messageCallback_) {
    messageCallback_(shared_from_this(), inputBuffer_);
  }
}

void TcpConnection::handleWrite() {
  if (outputBuffer_.empty()) {
    if (state_ == kDisconnecting) {
      ::shutdown(sockfd_, SHUT_WR);
    }
    return;
  }

  const ssize_t n = writeRaw(outputBuffer_.data(), outputBuffer_.size());
  if (n > 0) {
    outputBuffer_.erase(0, static_cast<size_t>(n));
    if (outputBuffer_.empty()) {
      if (writeCompleteCallback_) {
        writeCompleteCallback_(shared_from_this());
      }
      if (state_ == kDisconnecting) {
        ::shutdown(sockfd_, SHUT_WR);
      }
    }
    return;
  }

  if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
    handleError();
  }
}

void TcpConnection::handleError() {
  int err = 0;
  socklen_t len = sizeof(err);
  if (::getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &err, &len) != 0) {
    err = errno;
  }
  LOG_ERROR << "socket error, conn=" << name_ << " err=" << err
            << " msg=" << std::strerror(err);
  if (errorCallback_) {
    errorCallback_(shared_from_this(), err);
  }
}

void TcpConnection::closeSocket() {
  if (!socketClosed_ && sockfd_ >= 0) {
    ::close(sockfd_);
    socketClosed_ = true;
    sockfd_ = -1;
  }
}

ssize_t TcpConnection::writeRaw(const char* data, size_t len) {
  for (;;) {
    const ssize_t n = ::write(sockfd_, data, len);
    if (n >= 0) {
      return n;
    }
    if (errno == EINTR) {
      continue;
    }
    return -1;
  }
}

void TcpConnection::setState(StateE s) {
  state_ = s;
}
