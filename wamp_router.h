#ifndef WAMP_ROUTER_H
#define WAMP_ROUTER_H

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/spawn.hpp>
#include <beast/core/to_string.hpp>
#include <beast/websocket.hpp>
#include <beast/websocket/option.hpp>
#include <beast/websocket/ssl.hpp>
#include <iostream>
#include <string>
#include "http_server.h"
#include "json_serializer.h"
#include "msgpack_serializer.h"
#include "symbols.h"

namespace qflow {


template<typename NextStream>
class wamp_stream
{
public:
    wamp_stream(NextStream next) : next_(next)
    {

    }
    template<class CompletionToken>
    auto handshake(CompletionToken&& token)
    {
        beast::async_completion<CompletionToken, void(boost::system::error_code)> completion(token);
        boost::asio::spawn(next_.get_io_service(),
                           [this, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            try {
                auto roles = std::make_tuple(std::make_pair("caller", empty()), std::make_pair("subscriber", empty()));
                auto details = std::make_tuple(std::make_pair("roles", roles));
                auto msg = std::make_tuple(WampMsgCode::HELLO, "crossbardemo", details);
                msgpack_serializer s;
                json_serializer js;
                auto str2 = js.serialize(msg);
                auto str = s.serialize(msg);
                //str = "[1, \"crossbardemo\", {\"roles\": {\"caller\": {},\"subscriber\": {}}}]";
                std::cout << str;
                next_.async_write(boost::asio::buffer(str), yield);
                beast::streambuf sb;
                beast::websocket::opcode op;
                next_.async_read(op, sb, yield);
                std::string des = beast::to_string(sb.data());
                int t=0;
                /*ws.async_write(boost::asio::buffer(std::string("Hello, world!")), yield);
                beast::streambuf sb;
                beast::websocket::opcode op;
                ws.async_read(op, sb, yield);
                ws.async_close(beast::websocket::close_code::normal, yield);
                std::string s = beast::to_string(sb.data());*/
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
                auto message = e.what();
                std::cout << message << "\n\n";
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec), &handler);
        });
        return completion.result.get();
    }

private:
    NextStream next_;
};

struct protocol
{
    std::string str;
    protocol(const std::string& s) : str(s)
    {
    }
    template<class Body, class Fields>
    void operator()(beast::http::message<true, Body, Fields>& req)
    {
        req.fields.insert("Sec-WebSocket-Protocol", str);
    }

    template<class Body, class Fields>
    void operator()(beast::http::message<false, Body, Fields>& res)
    {
    }
};

class wamp_server_session : public std::enable_shared_from_this<wamp_server_session>
{
public:
    explicit wamp_server_session(boost::asio::ip::tcp::socket socket)
        : socket_(std::move(socket)), strand_(socket_.get_io_service())
    {
    }
    void operator()()
    {
        auto self(shared_from_this());
        boost::asio::spawn(strand_,
                           [this, self](boost::asio::yield_context yield)
        {
            try
            {
                std::string const host = "demo.crossbar.io";//demo.crossbar.io
                boost::asio::ip::tcp::resolver r {socket_.get_io_service()};
                boost::asio::ip::tcp::socket sock {socket_.get_io_service()};
                auto i = r.async_resolve(boost::asio::ip::tcp::resolver::query {host, "https"}, yield);
                boost::asio::async_connect(sock, i, yield);
                boost::asio::ssl::context ctx {boost::asio::ssl::context::sslv23};
                using stream_type = boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>;
                stream_type ssl_stream {sock, ctx};
                ssl_stream.async_handshake(boost::asio::ssl::stream_base::client, yield);
                beast::websocket::stream<stream_type&> ws {ssl_stream};
                ws.set_option(beast::websocket::decorate(protocol {KEY_WAMP_MSGPACK_SUB}));
                ws.set_option(beast::websocket::message_type {beast::websocket::opcode::binary});
                ws.async_handshake(host, "/ws",yield);
                wamp_stream<beast::websocket::stream<stream_type&>&> wamp {ws};
                wamp.handshake(yield);

                int t=0;

            }
            catch (boost::system::system_error& e)
            {
                auto message = e.what();
                std::cout << message << "\n\n";
            }
        });
    }
private:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand strand_;
};
using wamp_router = tcp_server<wamp_server_session>;

}
#endif
