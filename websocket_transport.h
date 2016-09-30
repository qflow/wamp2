#ifndef WEBSOCKET_TRANSPORT
#define WEBSOCKET_TRANSPORT

#include "server_session.h"
#include "util/any.h"
#include <memory>
#include <functional>

namespace qflow{

class websocket_transport_private;
template<typename Serializers>
class websocket_transport
{
public:
    websocket_transport(unsigned short port);
    ~websocket_transport();

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
    bool validate(std::weak_ptr<void> hdl);
    const std::unique_ptr<websocket_transport_private> d_ptr;
    Serializers serializers;

};
}
#endif
