#include "authenticator.h"
#include "connection.h"
#include "msgpack_serializer.h"
#include "websocket_transport.h"
#include "router.h"
#include "http_server.h"
#include "uri.h"

#include <unordered_map>
#include <iostream>
#include <cassert>
#include <functional>
#include <thread>

int main()
{
    boost::asio::io_service io_service;
    qflow::http_server(io_service, "0.0.0.0", "1234", [&](auto req, auto res) {
        qflow::uri url(req->url);
        res->version = req->version;
        if(url.contains_query_item("code"))
        {
            int i=0;
        }
        else
        {
            res->status = 303;
            //res->reason = "Moved Permanently";
            res->fields.insert("Server", "http_async_server");
            res->fields.insert("Location", "https://www.facebook.com/v2.8/dialog/oauth?client_id=1067375046723087&redirect_uri=http://localhost:1234" + url.path());
            //res->fields.insert("Content-Type", "text/html");
            //res->body = "Hello";
        }
    })();
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


