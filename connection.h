#ifndef CONNECTION_H
#define CONNECTION_H

#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <functional>
#include "msgpack_serializer.h"
#include "processor.h"
#include "session.h"

namespace qflow{


using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::client<websocketpp::config::asio> client;
typedef websocketpp::config::asio::message_type::ptr message_ptr;



template<typename serializer, typename user>
class session<client::connection_ptr, serializer, user>
{
public:
    using type = session<client::connection_ptr, serializer, user>;
    using ptr = std::shared_ptr<type>;
    session<client::connection_ptr, serializer, user>(client& c, const std::string uri, user& u) : _c(c),
        _uri(uri), _user(u)
    {
        _proc.set_send_callback(bind(&type::on_send,this,_1));
    }
    ~session<client::connection_ptr, serializer, user>()
    {

    }
    void connect()
    {
        websocketpp::lib::error_code ec;
        _con = _c.get_connection(_uri, ec);
        connect_handlers();
        _c.connect(_con);
    }

    SESSION_STATE state() const
    {
        return _state;
    }
    template<typename T>
    void add_registration(const std::string& uri, T&& reg)
    {
        _proc.add_registration<T>(uri, std::forward<T>(reg));
    }

private:
    client& _c;
    user& _user;
    std::string _uri;
    SESSION_STATE _state;
    client::connection_ptr _con;
    processor<serializer> _proc;
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg)
    {
        std::string s = msg->get_payload();
        _proc.post_message(s);
    }
    void on_open(websocketpp::connection_hdl /*hdl*/)
    {
    }
    void on_fail(websocketpp::connection_hdl /*hdl*/)
    {
        connect();
    }
    void on_close(websocketpp::connection_hdl /*hdl*/){

    }
    void on_pong_timeout(websocketpp::connection_hdl,std::string)
    {
        int i=0;
    }
    void on_send(const std::string msg)
    {
        _con->send(msg);
    }

    void connect_handlers()
    {
        _con->set_message_handler(bind(&type::on_message,this,_1,_2));
        _con->set_close_handler(bind(&type::on_close,this,_1));
        _con->set_open_handler(bind(&type::on_open,this,_1));
        _con->set_fail_handler(bind(&type::on_fail,this,_1));
        _con->set_pong_timeout_handler(bind(&type::on_pong_timeout,this,_1,_2));
    }
};



template<typename serializer, typename user>
typename session<client::connection_ptr, serializer, user>::ptr get_session(client& c, const std::string uri, user& u)
{
    using session_type = session<client::connection_ptr, serializer, user>;
    auto session_ptr = std::make_shared<session_type>(c, uri, u);
    return session_ptr;
}

}
#endif
