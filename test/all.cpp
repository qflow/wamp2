#include "authenticator.h"
#include "connection.h"
#include "router.h"
#include <iostream>
#include <cassert>
#include <functional>


int main()
{
    using serializers = std::tuple<qflow::msgpack_serializer>;
    auto ws_transport = std::make_shared<qflow::websocket_transport<serializers>>(8083);
    qflow::router<std::tuple<qflow::wampcra_authenticator>> router;
    router.add_transport(ws_transport);



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
    auto user = std::make_tuple("gemport", "gemport");
    qflow::wampcra_authenticator auth(user);
    auto callee = qflow::get_session<qflow::msgpack_serializer>(c, "ws://localhost:8083", "realm1", auth);
    auto caller = qflow::get_session<qflow::msgpack_serializer>(c, "ws://40.217.1.146:8080", "realm1", auth);

    callee->set_on_connected([callee, caller](){
        auto f = callee->add_registration("add", [](int a, int b){
            return a + b;
        });
        f.then([caller](){
            caller->set_on_connected([caller](){
                auto res_fut = caller->call<int>("add", 1 ,2);
                res_fut.then([](int res){
                    int u=0;
                });
            });
            caller->connect();
        });
    });
    callee->connect();


    c.run();
    int r=0;
}
