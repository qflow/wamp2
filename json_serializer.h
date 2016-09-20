#ifndef JSON_SERIALIZER_H
#define JSON_SERIALIZER_H

#include "util/any.h"
#include "util/adapters.h"
#include "util/for_each_t.h"
#include "symbols.h"
#include <forward_list>
#include <nlohmann/json.hpp>
#include <cassert>

using linb::any;
using linb::any_cast;

using namespace nlohmann;

namespace qflow{

class msgpack_serializer
{
public:
    static constexpr const char* KEY = KEY_WAMP_MSGPACK_SUB;
    using variant_type = json;
    msgpack_serializer()
    {

    }
    template<typename message_type>
    std::string serialize(message_type message)
    {
        json j = message;
        return j.dump();
    }
    auto deserialize(std::string m)
    {
        return json::parse(m);
    }
};

}
#endif
