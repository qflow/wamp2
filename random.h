#ifndef RANDOM_H
#define RANDOM_H

#include <random>

namespace qflow{

using rnd_type = unsigned long long;
class random
{
public:
    static rnd_type generate()
    {
        return d(e);
    }

private:
    static std::default_random_engine e;
    static std::uniform_int_distribution<rnd_type> d;
};
std::default_random_engine random::e{std::random_device{}()};
std::uniform_int_distribution<rnd_type> random::d{1, static_cast<rnd_type>(1E8)};
}
#endif
