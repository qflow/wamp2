#include "connection.h"
#include <iostream>
#include <cassert>
#include <functional>


int main()
{

    auto map = std::make_tuple(std::make_pair("key", "value"));
    auto obj = std::make_tuple(std::string("ahoj"), 5, map);
    qflow::msgpack_serializer serializer;
    std::string ss = serializer.serialize(obj);
    auto des = serializer.deserialize(ss);
    std::vector<msgpack::object> arr = adapters::as<std::vector<msgpack::object>>(des);
    std::string s = adapters::as<std::string>(arr[0]);
    assert(s == std::string("ahoj"));


    qflow::client c;
    c.init_asio();
    std::tuple<const char*, const char*> user = std::make_tuple("gemport", "gemport");
    auto session = qflow::get_session<qflow::msgpack_serializer>(c, "ws://40.217.1.146:8080", "realm1", user);
    session->add_registration("test", [](){
        
    });
    session->connect();


    c.run();
    int t=0;
}
