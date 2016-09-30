#include "msgpack_serializer.h"
#include <vector>
#include <string>
#include <sstream>

namespace qflow{

}
namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
    namespace adaptor {
    msgpack::object const& convert<std::vector<any>>::operator()(msgpack::object const& o, std::vector<any>& v) const {
        std::vector<any> list;
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        for(unsigned int i=0; i<o.via.array.size; i++)
        {
            any a = adapters::as<any>(o.via.array.ptr[i]);
            list.push_back(a);
        }
        v = list;
        return o;
    }
    msgpack::object const& convert<std::unordered_map<std::string,any>>::operator()(msgpack::object const& o, std::unordered_map<std::string,any>& v) const {
        std::unordered_map<std::string,any> map;
        if (o.type != msgpack::type::MAP) throw msgpack::type_error();
        for(unsigned int i=0; i<o.via.map.size; i++)
        {
            msgpack::object_kv pair = o.via.map.ptr[i];
            std::string key = adapters::as<std::string>(pair.key);
            any value = adapters::as<any>(pair.val);
            map[key] = value;
        }
        v = map;
        return o;
    }
    template <typename Stream>
    packer<Stream>& pack<any>::operator()(msgpack::packer<Stream>& o, any const& a) const {
        if(a.type() == typeid(bool))
        {
            o.pack(any_cast<bool>(a));
        }
        else if(a.type() == typeid(std::string))
        {
            o.pack(any_cast<std::string>(a));
        }
        else if(a.type() == typeid(const char*))
        {
            o.pack(any_cast<const char*>(a));
        }
        else if(a.type() == typeid(qflow::WampMsgCode))
        {
            o.pack(any_cast<qflow::WampMsgCode>(a));
        }
        else if(a.type() == typeid(int))
        {
            o.pack(any_cast<int>(a));
        }
        else if(a.type() == typeid(unsigned long long))
        {
            o.pack(any_cast<unsigned long long>(a));
        }
        else if(a.type() == typeid(unsigned int))
        {
            o.pack(any_cast<unsigned int>(a));
        }
        else if(a.type() == typeid(float))
        {
            o.pack(any_cast<float>(a));
        }
        else if(a.type() == typeid(double))
        {
            o.pack(any_cast<double>(a));
        }
        else if(a.type() == typeid(std::vector<any>))
        {
            o.pack(any_cast<std::vector<any>>(a));
        }
        else if(a.type() == typeid(std::unordered_map<std::string, any>))
        {
            o.pack(any_cast<std::unordered_map<std::string, any>>(a));
        }
        else
        {
            auto name = a.type().name();
            assert(false);
        }
        return o;
    }
    template packer<std::basic_stringstream<char>>& pack<any>::operator()(msgpack::packer<std::basic_stringstream<char>>& o, any const& a) const;
    }
}
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
