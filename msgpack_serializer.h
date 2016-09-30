#ifndef MSGPACK_SERIALIZER_H
#define MSGPACK_SERIALIZER_H

#include "util/any.h"
#include "util/adapters.h"
#include "util/for_each_t.h"
#include "symbols.h"
#include <forward_list>
#include <msgpack.hpp>
#include <cassert>
#include <vector>
#include <unordered_map>



template <typename T>
struct map_traits
{};
template<typename Key, typename... Value>
struct map_traits<std::tuple<std::pair<Key, Value>...>>
{
    static constexpr bool is_map = true;
};

template <typename T>
struct tuple_traits
{};

template<typename... T>
struct tuple_traits<std::tuple<T...>>
{
    static constexpr bool is_tuple = true;
};


namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {
template<>
struct pack<any> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, any const& a) const;
};
template<>
struct pack<std::vector<any>> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, std::vector<any> const& arr) const {
        o.pack_array(static_cast<uint32_t>(arr.size()));
        for(any a: arr)
        {
            o.pack(a);
        }
        return o;
    }
};
template<>
struct pack<std::unordered_map<std::string, any>> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, std::unordered_map<std::string, any> const& map) const {
        o.pack_map(static_cast<uint32_t>(map.size()));
        for(const auto& n: map)
        {
            o.pack(n.first);
            o.pack(n.second);
        }
        return o;
    }
};
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
        o.pack_map(std::tuple_size<T>::value);
        auto res = for_each_t(map, [&o](auto i, auto p){
            o.pack(std::get<0>(p));
            o.pack(std::get<1>(p));
            return 0;
        });
        return o;
    }
};
template<>
struct convert<std::vector<any>> {
    msgpack::object const& operator()(msgpack::object const& o, std::vector<any>& v) const;
};
template<>
struct convert<std::unordered_map<std::string,any>> {
    msgpack::object const& operator()(msgpack::object const& o, std::unordered_map<std::string,any>& v) const;
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
struct adapter<T, any>
{
    static T convert(any a)
    {
        return any_cast<T>(a);
    }
};
template<typename T>
struct adapter<any, T>
{
    static any convert(T t)
    {
        return any(t);
    }
};
template<>
struct adapter<msgpack::object, any>
{
    static msgpack::object convert(any a)
    {
        return msgpack::object(a);
    }
};
template<>
struct adapter<any, msgpack::object>
{
    static any convert(msgpack::object o);
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
template<typename... T>
struct adapter<any, std::tuple<T...>>
{
    static any convert(const std::tuple<T...>& t)
    {
        using Tuple = std::tuple<T...>;
        std::vector<any> list(std::tuple_size<Tuple>::value);
        auto res = for_each_t(t, [&list](auto idx, auto element){
            list[idx.value] = adapters::as<any>(element);
            return 0;
        });
        return any(list);
    }
};
template<>
struct adapter<any, std::shared_ptr<msgpack::object_handle>>
{
    static any convert(std::shared_ptr<msgpack::object_handle> p)
    {
        msgpack::object o = p->get();
        return adapters::as<any>(o);
    }
};
template<>
struct adapter<msgpack::object, qflow::WampMsgCode>
{
    static msgpack::object convert(qflow::WampMsgCode code);
};
template<>
struct adapter<unsigned long long, any>
{
    static unsigned long long convert(any a);
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
struct adapter<qflow::WampMsgCode, msgpack::object>
{
    static qflow::WampMsgCode convert(msgpack::object o)
    {
        return static_cast<qflow::WampMsgCode>(o.as<int>());
    }
};
template<>
struct adapter<qflow::WampMsgCode, any>
{
    static qflow::WampMsgCode convert(any a)
    {
        unsigned int t=any_cast<unsigned int>(a);
        return static_cast<qflow::WampMsgCode>(t);
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
