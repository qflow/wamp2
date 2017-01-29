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
    error = 2,
    not_authorized = 3,
    no_such_procedure = 4,
    procedure_already_exists = 5,
    no_such_registration = 6
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
            return "wamp.error";
        case wamp_errors::not_authorized:
            return "wamp.error.not_authorized";
        case wamp_errors::no_such_procedure:
            return "wamp.error.no_such_procedure";
        case wamp_errors::procedure_already_exists:
            return "wamp.error.procedure_already_exists";
        case wamp_errors::no_such_registration:
            return "wamp.error.no_such_registration";
        }
        return "undefined";
    }
};
struct registration
{
    id_type registration_id;
};
static const boost::system::error_category& wamp_category = qflow::wamp_category_impl();
boost::system::error_code make_error_code(qflow::wamp_errors e)
{
    return boost::system::error_code(
               static_cast<int>(e), wamp_category);
}
boost::system::error_code make_error_code(const std::string& str)
{
    if(str == "wamp.error.not_authorized")
        return make_error_code(wamp_errors::not_authorized);
    else if(str == "wamp.error.no_such_procedure")
        return make_error_code(wamp_errors::no_such_procedure);
    else if(str == "wamp.error.procedure_already_exists")
        return make_error_code(wamp_errors::procedure_already_exists);
    else if(str == "wamp.error.no_such_registration")
        return make_error_code(wamp_errors::no_such_registration);

    std::cout << "Unhandled wamp error: " + str;
    return make_error_code(wamp_errors::error);
}

