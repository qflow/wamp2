#ifndef WAMP_ROUTER_H
#define WAMP_ROUTER_H

#include "random.h"
#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/error.hpp>
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

enum class wamp_errors
{
    error = 2
};

class wamp_category_impl :
    public boost::system::error_category
{
public:
    const char *name() const noexcept {
        return "wamp";
    }
    std::string message(int ev) const
    {
        switch(static_cast<wamp_errors>(ev))
        {
        case wamp_errors::error :
            return "wamp error";
        }
        return "undefined";
    }
};
static const boost::system::error_category& wamp_category = qflow::wamp_category_impl();
boost::system::error_code make_error_code(qflow::wamp_errors e)
{
    return boost::system::error_code(
               static_cast<int>(e), wamp_category);
}

template<typename NextStream, typename Serializer>
class wamp_stream
{
public:
    wamp_stream(NextStream next) : next_(next)
    {

    }
    template<class CompletionToken>
    auto async_handshake(CompletionToken&& token)
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
                Serializer s;
                std::string str = s.serialize(msg);
                next_.async_write(boost::asio::buffer(str), yield);
                beast::streambuf sb;
                beast::websocket::opcode op;
                next_.async_read(op, sb, yield);
                auto m = s.deserialize(beast::to_string(sb.data()));
                using map = std::unordered_map<std::string, typename Serializer::variant_type>;
                using array = std::vector<typename Serializer::variant_type>;
                array arr = adapters::as<array>(m);
                WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
                if(code == WampMsgCode::WELCOME) listen();
                else ec = make_error_code(wamp_errors::error);
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec), &handler);
        });
        return completion.result.get();
    }
    template<class CompletionToken, class Callable>
    auto async_register(const std::string uri, Callable&& c, CompletionToken&& token)
    {
        beast::async_completion<CompletionToken, void(boost::system::error_code)> completion(token);
        boost::asio::spawn(next_.get_io_service(),
                           [this, uri, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            try {
                id_type requestId = random::generate();
                auto msg = std::make_tuple(WampMsgCode::REGISTER, requestId, empty(), uri);
                Serializer s;
                std::string str = s.serialize(msg);
                next_.async_write(boost::asio::buffer(str), yield);
                //beast::async_completion<boost::asio::yield_context, void(boost::system::error_code)> response(std::forward<boost::asio::yield_context>(yield[ec]));
                boost::asio::io_service::work work(next_.get_io_service());
                boost::asio::detail::async_result_init<const boost::asio::yield_context&, void(boost::system::error_code)> init(yield[ec]);//BOOST_ASIO_MOVE_CAST(boost::asio::yield_context)
                init.result.get();
                
                int t=0;
                
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec), &handler);
        });
        return completion.result.get();
    }

private:
    NextStream next_;
    //std::unordered_map<id_type, pending_reg> _pending_requests;
    void listen()
    {
        boost::asio::spawn(next_.get_io_service(),
                           [this](boost::asio::yield_context yield)
        {
            for (;;)
            {
                Serializer s;
                beast::streambuf sb;
                beast::websocket::opcode op;
                next_.async_read(op, sb, yield);
                auto m = s.deserialize(beast::to_string(sb.data()));
                using map = std::unordered_map<std::string, typename Serializer::variant_type>;
                using array = std::vector<typename Serializer::variant_type>;
                array arr = adapters::as<array>(m);
                WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
                int requestId = -1;
                if(code == WampMsgCode::WAMP_ERROR)
                {
                    requestId = adapters::as<id_type>(arr[2]);
                }
                
            }
        });
    }
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
                wamp_stream<beast::websocket::stream<stream_type&>&, msgpack_serializer> wamp {ws};
                wamp.async_handshake(yield);
                wamp.async_register("test.add", [](){}, yield);

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
