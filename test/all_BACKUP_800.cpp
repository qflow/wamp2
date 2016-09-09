#include "connection.h"
#include <vector>
#include <string>
#include <iostream>
#include <cassert>

int main()
{
    //qflow::connection c;

    auto obj = std::make_tuple(std::string("ahoj"), 5);
    qflow::msgpack_serializer serializer;
    std::string ss = serializer.serialize(obj);
    auto des = serializer.deserialize(ss);
    std::vector<msgpack::object> arr = adapters::as<std::vector<msgpack::object>>(des);
    std::string s = adapters::as<std::string>(arr[0]);
    assert(s == std::string("ahoj"));
<<<<<<< HEAD
=======


    qflow::client c;
    c.init_asio();
    websocketpp::lib::error_code ec;
    auto conn = c.get_connection("ws://40.217.1.146:8080", ec);

    auto future = qflow::connect<qflow::msgpack_serializer>(c, "ws://40.217.1.146:8080");
    future.then([](auto session){
        if(session->state() == qflow::SESSION_STATE::OPENED)
        {
            int t=0;
        }
    });
    c.run();
>>>>>>> 0c70a73c6c4b8efd66113cd9db43958c67d0bb49
}