template<typename NextStream, typename Serializer>
class wamp_stream
{
public:
    wamp_stream(NextStream next) : next_(next), strand_(next.get_io_service())
    {

    }
    ~wamp_stream()
    {
        is_alive_ = false;
    }
    template<class CompletionToken>
    auto async_handshake(const std::string& realm, CompletionToken&& token)
    {
        beast::async_completion<CompletionToken, void(boost::system::error_code)> completion(token);
        boost::asio::spawn(next_.get_io_service(),
                           [this, realm, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            try {
                auto roles = std::make_tuple(std::make_pair("caller", empty()), std::make_pair("subscriber", empty()),
                                             std::make_pair("callee", empty()), std::make_pair("publisher", empty()));
                auto details = std::make_tuple(std::make_pair("roles", roles));
                auto msg = std::make_tuple(WampMsgCode::HELLO, realm, details);
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
                if(code != WampMsgCode::WELCOME) 
                    ec = make_error_code(wamp_errors::error);
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec), &handler);
        });
        return completion.result.get();
    }

    template<class CompletionToken, typename... Args>
    auto async_call(const std::string& uri, CompletionToken&& token, Args... args)
    {
        response_completion completion(token);
        auto argsTuple = std::make_tuple(args...);
        id_type requestId = random::generate();
        auto msg = std::make_tuple(WampMsgCode::CALL, requestId, empty(), uri, argsTuple);
        boost::asio::spawn(next_.get_io_service(),
                           [this, requestId, msg, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            array res;
            try {
                res = async_send_receive(requestId, msg, yield);
                int t=0;

            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec, res), &handler);
        });
        return completion.result.get();
    }
    template<class CompletionToken, class Callable>
    auto async_register(const std::string uri, Callable&& c, CompletionToken&& token)
    {
        beast::async_completion<CompletionToken, void(boost::system::error_code, id_type)> completion(token);
        boost::asio::spawn(next_.get_io_service(),
                           [this, uri, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            id_type registration_id;
            try {
                id_type requestId = random::generate();
                auto msg = std::make_tuple(WampMsgCode::REGISTER, requestId, empty(), uri);
                auto arr = async_send_receive(requestId, msg, yield);
                WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
                if(code == WampMsgCode::WAMP_REGISTERED)
                {
                    registration_id = adapters::as<id_type>(arr[2]);
                }
                else ec = make_error_code(wamp_errors::error);
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec, registration_id), &handler);
        });
        return completion.result.get();
    }
    template<class CompletionToken>
    auto async_unregister(id_type reg_id, CompletionToken&& token)
    {
        beast::async_completion<CompletionToken, void(boost::system::error_code)> completion(token);
        boost::asio::spawn(next_.get_io_service(),
                           [this, reg_id, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            try {
                id_type requestId = random::generate();
                auto msg = std::make_tuple(WampMsgCode::UNREGISTER, requestId , reg_id);
                auto arr = async_send_receive(requestId, msg, yield);
                WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
                if(code != WampMsgCode::UNREGISTERED) ec = make_error_code(wamp_errors::error);
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec), &handler);
        });
        return completion.result.get();
    }
    template<class CompletionToken>
    auto async_subscribe(const std::string uri, CompletionToken&& token)
    {
        beast::async_completion<CompletionToken, void(boost::system::error_code, id_type)> completion(token);
        boost::asio::spawn(next_.get_io_service(),
                           [this, uri, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            id_type subscription_id;
            try {
                id_type requestId = random::generate();
                auto msg = std::make_tuple(WampMsgCode::SUBSCRIBE, requestId, empty(), uri);
                auto arr = async_send_receive(requestId, msg, yield);
                WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
                if(code == WampMsgCode::SUBSCRIBED) subscription_id = adapters::as<id_type>(arr[2]);
                else ec = make_error_code(wamp_errors::error);
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec, subscription_id), &handler);
        });
        return completion.result.get();
    }
    template<class CompletionToken>
    auto async_unsubscribe(id_type reg_id, CompletionToken&& token)
    {
        beast::async_completion<CompletionToken, void(boost::system::error_code)> completion(token);
        boost::asio::spawn(next_.get_io_service(),
                           [this, reg_id, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            try {
                id_type requestId = random::generate();
                auto msg = std::make_tuple(WampMsgCode::UNSUBSCRIBE, requestId , reg_id);
                auto arr = async_send_receive(requestId, msg, yield);
                WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
                if(code != WampMsgCode::UNSUBSCRIBED) ec = make_error_code(wamp_errors::error);
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec), &handler);
        });
        return completion.result.get();
    }
    template<class CompletionToken, typename... Args>
    auto async_publish(const std::string& uri, bool ack, CompletionToken&& token, Args... args)
    {
        auto argsTuple = std::make_tuple(args...);
        beast::async_completion<CompletionToken, void(boost::system::error_code)> completion(token);
        id_type requestId = random::generate();
        std::map<std::string, bool> options;
        if(ack) options["acknowledge"] = true;
        auto msg = std::make_tuple(WampMsgCode::PUBLISH, requestId, options, uri, argsTuple);
        boost::asio::spawn(next_.get_io_service(),
                           [this, ack, requestId, msg, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            try {
                if(ack)
                {
                    auto arr = async_send_receive(requestId, msg, yield);
                    WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
                    if(code != WampMsgCode::PUBLISHED) ec = make_error_code(wamp_errors::error);
                }
                else
                {
                    Serializer s;
                    std::string str = s.serialize(msg);
                    next_.async_write(boost::asio::buffer(str), yield);
                }
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec), &handler);
        });
        return completion.result.get();
    }
    template<class Result, class CompletionToken>
    auto async_receive_event(id_type subscription_id, CompletionToken&& token)
    {
        beast::async_completion<CompletionToken, void(boost::system::error_code, Result)> completion(token);
        boost::asio::spawn(next_.get_io_service(),
                           [this, subscription_id, handler = completion.handler](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;
            Result res;
            try {
                auto msg = async_receive(subscription_id, yield);
                auto arr = msg[4];
                auto vec = adapters::as<std::vector<msgpack::object>>(arr);
                res = adapters::as<Result>(vec);
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            boost::asio::asio_handler_invoke(std::bind(handler, ec, res), &handler);
        });
        return completion.result.get();
    }
    template<class CompletionToken>
    auto async_receive(id_type registration_or_subscription_id, CompletionToken&& token)
    {
        response_completion completion(token);
        pending_requests_.insert({registration_or_subscription_id, completion.handler});
        listen();
        return completion.result.get();
    }
private:
    using variant_type = typename Serializer::variant_type;
    using array = std::vector<variant_type>;
    using response_completion = beast::async_completion<boost::asio::yield_context, void(boost::system::error_code, array)>;
    using void_completion = beast::async_completion<boost::asio::yield_context, void(boost::system::error_code)>;
    using handler_type = typename response_completion::handler_type;

    NextStream next_;
    std::unordered_multimap<id_type, const handler_type> pending_requests_;
    boost::asio::io_service::strand strand_;
    std::shared_ptr<void_completion> comp_;
    bool is_alive_ = true;
    template<class Message, class CompletionToken>
    auto async_send_receive(id_type requestId, Message&& msg, CompletionToken&& token)
    {
        Serializer s;
        std::string str = s.serialize(msg);
        response_completion completion(token);
        pending_requests_.insert({requestId, completion.handler});
        boost::asio::spawn(token,
                           [this, str](boost::asio::yield_context yield)
        {
            try {
                next_.async_write(boost::asio::buffer(str), yield);
                listen();
            }
            catch(boost::system::system_error& e)
            {
                auto message = e.what();
            }
        });
        return completion.result.get();
    }
    void listen()
    {
        if(!is_alive_) return;
        boost::asio::spawn(strand_,
                           [this](boost::asio::yield_context yield)
        {
            try {
                //while(is_alive_){
                Serializer s;
                beast::streambuf sb;
                beast::websocket::opcode op;
                int i=0;
                next_.async_read(op, sb, yield);
                auto m = s.deserialize(beast::to_string(sb.data()));
                using map = std::unordered_map<std::string, variant_type>;
                array arr = adapters::as<array>(m);
                WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
                id_type requestId = 0;
                boost::system::error_code ec;
                if(code == WampMsgCode::WAMP_ERROR)
                {
                    requestId = adapters::as<id_type>(arr[2]);
                    std::string error_uri = adapters::as<std::string>(arr[4]);
                    ec = make_error_code(error_uri);
                    auto iter = pending_requests_.find(requestId);
                    auto handler = iter->second;
                    pending_requests_.erase(iter);
                    boost::asio::asio_handler_invoke(std::bind(handler, ec, std::move(arr)), &handler);
                }
                else if(code == WampMsgCode::SUBSCRIBED || code == WampMsgCode::WAMP_REGISTERED
                        || code == WampMsgCode::UNREGISTERED || code == WampMsgCode::PUBLISHED
                        || code == WampMsgCode::UNSUBSCRIBED || code == WampMsgCode::EVENT)
                {
                    requestId = adapters::as<id_type>(arr[1]);
                    auto it = pending_requests_.find(requestId);
                    if(it != pending_requests_.end())
                    {
                        auto handler = it->second;
                        pending_requests_.erase(it);
                        boost::asio::asio_handler_invoke(std::bind(handler, ec, std::move(arr)), &handler);
                    }
                }
                //listen();
                //}
            }
            catch(boost::system::system_error& e)
            {
                auto message = e.what();
                std::cout << message << "\n\n";
            }
            int t=0;
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
                std::string const host = "127.0.0.1";//demo.crossbar.io
                boost::asio::ip::tcp::resolver r {socket_.get_io_service()};
                boost::asio::ip::tcp::socket sock {socket_.get_io_service()};
                boost::asio::ip::tcp::socket sock2 {socket_.get_io_service()};
                auto i = r.async_resolve(boost::asio::ip::tcp::resolver::query {host, "8080"}, yield);
                boost::asio::async_connect(sock, i, yield);
                boost::asio::async_connect(sock2, i, yield);
                //boost::asio::ssl::context ctx {boost::asio::ssl::context::sslv23};
                using stream_type = boost::asio::ip::tcp::socket;
                //tream_type ssl_stream {sock, ctx};
                //ssl_stream.async_handshake(boost::asio::ssl::stream_base::client, yield);
                beast::websocket::stream<stream_type&> ws {sock};
                //beast::websocket::stream<stream_type&> ws2 {sock2};

                ws.set_option(beast::websocket::decorate(protocol {KEY_WAMP_MSGPACK_SUB}));
                ws.set_option(beast::websocket::message_type {beast::websocket::opcode::binary});
                ws.async_handshake(host, "/ws",yield);
                wamp_stream<beast::websocket::stream<stream_type&>&, msgpack_serializer> wamp {ws};
                wamp.async_handshake("realm1", yield);

                /*ws2.set_option(beast::websocket::decorate(protocol {KEY_WAMP_MSGPACK_SUB}));
                ws2.set_option(beast::websocket::message_type {beast::websocket::opcode::binary});
                ws2.async_handshake(host, "/ws",yield);
                wamp_stream<beast::websocket::stream<stream_type&>&, msgpack_serializer> wamp2 {ws2};
                wamp2.async_handshake("realm1", yield);*/

                //auto sub_id = wamp.async_subscribe("test.topic", yield);
                //auto reg = wamp.async_register("test.add", []() {}, yield);
                //wamp.async_unregister(reg, yield);
                //wamp.async_publish("test.topic2", yield, "ahoj");
                //auto res = wamp.async_call("test.add2", yield, 1, 2);

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
