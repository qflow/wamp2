#ifndef ROUTER_H
#define ROUTER_H

#include "util/any.h"
#include "util/for_each_t.h"
#include "util/for_index.h"
#include "util/adapters.h"
#include "symbols.h"
#include <thread>
#include <unordered_map>
#include <tuple>
#include <unordered_set>
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>

using linb::any;
using linb::any_cast;

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::ref;
typedef websocketpp::config::asio::message_type::ptr message_ptr;

namespace qflow{


class server_session_base
{
public:

};
using server_session_ptr = std::shared_ptr<server_session_base>;

template<typename Connection, typename Serializer>
class server_session : public server_session_base
{

};
template<typename Serializer>
class server_session<server::connection_ptr, Serializer> : public server_session_base
{
public:
    server_session(server::connection_ptr con) : _con(con)
    {

    }
    template<typename T>
    void set_on_message(T&& f)
    {
        _on_message = f;
    }

private:
    Serializer _s;
    server::connection_ptr _con;
    std::function<void(typename Serializer::variant_type)> _on_message;
};

template<typename Serializers>
class websocket_transport
{
public:
    websocket_transport(unsigned short port)
    {
        s.set_validate_handler(bind(&websocket_transport::validate,this,_1));
        s.init_asio();
        s.set_reuse_addr(true);
        s.listen(port);
        s.start_accept();
        t = std::thread(&server::run, &s);
    }

    template<typename T>
    void set_on_new_session(T&& callback)
    {
        _on_new_session = callback;
    }
    template<typename T>
    void set_on_message(T&& callback)
    {
        _on_message = callback;
    }

private:
    std::function<void(server_session_ptr)> _on_new_session;
    std::function<void(server_session_ptr, any)> _on_message;
    bool validate(connection_hdl hdl)
    {
        server::connection_ptr con = s.get_con_from_hdl(hdl);
        const std::vector<std::string> & subp_requests = con->get_requested_subprotocols();
        bool found = false;
        auto a = for_each_t(serializers, [this, &found, subp_requests, con](auto, auto s){
            if(found) return 0;
            for(std::string subp: subp_requests)
            {
                if(subp == s.KEY)
                {
                    using serializer_type = decltype(s);
                    auto session = std::make_shared<server_session<server::connection_ptr, serializer_type>>(con);
                    con->set_message_handler([this, session](websocketpp::connection_hdl /*hdl*/, message_ptr msg){
                        std::string msg_str = msg->get_payload();
                        serializer_type s;
                        auto msg_native = s.deserialize(msg_str);
                        if(_on_message) _on_message(session, adapters::as<any>(msg_native));
                    });
                    if(_on_new_session) _on_new_session(session);
                    found = true;
                }
            }
            return 0;
        });
        return found;
    }
    server s;
    Serializers serializers;
    std::thread t;

};

template<typename Authenticators>
class router
{
public:
    router()
    {
    }
    template<typename T>
    void add_transport(T transport_ptr)
    {
        transport_ptr->set_on_new_session(std::bind(&router::on_new_session, this, _1));
        transport_ptr->set_on_message(std::bind(&router::on_message, this, _1, _2));
    }

private:
    void on_new_session(server_session_ptr session)
    {
        _sessions.insert(session);
    }
    void on_message(server_session_ptr session, any message)
    {
        using map = std::unordered_map<std::string, any>;
        using array = std::vector<any>;
        array arr = adapters::as<array>(message);
        WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
        if(code == WampMsgCode::HELLO)
        {

            map details = adapters::as<map>(arr[2]);
            array auth_methods = adapters::as<array>(details["authmethods"]);
            for(auto v: auth_methods)
            {
                std::string method = adapters::as<std::string>(v);
                auto res = for_index<std::tuple_size<Authenticators>::value>([method, session, details](auto idx){
                    using auth_type = typename std::tuple_element<idx, Authenticators>::type;
                    if(auth_type::KEY == method)
                    {
                        std::string authId = adapters::as<std::string>(details.at("authid"));
                        int r=0;
                    }
                    return 0;
                });
            }
        }
    }

    std::unordered_set<server_session_ptr> _sessions;
};

class realm
{

};
}
#endif
