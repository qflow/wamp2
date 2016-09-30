#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include "util/any.h"
#include "util/adapters.h"
#include "random.h"
#include <memory>

namespace qflow{
class server_session_base
{
public:
    virtual void post_message(any message) = 0;
    template<typename T>
    void post_message(T msg)
    {
        any a = adapters::as<any>(msg);
        post_message(a);
    }
    id_type sessionId() const
    {
        return _sessionId;
    }
protected:
    id_type _sessionId = random::generate();
};
using server_session_ptr = std::shared_ptr<server_session_base>;

template<typename Connection, typename Serializer>
class server_session : public server_session_base
{

};
}
#endif
