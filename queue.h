#ifndef SIGNAL_WRAPPER_H
#define SIGNAL_WRAPPER_H

#include <mutex>
#include <queue>
#include <atomic>
#include <vector>


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
}
#endif
