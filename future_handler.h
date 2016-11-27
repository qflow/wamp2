#ifndef FUTURE_HANDLER_H
#define FUTURE_HANDLER_H

#include "future.h"
#include <boost/asio/handler_type.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/async_result.hpp>
#include <tuple>

namespace qflow {
constexpr struct use_future_t {
    constexpr use_future_t() {}
} use_future;

void unpack(std::tuple<>) {}
template<typename U>
U unpack(std::tuple<U> t)
{
    return std::move(std::get<0>(t));
}
template<typename... U>
std::tuple<U...> unpack(std::tuple<U...> u)
{
    return u;
}

template<typename... T>
class future_handler
{
public:
    explicit future_handler(use_future_t) : p(new promise<value_type>()) {}
    using value_type = decltype(unpack(std::declval<std::tuple<T...>>()));
    template<class... Args>
    void operator()(Args&&... args)
    {
        p->set_value(unpack(std::make_tuple(std::forward<Args>(args)...)));
    }
    std::shared_ptr<promise<value_type>> p;
};

/*template<class... T>
class future_handler<boost::system::error_code, T...>
{
public:
    explicit future_handler(use_future_t) : p(new promise<value_type>()) {}
    using value_type = std::tuple<T...>;
    template<class... Args>
    void operator()(boost::system::error_code e, Args&&... args)
    {
        if(e) p->set_exception(std::make_exception_ptr(boost::system::system_error(e)));
        else
            p->set_value(value_type(std::forward<Args>(args)...));
    }
    std::shared_ptr<promise<value_type>> p;
};*/
}
namespace boost {
namespace asio {

template<class R, class... Args>
struct handler_type<qflow::use_future_t, R(Args...)>
{
    using type = qflow::future_handler<std::decay_t<Args>...>;
};
template<class... T>
class async_result<qflow::future_handler<T...>>
{
    using value_type = typename qflow::future_handler<T...>::value_type;
public:
    using type = qflow::future<value_type>;
    type f;
    explicit async_result(qflow::future_handler<T...>& h) : f(h.p->get_future())    {}
    type get() {
        return std::move(f);
    }
};
}
}
#endif
