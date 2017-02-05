#include "authenticator.h"
#include "connection.h"
#include "msgpack_serializer.h"
#include "websocket_transport.h"
#include "router.h"
#include "http_server.h"
#include "translate.h"
#include "wamp_router.h"

#include <unordered_map>
#include <iostream>
#include <cassert>
#include <functional>
#include <thread>
#include <boost/thread/thread.hpp>
#include <boost/asio/use_future.hpp>

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    Proxy proxy_config;
    proxy_config.set_address("0.0.0.0");
    proxy_config.set_port("1234");
    (*proxy_config.mutable_uri_translations())["google(.*)"] = "http://www.google.com/<1>";
    (*proxy_config.mutable_uri_translations())["buck"] = "http://video.webmfiles.org/big-buck-bunny_trailer.webm";
    
    std::string out;
    google::protobuf::TextFormat::PrintToString(proxy_config, &out);
    std::cout << out;


    boost::thread_group threadpool;
    boost::asio::io_service io_service;
    boost::asio::io_service::work work(io_service);
    for(int i=0; i<4; i++)
    {
        threadpool.create_thread(
            boost::bind(&boost::asio::io_service::run, &io_service)
        );
    }

    qflow::tcp_server<qflow::http_session>(io_service, proxy_config.address(), proxy_config.port())(proxy_config);
    qflow::wamp_router(io_service, "0.0.0.0", "5555")();
    boost::asio::spawn(io_service,
                           [&](boost::asio::yield_context yield)
        {
            try
            {
                std::string const host = "127.0.0.1";//demo.crossbar.io
                boost::asio::ip::tcp::resolver r {io_service};
                boost::asio::ip::tcp::socket sock {io_service};
                boost::asio::ip::tcp::socket sock2 {io_service};
                auto i = r.async_resolve(boost::asio::ip::tcp::resolver::query {host, "8080"}, yield);
                boost::asio::async_connect(sock, i, yield);
                boost::asio::async_connect(sock2, i, yield);
                //boost::asio::ssl::context ctx {boost::asio::ssl::context::sslv23};
                using stream_type = boost::asio::ip::tcp::socket;
                //tream_type ssl_stream {sock, ctx};
                //ssl_stream.async_handshake(boost::asio::ssl::stream_base::client, yield);
                beast::websocket::stream<stream_type&> ws {sock};
                ws.set_option(beast::websocket::decorate(qflow::protocol {qflow::KEY_WAMP_MSGPACK_SUB}));
                ws.set_option(beast::websocket::message_type {beast::websocket::opcode::binary});
                ws.async_handshake(host, "/ws",yield);
                qflow::wamp_stream<beast::websocket::stream<stream_type&>&, qflow::msgpack_serializer> wamp {ws};
                wamp.async_handshake("realm1", yield);
                
                beast::websocket::stream<stream_type&> ws2 {sock2};
                ws2.set_option(beast::websocket::decorate(qflow::protocol {qflow::KEY_WAMP_MSGPACK_SUB}));
                ws2.set_option(beast::websocket::message_type {beast::websocket::opcode::binary});
                ws2.async_handshake(host, "/ws",yield);
                qflow::wamp_stream<beast::websocket::stream<stream_type&>&, qflow::msgpack_serializer> wamp2 {ws2};
                wamp2.async_handshake("realm1", yield);
                
                qflow::id_type sub_id = wamp.async_subscribe("test.ev", yield);
                wamp2.async_publish("test.ev", false, yield, "ahoj");
                auto ev = wamp.async_receive_event<std::tuple<std::string>>(sub_id, yield);
                qflow::id_type reg_id = wamp.async_register("test.add", yield);
                boost::asio::spawn(io_service,
                           [&](boost::asio::yield_context yield2)
                {
                    wamp.async_handle_invocation(reg_id, [](int a, int b){
                        return a + b;
                    }, yield2);
                    int t=0;
                });
                auto res = wamp2.async_call_complete<std::tuple<int>>("test.add", yield, 1, 2);
                int t=0;

            }
            catch (const std::exception& e)
            {
                auto message = e.what();
                std::cout << message << "\n\n";
            }
        });
    
    
    threadpool.join_all();


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
