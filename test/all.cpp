#include "authenticator.h"
#include "connection.h"
#include "router.h"
#include <unordered_map>
#include <iostream>
#include <cassert>
#include <functional>


class A
{
public:
    virtual int get() = 0;
};

class B: public A
{
public:
    int val = 5;
    int get()
    {
        return val;
    }
    B()
    {

    }

    B(const B& other) : val(other.val)
    {

    }
};

int main()
{
    B b;
    A* b2 = static_cast<A*>(new B(b));
    int i = b2->get();



    using serializers = std::tuple<qflow::msgpack_serializer>;

    std::unordered_map<std::string, std::string> credentials = {{"gemport", "gemport"}};
    qflow::wampcra_authenticator<decltype(credentials)> wampcra(credentials);
    auto authenticators = std::make_tuple(wampcra);
    auto ws_transport = std::make_shared<qflow::websocket_transport<serializers>>(8083);
    qflow::router<decltype(authenticators)> router(authenticators);
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
    qflow::wampcra_authenticator<std::unordered_map<std::string, std::string>> client_auth(user);
    auto callee = qflow::get_session<qflow::msgpack_serializer>(c, "ws://localhost:8083", "realm1", client_auth);
    auto caller = qflow::get_session<qflow::msgpack_serializer>(c, "ws://40.217.1.146:8080", "realm1", client_auth);

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
