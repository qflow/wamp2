#ifndef ROUTER_H
#define ROUTER_H

#include "util/for_each_t.h"
#include <thread>
#include <tuple>
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::ref;

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

private:
    Serializer _s;
    server::connection_ptr _con;
};

template<typename Serializers>
class websocket_transport
{
public:
    websocket_transport(unsigned short port)
    {
        s.set_validate_handler(bind(&websocket_transport::validate,this,_1));
        s.init_asio();
        s.listen(port);
        s.start_accept();
        t = std::thread(&server::run, &s);
    }
    template<typename T>
    void set_on_new_session(T&& callback)
    {
        _on_new_session = callback;
    }

private:
    std::function<void(server_session_ptr)> _on_new_session;
    bool validate(connection_hdl hdl)
    {
        server::connection_ptr con = s.get_con_from_hdl(hdl);
        const std::vector<std::string> & subp_requests = con->get_requested_subprotocols();
        bool found = false;
        auto a = for_each_t(serializers, [this, &found, subp_requests, con](auto idx, auto s){
            if(found) return 0;
            for(std::string subp: subp_requests)
            {
                if(subp == s.KEY)
                {
                    auto session = std::make_shared<server_session<server::connection_ptr, decltype(s)>>(con);
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

class router
{
public:
    router()
    {
    }
    template<typename T>
    void add_transport(T transport_ptr)
    {
        transport_ptr->set_on_new_session([](auto session){
            server_session_ptr p = session;
        });
    }

private:
};

class realm
{

};
}
#endif
