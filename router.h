#ifndef ROUTER_H
#define ROUTER_H

#include "authenticator.h"
#include "util/any.h"
#include "util/for_each_t.h"
#include "util/adapters.h"
#include "symbols.h"
#include "random.h"
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
    router(Authenticators& auths) : _authenticators(auths)
    {
    }
    template<typename T>
    void add_transport(T transport_ptr)
    {
        transport_ptr->set_on_new_session(std::bind(&router::on_new_session, this, _1));
        transport_ptr->set_on_message(std::bind(&router::on_message, this, _1, _2));
    }

private:
    using map = std::unordered_map<std::string, any>;
    std::unordered_map<server_session_ptr, std::shared_ptr<authenticator>> _authenticating_sessions;
    void on_new_session(server_session_ptr session)
    {
        _sessions.insert(session);
    }
    void welcome(server_session_ptr session)
    {
        map roles = {{"broker", map()}, {"dealer", map()}};
        map details = {{"roles", roles}};
        auto msg = std::make_tuple(WampMsgCode::WELCOME, session->sessionId(), details);
        session->post_message(msg);
    }
    struct registration
    {
        id_type id;
        server_session_ptr session;
    };

    void on_message(server_session_ptr session, any message)
    {
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
                auto res = for_each_t(_authenticators, [method, session, details, this](auto, auto auth_prototype){
                    auto k = auth_prototype.KEY;
                    if(k == method)
                    {
                        std::string authId = adapters::as<std::string>(details.at("authid"));
                        token t = {session->sessionId(), authId};
                        auto auth = std::make_shared<decltype(auth_prototype)>(auth_prototype);
                        _authenticating_sessions[session] = auth;
                        std::string challenge = auth->challenge(t);
                        auto msg = std::make_tuple(WampMsgCode::CHALLENGE, k, map{{"challenge", challenge}});
                        session->post_message(msg);
                    }
                    return 0;
                });
            }
        }
        else if(code == WampMsgCode::AUTHENTICATE)
            {
                std::string response = adapters::as<std::string>(arr[1]);
                auto auth = _authenticating_sessions[session];
                std::string new_challenge;
                AUTH_RESULT res = auth->authenticate(response, new_challenge);
                if(res == AUTH_RESULT::ACCEPTED)
                {
                    welcome(session);
                    _authenticating_sessions.erase(session);
                }
                /*if(authResult == AUTH_RESULT::REJECTED)
                {
                    abort(KEY_ERR_NOT_AUTHENTICATED);
                }
                if(authResult == AUTH_RESULT::CONTINUE)
                {
                    QVariantMap obj;
                    obj["challenge"] = _authSession->outBuffer;
                    QVariantList authArr{WampMsgCode::CHALLENGE, _authSession->authenticator->authMethod(), obj};
                    sendWampMessage(authArr);
                }*/
        }
        else if(code == WampMsgCode::REGISTER)
        {
            std::string uri = adapters::as<std::string>(arr[3]);
            id_type requestId = adapters::as<id_type>(arr[1]);
            id_type registrationId = random::generate();
            _uri_registration[uri] = {registrationId, session};
            auto msg = std::make_tuple(WampMsgCode::WAMP_REGISTERED, requestId, registrationId);
            session->post_message(msg);
        }
        else if(code == WampMsgCode::CALL)
        {
            id_type requestId = adapters::as<id_type>(arr[1]);
            std::string uri = adapters::as<std::string>(arr[3]);
            array params;
            if(arr.size()>4) params = adapters::as<array>(arr[4]);
            auto reg = _uri_registration.find(uri);
            if(reg != _uri_registration.end())
            {
                auto msg = std::make_tuple(WampMsgCode::INVOCATION, requestId, reg->second.id, map(), params);
                _pending_invocations[requestId] = session;
                reg->second.session->post_message(msg);
            }
            else
            {
                //error(WampMsgCode::CALL, KEY_ERR_NO_SUCH_PROCEDURE, requestId, {{"procedureUri", uri}});
            }
        }
        else if(code == WampMsgCode::YIELD)
        {
            id_type requestId = adapters::as<id_type>(arr[1]);
            auto caller = _pending_invocations[requestId];
            _pending_invocations.erase(requestId);
            array resultArr;
            if(arr.size()>3) resultArr = adapters::as<array>(arr[3]);
            auto msg = std::make_tuple(WampMsgCode::RESULT, requestId, map(), resultArr);

        }
    }
    std::unordered_map<std::string, registration> _uri_registration;
    std::unordered_map<id_type, server_session_ptr> _pending_invocations;
    Authenticators _authenticators;
    std::unordered_set<server_session_ptr> _sessions;
};
}
#endif
