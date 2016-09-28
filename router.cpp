#include "router.h"
#include "msgpack_serializer.h"

namespace qflow{
void router::on_message(server_session_ptr session, any message)
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
            auto i = _authenticators.find(method);
            if(i != _authenticators.end())
            {
                auto auth_prototype = i->second;
                std::string authId = adapters::as<std::string>(details.at("authid"));
                token t = {session->sessionId(), authId};
                auto auth = std::shared_ptr<authenticator>(auth_prototype->clone());
                _authenticating_sessions[session] = auth;
                std::string challenge = auth->challenge(t);
                auto msg = std::make_tuple(WampMsgCode::CHALLENGE, method, map{{"challenge", challenge}});
                session->post_message(msg);
                return;
            }
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
            else if(res == AUTH_RESULT::REJECTED)
            {
                auto msg = std::make_tuple(WampMsgCode::ABORT, map(), KEY_ERR_NOT_AUTHENTICATED);
                session->post_message(msg);
            }
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
            auto msg = std::make_tuple(WampMsgCode::WAMP_ERROR, WampMsgCode::CALL, requestId, map{{"procedureUri", uri}}, KEY_ERR_NO_SUCH_PROCEDURE);
            session->post_message(msg);
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
        caller->post_message(msg);
    }
}
}
