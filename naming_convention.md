# Muduo 项目命名规范（C++）

## 1. 规则来源
本规范基于两部分：

1. Muduo 代码风格习惯（如 `TcpServer`、`EventLoop`、`setMessageCallback`）。
2. C++ 工程通用可读性规范（类型与函数可区分、成员变量可识别、回调语义清晰）。

目标：让你写的业务代码和 Muduo 风格一致，降低维护成本。

---

## 2. 总体原则
1. 类型名用 `PascalCase`，函数名用 `camelCase`。
2. 成员变量统一后缀 `_`。
3. 常量名有明确前缀或 `k` 前缀。
4. 名称体现职责，不用模糊词（如 `data1`、`tmp2`）。
5. 缩写词保持稳定写法（`Tcp`、`Ip`、`Id`、`Fd`）。

---

## 3. 具体命名规则

## 3.1 类、结构体、枚举、类型别名
1. 类名：`PascalCase`，名词化。
2. 结构体：`PascalCase`，表示数据承载。
3. 枚举类型：`PascalCase`。
4. 类型别名：`PascalCase` 或 `XxxCallback`。

示例：
```cpp
class AppServer;
struct ConnectionContext;
enum class ConnState;
using MessageHandler = std::function<void(...)>;
```

---

## 3.2 函数命名
1. 普通函数：`camelCase`，动词开头。
2. Getter：`xxx()`，避免 `getXxx()`（与 Muduo 风格统一）。
3. Setter：`setXxx(...)`。
4. 布尔判断：`isXxx()` / `hasXxx()` / `canXxx()`。
5. 回调函数：`onXxx(...)`。
6. 事件处理：`handleXxx(...)`。

示例：
```cpp
void start();
void stop();
void setThreadNum(int numThreads);
bool isAuthenticated() const;
void onMessage(const TcpConnectionPtr&, Buffer*, Timestamp);
void handleRead(Timestamp receiveTime);
```

---

## 3.3 变量命名
1. 局部变量：`camelCase`。
2. 成员变量：`camelCase_`（必须带下划线后缀）。
3. 全局变量：尽量禁止；必须使用时加命名空间并前缀 `g_`。
4. 布尔变量：`is/has/can/need/enable` 前缀。

示例：
```cpp
int maxPacketBytes;
bool needClose;
EventLoop* loop_;
Buffer inputBuffer_;
```

---

## 3.4 常量命名
1. 编译期常量：`kPascalCase`。
2. 宏常量：仅系统兼容时使用，全大写下划线。
3. 配置项 key：小写加点分隔或下划线（按配置文件风格统一）。

示例：
```cpp
constexpr int kDefaultIdleTimeoutSec = 60;
constexpr size_t kDefaultHighWaterMark = 64 * 1024 * 1024;
```

---

## 3.5 文件命名
1. 业务层文件统一小写下划线：`app_server.h/.cc`。
2. 与 Muduo 原生类同名适配时，可保留 `PascalCase` 文件名，但项目内尽量统一一种风格。
3. 一个类对应一对 `.h/.cc`，文件名与主类语义一致。

推荐：
```text
app_server.h
app_server.cc
protocol_codec.h
protocol_codec.cc
session_manager.h
session_manager.cc
```

---

## 3.6 回调与处理器命名
1. 注册函数：`setXxxCallback`。
2. 回调变量：`xxxCallback_`。
3. 触发函数：`onXxx`（业务层）或 `handleXxx`（网络层/内部）。

示例：
```cpp
void setMessageCallback(MessageCallback cb);
MessageCallback messageCallback_;
void onConnection(const TcpConnectionPtr& conn);
void handleClose();
```

---

## 3.7 枚举值命名
1. `enum class` 值使用 `PascalCase`。
2. 若沿用 Muduo 风格状态机，可保留 `kXxx`（如 `kConnected`）。

示例：
```cpp
enum class ErrCode {
  Ok = 0,
  BadPacket = 1001,
  Unauthorized = 1002
};

enum StateE {
  kConnecting,
  kConnected,
  kDisconnecting,
  kDisconnected
};
```

---

## 4. 缩写词规范（统一很重要）
统一写法如下：

1. `Tcp` 不写 `TCP`
2. `Ip` 不写 `IP`（类型/变量中）
3. `Id` 不写 `ID`
4. `Fd` 不写 `FD`
5. `Url` 不写 `URL`

示例：
```cpp
TcpServer tcpServer;
std::string peerIp;
int connId;
int listenFd;
```

---

## 5. 你当前项目建议采用的命名映射
把你文档里的类名映射为文件名：

1. `AppServer` -> `app_server.h/.cc`
2. `ServerConfig` -> `server_config.h/.cc`
3. `ProtocolCodec` -> `protocol_codec.h/.cc`
4. `SessionManager` -> `session_manager.h/.cc`
5. `AuthService` -> `auth_service.h/.cc`
6. `RateLimiter` -> `rate_limiter.h/.cc`
7. `BanEventPublisher` -> `ban_event_publisher.h/.cc`
8. `MetricsCollector` -> `metrics_collector.h/.cc`
9. `AdminCommandHandler` -> `admin_command_handler.h/.cc`

---

## 6. 反例（避免）
1. `Doit()`、`Process2()`：语义不清。
2. `m_loop`、`_loop`、`loop__`：成员变量风格混乱。
3. `GetIP()`、`SetID()`：缩写大小写不统一。
4. `CB`、`Func1`：回调语义缺失。
5. 同项目同时使用 `snake_case` 和 `camelCase` 函数名。

---

## 7. 最小落地检查清单
1. 类名是否都是 `PascalCase`。
2. 成员变量是否统一 `_` 后缀。
3. 回调是否统一 `setXxxCallback/onXxx/handleXxx`。
4. 文件名是否统一风格（推荐 `snake_case`）。
5. 缩写词写法是否一致（`Tcp/Ip/Id/Fd`）。

