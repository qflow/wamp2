#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <string>
#include <functional>
#include <unordered_map>
#include <forward_list>
#include "util/functor.h"
#include "symbols.h"

namespace qflow{
template<typename serializer, typename authenticator_type>
class processor
{
public:
    processor()
    {
    }
    void set_authenticator(authenticator_type&& auth)
    {
        _auth = std::move(auth);
    }
    void set_realm(const std::string& value)
    {
        _realm = value;
    }
    using functor_ptr = std::shared_ptr<functor<typename serializer::variant_type>>;
    template<typename Function>
    void set_send_callback(Function&& f)
    {
        _sendCallback = f;
    }
    void post_message(const std::string str)
    {
        auto message = s.deserialize(str);
        message_received(message);
    }
    template<typename T>
    void add_registration(const std::string& uri, T&& reg)
    {
        auto f = new functor_impl<typename serializer::variant_type, T>(std::forward<T>(reg));
        _registrations[uri] = functor_ptr(f);
    }
    void hello()
    {
        std::forward_list<std::string> authmethods{authenticator_type::KEY};
        auto options = std::make_tuple(std::make_pair("authmethods", authmethods), std::make_pair("authid", _auth.authid()));
        auto msg = std::make_tuple(WampMsgCode::HELLO, _realm, options);
        send(msg);
    }
private:
    std::unordered_map<std::string, functor_ptr> _registrations;
    serializer s;
    authenticator_type _auth;
    std::function<void(std::string)> _sendCallback;
    std::string _realm;
    template<typename message>
    void message_received(message m)
    {
        using map = std::unordered_map<std::string, typename serializer::variant_type>;
        using array = std::vector<typename serializer::variant_type>;
        array arr = adapters::as<array>(m);
        WampMsgCode code = adapters::as<WampMsgCode>(arr[0]);
        if(code == WampMsgCode::CHALLENGE)
        {
            map extra = adapters::as<map>(arr[2]);
            std::string challenge = adapters::as<std::string>(extra["challenge"]);
            std::string response = _auth.response(challenge);
            auto msg = std::make_tuple(WampMsgCode::AUTHENTICATE, response);
            send(msg);
        }
        else if(code == WampMsgCode::WELCOME)
        {
            int r = 0;
        }
        int t=0;
    }
    template<typename message>
    void send(message m)
    {
        _sendCallback(s.serialize(m));
    }

};
}

#endif
