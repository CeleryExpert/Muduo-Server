#include "logger.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <thread>

namespace {

std::mutex g_logMutex;
Logger::LogLevel g_logLevel = Logger::INFO;

void defaultOutput(const std::string& msg) {
  std::fwrite(msg.data(), 1, msg.size(), stdout);
}

void defaultFlush() {
  std::fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

thread_local std::string t_connectionId;
thread_local std::string t_requestId;

const char* levelName(Logger::LogLevel level) {
  switch (level) {
    case Logger::TRACE:
      return "TRACE";
    case Logger::DEBUG:
      return "DEBUG";
    case Logger::INFO:
      return "INFO";
    case Logger::WARN:
      return "WARN";
    case Logger::ERROR:
      return "ERROR";
    case Logger::FATAL:
      return "FATAL";
  }
  return "UNKNOWN";
}

std::string currentTimeString() {
  const auto now = std::chrono::system_clock::now();
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()) %
                  1000;
  const std::time_t tt = std::chrono::system_clock::to_time_t(now);

  std::tm tmLocal{};
  localtime_r(&tt, &tmLocal);

  std::ostringstream oss;
  oss << std::put_time(&tmLocal, "%Y-%m-%d %H:%M:%S")
      << '.' << std::setw(3) << std::setfill('0') << ms.count();
  return oss.str();
}

}  // namespace

void Logger::setLogLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(g_logMutex);
  g_logLevel = level;
}

Logger::LogLevel Logger::logLevel() {
  std::lock_guard<std::mutex> lock(g_logMutex);
  return g_logLevel;
}

void Logger::setOutput(OutputFunc out) {
  std::lock_guard<std::mutex> lock(g_logMutex);
  g_output = out ? std::move(out) : defaultOutput;
}

void Logger::setFlush(FlushFunc flush) {
  std::lock_guard<std::mutex> lock(g_logMutex);
  g_flush = flush ? std::move(flush) : defaultFlush;
}

void Logger::setConnectionId(const std::string& connectionId) {
  t_connectionId = connectionId;
}

void Logger::setRequestId(const std::string& requestId) {
  t_requestId = requestId;
}

void Logger::clearContext() {
  t_connectionId.clear();
  t_requestId.clear();
}

void Logger::write(LogLevel level, const char* file, int line, const std::string& body) {
  std::ostringstream oss;
  oss << currentTimeString()
      << " [" << levelName(level) << "]"
      << " [tid=" << std::this_thread::get_id() << "]";

  if (!t_connectionId.empty()) {
    oss << " [conn=" << t_connectionId << "]";
  }
  if (!t_requestId.empty()) {
    oss << " [req=" << t_requestId << "]";
  }

  oss << " [" << file << ":" << line << "] "
      << body << '\n';

  std::lock_guard<std::mutex> lock(g_logMutex);
  g_output(oss.str());
  if (level >= ERROR) {
    g_flush();
  }
}

LogMessage::LogMessage(const char* file, int line, Logger::LogLevel level)
    : file_(file), line_(line), level_(level) {}

LogMessage::~LogMessage() {
  Logger::write(level_, file_, line_, stream_.str());
  if (level_ == Logger::FATAL) {
    std::abort();
  }
}
