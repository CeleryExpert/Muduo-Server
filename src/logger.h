#ifndef X_MUDUO_SRC_LOGGER_H_
#define X_MUDUO_SRC_LOGGER_H_

#include <functional>
#include <sstream>
#include <string>

class Logger {
 public:
  enum LogLevel {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
  };

  using OutputFunc = std::function<void(const std::string&)>;
  using FlushFunc = std::function<void()>;

  static void setLogLevel(LogLevel level);
  static LogLevel logLevel();

  static void setOutput(OutputFunc out);
  static void setFlush(FlushFunc flush);

  // Optional per-thread context fields.
  static void setConnectionId(const std::string& connectionId);
  static void setRequestId(const std::string& requestId);
  static void clearContext();

 private:
  friend class LogMessage;
  static void write(LogLevel level, const char* file, int line, const std::string& body);
};

class LogMessage {
 public:
  LogMessage(const char* file, int line, Logger::LogLevel level);
  ~LogMessage();

  std::ostringstream& stream() { return stream_; }

 private:
  const char* file_;
  int line_;
  Logger::LogLevel level_;
  std::ostringstream stream_;
};

#define LOG_TRACE \
  if (Logger::logLevel() > Logger::TRACE) \
    ; \
  else \
    LogMessage(__FILE__, __LINE__, Logger::TRACE).stream()

#define LOG_DEBUG \
  if (Logger::logLevel() > Logger::DEBUG) \
    ; \
  else \
    LogMessage(__FILE__, __LINE__, Logger::DEBUG).stream()

#define LOG_INFO \
  if (Logger::logLevel() > Logger::INFO) \
    ; \
  else \
    LogMessage(__FILE__, __LINE__, Logger::INFO).stream()

#define LOG_WARN \
  if (Logger::logLevel() > Logger::WARN) \
    ; \
  else \
    LogMessage(__FILE__, __LINE__, Logger::WARN).stream()

#define LOG_ERROR \
  if (Logger::logLevel() > Logger::ERROR) \
    ; \
  else \
    LogMessage(__FILE__, __LINE__, Logger::ERROR).stream()

#define LOG_FATAL LogMessage(__FILE__, __LINE__, Logger::FATAL).stream()

#endif  // X_MUDUO_SRC_LOGGER_H_

