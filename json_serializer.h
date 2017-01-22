#ifndef JSON_SERIALIZER_H
#define JSON_SERIALIZER_H

#include "util/adapters.h"
#include "util/for_each_t.h"
#include "symbols.h"
#include <forward_list>
#include <nlohmann/json.hpp>
#include <cassert>
#include <experimental/any>

using namespace nlohmann;

namespace adapters{

template<>
struct adapter<json, empty>
{
    static json convert(empty)
    {
        return json::object();
    }
};

template<typename T>
struct adapter<json, T>
{
    static json convert(T t)
    {
        return json(t);
    }
};
template<>
struct adapter<json, qflow::WampMsgCode>
{
    static json convert(qflow::WampMsgCode code)
    {
        return json(static_cast<int>(code));
    }
};

template<typename Key, typename... Value>
struct adapter<json, std::tuple<std::pair<Key, Value>...>>
{
    static json convert(const std::tuple<std::pair<Key, Value>...>& map)
    {
        json j = json::object();
        auto res = for_each_t(map, [&](auto i, auto p){
            j[std::get<0>(p)] = adapters::as<json>(std::get<1>(p));
            return 0;
        });
        return j;
    }
};
/*template<typename T>
struct adapter<json, T, std::enable_if_t<map_traits<T>::is_map>> 
{
    static json convert(T const& map) {
        json j = json::object();
        auto res = for_each_t(map, [&](auto i, auto p){
            j[std::get<0>(p)] = adapters::as<json>(std::get<1>(p));
            return 0;
        });
        return j;
    }
};*/


template<typename... T>
struct adapter<json, std::tuple<T...>>
{
    static json convert(const std::tuple<T...>& t)
    {
        using Tuple = std::tuple<T...>;
        std::vector<json> list(std::tuple_size<Tuple>::value);
        auto res = for_each_t(t, [&list](auto idx, auto element){
            list[idx.value] = adapters::as<json>(element);
            return 0;
        });
        return json(list);
    }
};
}

namespace qflow{

class json_serializer
{
public:
    static constexpr const char* KEY = KEY_WAMP_MSGPACK_SUB;
    using variant_type = json;
    json_serializer()
    {

    }
    template<typename message_type>
    std::string serialize(message_type message)
    {
        json j = adapters::as<json>(message);
        return j.dump();
    }
    auto deserialize(std::string m)
    {
        return json::parse(m);
    }
};

}
#endif
