#include "authenticator.h"
#include "connection.h"
#include "msgpack_serializer.h"
#include "websocket_transport.h"
#include "router.h"
#include "http_server.h"
#include "translate.h"

#include <unordered_map>
#include <iostream>
#include <cassert>
#include <functional>
#include <thread>

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    Proxy proxy_config;
    proxy_config.set_address("0.0.0.0");
    proxy_config.set_port("1234");
    (*proxy_config.mutable_uri_translations())["google"] = "http://www.google.com";
    (*proxy_config.mutable_uri_translations())["buck"] = "http://video.webmfiles.org/big-buck-bunny_trailer.webm";
    std::string s = translate("https://www.google.cz/?gfe_rd=cr&ei=zLt_WJbJB9Sv8wfi16uwCg", 
                          *proxy_config.mutable_uri_translations());
    
    std::string out;
    google::protobuf::TextFormat::PrintToString(proxy_config, &out);
    std::cout << out;
    
    
    qflow::uri u("http://localhost.com/res?param=val&param2=val2");
    boost::asio::io_service io_service;
    qflow::tcp_server<qflow::http_session>(io_service, proxy_config.address(), proxy_config.port())(proxy_config);
    io_service.run();


    /*qflow::client c;
    c.init_asio();
    auto user = std::make_tuple("gemport", "gemport");
    qflow::wampcra_authenticator client_auth(user);
    auto callee = qflow::get_session<qflow::msgpack_serializer>(c, "ws://localhost:8083", "realm1", client_auth);
    auto caller = qflow::get_session<qflow::msgpack_serializer>(c, "ws://localhost:8083", "realm1", client_auth);

    callee->set_on_connected([callee, caller]() {
        auto f = callee->add_registration("add", [](int a, int b) {
            return a + b;
        });
        f.then([caller]() {
            caller->set_on_connected([caller]() {
                auto res_fut = caller->call<int>("add", 1 ,2);
                res_fut.then([](int res) {
                    std::cout << res;
                });
            });
            caller->connect();
        });
    });
    callee->connect();


    c.run();*/
    int r=0;
}
