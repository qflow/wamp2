#ifndef MSGPACK_SERIALIZER_H
#define MSGPACK_SERIALIZER_H

#include "util/adapters.h"
#include "util/for_each_t.h"
#include "symbols.h"
#include <forward_list>
#include <msgpack.hpp>

template <typename T>
struct map_traits
{};

template<typename Key, typename... Value>
struct map_traits<std::tuple<std::pair<Key, Value>...>>
{
    static constexpr bool is_map = true;
};


namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {
template<>
struct pack<qflow::WampMsgCode> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, qflow::WampMsgCode const& code) const {
        o.pack(static_cast<int>(code));
        return o;
    }
};
template<typename T>
struct pack<T, std::enable_if_t<map_traits<T>::is_map>> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, T const& map) const {
        auto res = for_each_t(map, [](auto i, auto p){
            int r=0;
            return 0;
        });
        return o;
    }
};
}
}
}



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
    template<>
    struct adapter<msgpack::object, qflow::WampMsgCode>
    {
        static msgpack::object convert(qflow::WampMsgCode code)
        {
            return msgpack::object(static_cast<int>(code));
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
