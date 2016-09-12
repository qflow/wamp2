#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <type_traits>
#include "util/function_traits.h"
#include <iostream>

namespace qflow{
class registration
{

};
template<typename T, typename Enable = void>
class registration_impl : public registration
{

};
template<typename T>
class registration_impl<T, std::enable_if_t<function_traits<T>::is_callable>>
{
public:
    registration_impl(T&& f) : _func(f)
    {
    }
    void test()
    {

    }
private:
    T _func;
};

}

#endif
