#include "authenticator.h"
#include "connection.h"
#include "router.h"
#include <unordered_map>
#include <iostream>
#include <cassert>
#include <functional>

int main()
{
    using serializers = std::tuple<qflow::msgpack_serializer>;

    std::unordered_map<std::string, std::string> credentials = {{"gemport", "gemport"}};
    auto wampcra = std::make_shared<qflow::wampcra_authenticator>(credentials);
    qflow::websocket_transport<serializers> ws_transport(8083);
    qflow::router router;
    router.add_transport(&ws_transport);
    router.add_authenticator(wampcra);


    qflow::client c;
    c.init_asio();
    auto user = std::make_tuple("gemport", "gemport");
    qflow::wampcra_authenticator client_auth(user);
    auto callee = qflow::get_session<qflow::msgpack_serializer>(c, "ws://localhost:8083", "realm1", client_auth);
    auto caller = qflow::get_session<qflow::msgpack_serializer>(c, "ws://localhost:8083", "realm1", client_auth);

    callee->set_on_connected([callee, caller](){
        auto f = callee->add_registration("add", [](int a, int b){
            return a + b;
        });
        f.then([caller](){
            caller->set_on_connected([caller](){
                auto res_fut = caller->call<int>("add", 1 ,2);
                res_fut.then([](int res){
                    std::cout << res;
                });
            });
            caller->connect();
        });
    });
    callee->connect();


    c.run();
    int r=0;
}
