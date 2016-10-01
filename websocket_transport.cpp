#include "util/any.h"
#include "websocket_transport.h"
#include "msgpack_serializer.h"
#include <thread>


namespace qflow{

using server = websocketpp::server<websocketpp::config::asio>;
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::ref;
typedef websocketpp::config::asio::message_type::ptr message_ptr;

class websocket_transport_private
{
public:
    server s;
    std::thread t;
    websocket_transport_private()
    {

    }
    ~websocket_transport_private()
    {

    }
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
websocket_transport<Serializers>::websocket_transport(unsigned short port) : d_ptr(new websocket_transport_private())
{
    d_ptr->s.set_validate_handler(bind(&websocket_transport::validate,this,_1));
    d_ptr->s.init_asio();
    d_ptr->s.set_reuse_addr(true);
    d_ptr->s.listen(port);
    d_ptr->s.start_accept();
    d_ptr->t = std::thread(&server::run, &d_ptr->s);
}
template<typename Serializers>
bool websocket_transport<Serializers>::validate(connection_hdl hdl)
{
    server::connection_ptr con = d_ptr->s.get_con_from_hdl(hdl);
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
template<typename Serializers>
websocket_transport<Serializers>::~websocket_transport()
{

}

using serializers = std::tuple<qflow::msgpack_serializer>;
template class websocket_transport<serializers>;

}
