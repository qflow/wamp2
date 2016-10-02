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
}
#endif
