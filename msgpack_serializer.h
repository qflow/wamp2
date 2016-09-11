#ifndef MSGPACK_SERIALIZER_H
#define MSGPACK_SERIALIZER_H

#include "util/adapters.h"
#include "symbols.h"
#include <msgpack.hpp>


namespace adapters
{
    template<typename T>
    struct adapter<T, msgpack::object>
    {
        static T convert(const msgpack::object& o)
        {
            return o.as<T>();
        }
    };
    template<typename T>
    struct adapter<T, std::shared_ptr<msgpack::object_handle>>
    {
        static T convert(std::shared_ptr<msgpack::object_handle> p)
        {
            msgpack::object o = p->get();
            return o.as<T>();
        }
    };
    template<typename T>
    struct adapter<msgpack::object, T>
    {
        static msgpack::object convert(T t)
        {
            return msgpack::object(t);
        }
    };
}

namespace qflow{

class msgpack_serializer
{
public:
    static constexpr const char* KEY = KEY_WAMP_MSGPACK_SUB;
    using variant_type = msgpack::object;
    msgpack_serializer()
    {

    }
    template<typename message_type>
    std::string serialize(message_type message)
    {
        std::stringstream buffer;
        msgpack::pack(buffer, message);
        return buffer.str();
    }
    auto deserialize(std::string m)
    {
        auto p = std::make_shared<msgpack::object_handle>();
        msgpack::unpack(*p.get(), m.data(), m.size());
        return p;
    }
};

}
#endif
