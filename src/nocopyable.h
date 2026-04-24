#pragma once
class nocopyable{
public:
    nocopyable& operator=(const nocopyable&) = delete;
    nocopyable(const nocopyable&) = delete;
protected:
    nocopyable() = default;
    ~nocopyable() = default;
};
