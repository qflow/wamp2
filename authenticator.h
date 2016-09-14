#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "symbols.h"
#include <tuple>

namespace qflow{

template<typename T>
class authenticator
{
public:
    static constexpr const char* KEY = "unknown";
};

template<>
class authenticator<std::tuple<const char*, const char*>>
{
public:
    static constexpr const char* KEY = KEY_WAMPCRA;
    authenticator()
    {
    }
    using user = std::tuple<const char*, const char*>;
    authenticator(user& u) : _user(u)
    {
    }
    authenticator(const authenticator& other)  = delete;
    authenticator(authenticator&& other) : _user(std::move(other._user))
    {

    }
    authenticator& operator=(authenticator&& other)
    {
        _user = other._user;
    }
private:
    user _user;
};
}
#endif