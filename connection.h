#ifndef CONNECTION_H
#define CONNECTION_H

#include "util/adapters.h"
#include <memory>
#include <string>
#include <vector>
#include <executors/future.h>
#include "symbols.h"
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <functional>
#include <msgpack.hpp>

namespace adapters
{
    template<typename T>
    struct adapter<T, msgpack::object>
    {
        static T convert(const msgpack::object& o)
        {
            return o.as<T>();
        }
    };
    template<typename T>
    struct adapter<T, std::shared_ptr<msgpack::object_handle>>
    {
        static T convert(std::shared_ptr<msgpack::object_handle> p)
        {
            msgpack::object o = p->get();
            return o.as<T>();
        }
    };
    template<typename T>
    struct adapter<msgpack::object, T>
    {
        static msgpack::object convert(T t)
        {
            return msgpack::object(t);
        }
    };
}

namespace qflow{


using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::client<websocketpp::config::asio> client;
typedef websocketpp::config::asio::message_type::ptr message_ptr;

using message = std::string;

template<typename serializer>
class processor
{
public:
    serializer s;
    std::function<void(std::string)> _sendCallback;
    template<typename message>
    void message_received(message m)
    {
        auto arr = adapters::as<std::vector<message>>(std::forward<message>(m));
    }
    template<typename message>
    void send(message m)
    {
        _sendCallback(s.serialize(m));
    }
    template<typename Function>
    void register_send_callback(Function&& f)
    {
        _sendCallback = f;
    }

};

class msgpack_serializer
{
public:
    static constexpr const char* KEY = KEY_WAMP_MSGPACK_SUB;
    using variant_type = msgpack::object;
    msgpack_serializer()
    {

    }
    template<typename message_type>
    std::string serialize(message_type message)
    {
        std::stringstream buffer;
        msgpack::pack(buffer, message);
        return buffer.str();
    }
    auto deserialize(std::string m)
    {
        auto p = std::make_shared<msgpack::object_handle>();
        msgpack::unpack(*p.get(), m.data(), m.size());
        return p;
    }
};


enum SESSION_STATE : int {
    OPENED = 1,
    CLOSED = -1
};
template<typename transport_connection_type, typename serializer>
class session
{
public:
    SESSION_STATE state() const;
private:
    SESSION_STATE _state;
};

template<typename serializer>
class session<client::connection_ptr, serializer>
{
public:
    session<client::connection_ptr, serializer>(client::connection_ptr con, SESSION_STATE state = SESSION_STATE::OPENED) : _con(con), _state(state)
    {

    }
    SESSION_STATE state() const
    {
        return _state;
    }
    using type = session<client::connection_ptr, serializer>;
    using ptr = std::shared_ptr<type>;
private:
    SESSION_STATE _state;
    client::connection_ptr _con;
    processor<serializer> proc;
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg)
    {
        int t=0;
    }
    void connect_handlers()
    {
        _con->set_message_handler(bind(&type::on_message,this,_1,_2));
    }
};



template<typename serializer>
qflow::future<typename session<client::connection_ptr, serializer>::ptr> get_session(client& c, std::string uri)
{
    using session_type = session<client::connection_ptr, serializer>;
    auto promise = std::make_shared<qflow::promise<typename session_type::ptr>>();
    auto future = promise->get_future();
    websocketpp::lib::error_code ec;
    auto con = c.get_connection(uri, ec);
    con->add_subprotocol(serializer::KEY);
    con->set_open_handler([con, promise](websocketpp::connection_hdl /*hdl*/){
        auto session = std::make_shared<session_type>(con);
        std::string protocol = con->get_subprotocol();
        promise->set_value(session);
    });
    con->set_fail_handler([con, promise](websocketpp::connection_hdl /*hdl*/){
        auto session = std::make_shared<session_type>(con, SESSION_STATE::CLOSED);
        promise->set_value(session);
    });
    con->set_close_handler([con, promise](websocketpp::connection_hdl /*hdl*/){
        auto session = std::make_shared<session_type>(con, SESSION_STATE::CLOSED);
        promise->set_value(session);

    });
    c.connect(con);
    return future;
}

}
#endif
