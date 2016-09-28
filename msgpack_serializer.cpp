#include "msgpack_serializer.h"

namespace qflow{

}
namespace adapters{
any adapter<any, msgpack::object>::convert(msgpack::object o)
{
    if(o.type == msgpack::type::BOOLEAN)
    {
        return any(o.as<bool>());
    }
    else if(o.type == msgpack::type::STR)
    {
        return any(o.as<std::string>());
    }
    else if(o.type == msgpack::type::POSITIVE_INTEGER)
    {
        unsigned int i = o.as<unsigned int>();
        return any(i);
    }
    else if(o.type == msgpack::type::NEGATIVE_INTEGER)
    {
        return any(o.as<int>());
    }
    else if(o.type == msgpack::type::FLOAT)
    {
        return any(o.as<float>());
    }
    else if(o.type == msgpack::type::BIN)
    {
        std::string s(o.via.bin.ptr, o.via.bin.size);
        return any(s);
    }
    else if(o.type == msgpack::type::ARRAY)
    {
        return any(o.as<std::vector<any>>());
    }
    else if(o.type == msgpack::type::MAP)
    {
        return any(o.as<std::unordered_map<std::string, any>>());
    }
    else
    {
        assert(false);
    }
    return any();
}
msgpack::object adapter<msgpack::object, qflow::WampMsgCode>::convert(qflow::WampMsgCode code)
{
    return msgpack::object(static_cast<int>(code));
}
unsigned long long adapter<unsigned long long, any>::convert(any a)
{
    unsigned int t=any_cast<unsigned int>(a);
    return static_cast<unsigned long long>(t);
}
}
