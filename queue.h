#ifndef SIGNAL_WRAPPER_H
#define SIGNAL_WRAPPER_H

#include <memory>
#include <mutex>
#include <queue>
#include <atomic>
#include <vector>
#include <deque>
#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>
#include <beast/core/async_completion.hpp>
#include <boost/lockfree/queue.hpp>


namespace qflow {
template<typename T>
class queue
{
public:
    queue()
    {

    }
    void push(T element)
    {
        while (lock_.test_and_set(std::memory_order_acquire));
        vec_.push_back(element);
        lock_.clear();
    }
    std::vector<T> pull()
    {
        while (lock_.test_and_set(std::memory_order_acquire));
        auto res = vec_;
        vec_.clear();
        lock_.clear();
        return res;
    }
    void flush()
    {
        while (lock_.test_and_set(std::memory_order_acquire));
        vec_.clear();
        lock_.clear();
    }
private:
    std::vector<T> vec_;
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
};

template<typename T>
class async_queue
{
public:
    async_queue(boost::asio::io_service& io_service) : io_service_(io_service), strand_(io_service)
    {

    }
    void async_push(T element)
    {
        boost::asio::spawn(strand_,
                           [this, element](boost::asio::yield_context)
        {
            boost::system::error_code ec;
            completed_.clear();
            vec_.push_back(element);
            if(!handlers_.empty())
            {
                for(auto handler: handlers_)
                {
                    std::vector<T> vec_copy = vec_;
                    boost::asio::asio_handler_invoke(std::bind(handler, ec, std::move(vec_copy)), &handler);
                    completed_.push_back(handler);
                }
                vec_.clear();
                handlers_.swap(completed_);
            }

        });
    }
    template<class CompletionToken>
    auto async_pull(CompletionToken&& token)
    {
        response_completion completion(std::forward<CompletionToken>(token));
        boost::asio::spawn(strand_,
                           [this, handler = completion.handler](boost::asio::yield_context)
        {
            handlers_.push_back(std::move(handler));
        });
        return completion.result.get();
    }
private:
    using response_completion = beast::async_completion<boost::asio::yield_context, void(boost::system::error_code, std::vector<T>)>;
    using handler_type = typename response_completion::handler_type;
    std::vector<T> vec_;
    boost::asio::io_service& io_service_;
    boost::asio::io_service::strand strand_;
    std::vector<handler_type> handlers_;
    std::vector<handler_type> completed_;
};
}
#endif
