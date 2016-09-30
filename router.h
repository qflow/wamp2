#ifndef ROUTER_H
#define ROUTER_H

#include "server_session.h"
#include "authenticator.h"
#include "util/any.h"
#include "symbols.h"
#include "random.h"
#include <unordered_map>
#include <unordered_set>

#include <functional>

using namespace std::placeholders;

namespace qflow{


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
    template<typename T>
    void add_authenticator(std::shared_ptr<T> authenticator_ptr)
    {
        auto k = T::KEY;
        _authenticators[k] = authenticator_ptr;
    }

private:
    using map = std::unordered_map<std::string, any>;
    std::unordered_map<server_session_ptr, std::shared_ptr<authenticator>> _authenticating_sessions;
    void on_new_session(server_session_ptr session)
    {
        _sessions.insert(session);
    }
    void welcome(server_session_ptr session);
    struct registration
    {
        id_type id;
        server_session_ptr session;
    };

    void on_message(server_session_ptr session, any message);
    std::unordered_map<std::string, registration> _uri_registration;
    std::unordered_map<id_type, server_session_ptr> _pending_invocations;
    std::unordered_map<std::string, std::shared_ptr<authenticator>> _authenticators;
    std::unordered_set<server_session_ptr> _sessions;
};
}
#endif
