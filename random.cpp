#include "random.h"
namespace qflow{

void testx()
{
    if constexpr(true)
    {

    }
}

std::default_random_engine random::e{std::random_device{}()};
std::uniform_int_distribution<id_type> random::d{1, static_cast<id_type>(1E8)};
}
