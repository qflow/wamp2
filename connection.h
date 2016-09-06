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
    const std::string KEY = KEY_WAMP_MSGPACK_SUB;
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



template<typename transport_connection_type, typename serializer>
class session
{

};
template<typename serializer>
class session<client::connection_ptr, serializer>
{
public:
    using type = session<client::connection_ptr, serializer>;
    client::connection_ptr con;
    processor<serializer> proc;
    void on_open(websocketpp::connection_hdl /*hdl*/)
    {
    }
    bool on_validate(websocketpp::connection_hdl hdl)
    {
        return true;
    }

    void on_message(websocketpp::connection_hdl hdl, message_ptr msg)
    {
    }
    void on_close(websocketpp::connection_hdl /*hdl*/)
    {
    }
    void on_fail(websocketpp::connection_hdl /*hdl*/)
    {
    }
    void connect_handlers()
    {
        con->set_message_handler(bind(&type::on_message,this,::_1,::_2));
        con->set_close_handler(bind(&type::on_close,this,::_1));
        con->set_open_handler(bind(&type::on_open,this,::_1));
        con->set_fail_handler(bind(&type::on_fail,this,::_1));
        con->set_validate_handler(bind(&type::on_validate,this,::_1));
    }
};

template<typename serializer>
qflow::future<session<client::connection_ptr, serializer>> connect(client& c, std::string uri)
{
    using sesion_type = session<client::connection_ptr, serializer>;
    qflow::promise<session_type> promise;
    auto future = promise.get_future();
    session_type session;
    promise.set_value(session);
    websocketpp::lib::error_code ec;
    session.con = c.get_connection(uri, ec);
    session.con->add_subprotocol(serializer::KEY);
    session.connect_handlers();
    c.connect(session.con);
    return future;
}

}
#endif
