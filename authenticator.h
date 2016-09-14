#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "symbols.h"
#include <tuple>

namespace qflow{

template<typename T, typename Enable = void>
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
private:
    user _user;
};
}
#endif
