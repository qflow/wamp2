#ifndef WEBSOCKET_TRANSPORT
#define WEBSOCKET_TRANSPORT

#include "websocket_dependencies.h"
#include "server_session.h"
#include "msgpack_serializer.h"
#include "util/any.h"
#include "util/for_each_t.h"
#include <memory>
#include <functional>


namespace qflow{

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
    void post_message(any message)
    {
        std::string str = _s.serialize(message);
        _con->send(str, websocketpp::frame::opcode::binary);
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

    ~websocket_transport()
    {

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
    bool validate(std::weak_ptr<void> hdl)
    {
        server::connection_ptr con = s.get_con_from_hdl(hdl);
        const std::vector<std::string> & subp_requests = con->get_requested_subprotocols();
        bool found = false;
        auto a = for_each_t(serializers, [this, &found, subp_requests, con](auto, auto s){
            if(found) return 0;
            for(std::string subp: subp_requests)
            {
                using serializer_type = decltype(s);
                auto k = serializer_type::KEY;
                if(subp == k)
                {
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
    std::thread t;
    Serializers serializers;

};
template class websocket_transport<std::tuple<qflow::msgpack_serializer>>;
}
#endif
