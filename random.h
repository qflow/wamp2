#ifndef RANDOM_H
#define RANDOM_H

#include <random>

namespace qflow{

using id_type = unsigned long long;
class random
{
public:
    static id_type generate()
    {
        return d(e);
    }

private:
    static std::default_random_engine e;
    static std::uniform_int_distribution<id_type> d;
};
std::default_random_engine random::e{std::random_device{}()};
std::uniform_int_distribution<id_type> random::d{1, static_cast<id_type>(1E8)};
}
#endif
