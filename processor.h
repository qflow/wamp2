#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "random.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <forward_list>
#include "util/functor.h"
#include "symbols.h"
#include "future.h"

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
    template<typename T>
    void set_on_connected(T&& handler)
    {
        _on_connected_handler = handler;
    }
    void post_message(const std::string str)
    {
        auto message = s.deserialize(str);
        message_received(message);
    }
    struct pending_reg
    {
        qflow::promise<void> p;
        functor_ptr reg;
        std::string uri;
    };

    template<typename T>
    qflow::future<void> add_registration(const std::string& uri, T&& reg)
    {
        pending_reg p_reg;
        p_reg.uri = uri;
        qflow::future<void> fut = p_reg.p.get_future();
        functor_ptr f = std::make_shared<functor_impl<typename serializer::variant_type, T>>(std::forward<T>(reg));
        p_reg.reg = f;
        _registrations[uri] = f;
        id_type requestId = random::generate();
        _pending_registrations[requestId] = std::move(p_reg);
        auto msg = std::make_tuple(WampMsgCode::REGISTER, requestId, empty_map(), uri);
        send(msg);
        return fut;
    }
    void hello()
    {
        std::forward_list<std::string> authmethods{authenticator_type::KEY};
        auto options = std::make_tuple(std::make_pair("authmethods", authmethods), std::make_pair("authid", _auth.authid()));
        auto msg = std::make_tuple(WampMsgCode::HELLO, _realm, options);
        send(msg);
    }
    struct pending_call
    {
        functor<typename serializer::variant_type> f;
    };

    template<typename ResultType, typename... Args>
    qflow::future<ResultType> call(const std::string& uri, Args... args)
    {
        auto p = std::make_shared<qflow::promise<ResultType>>();
        qflow::future<ResultType> fut = p->get_future();
        auto f = make_functor_shared<typename serializer::variant_type>([p](ResultType val){
            p->set_value(val);
        });
        id_type requestId = random::generate();
        _pending_calls[requestId] = f;
        auto argsTuple = std::make_tuple(args...);
        auto msg = std::make_tuple(WampMsgCode::CALL, requestId, empty_map(), uri, argsTuple);
        send(msg);
        return fut;
    }
private:
    std::function<void()> _on_connected_handler;
    using empty_map = std::unordered_map<std::string, std::string>;
    std::unordered_map<std::string, functor_ptr> _registrations;
    std::unordered_map<id_type, pending_reg> _pending_registrations;
    std::unordered_map<id_type, functor_ptr> _complete_registrations;
    std::unordered_map<id_type, functor_ptr> _pending_calls;
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
            if(_on_connected_handler) _on_connected_handler();
        }
        else if(code == WampMsgCode::WAMP_REGISTERED)
        {
            id_type regId = adapters::as<id_type>(arr[2]);
            id_type requestId = adapters::as<id_type>(arr[1]);
            pending_reg p_reg = std::move(_pending_registrations[requestId]);
            p_reg.p.set_value();
            _pending_registrations.erase(requestId);
            _complete_registrations[regId] = p_reg.reg;
        }
        else if(code == WampMsgCode::INVOCATION)
        {
            id_type requestId = adapters::as<id_type>(arr[1]);
            id_type regId = adapters::as<id_type>(arr[2]);
            auto reg = _complete_registrations[regId];
            array args;
            if(arr.size() > 4)
            {
                args = adapters::as<array>(arr[4]);
            }
            typename serializer::variant_type res = reg->invoke(args);
            auto resultArr = std::make_tuple(res);
            auto msg = std::make_tuple(WampMsgCode::YIELD, requestId, empty_map(), resultArr);
            send(msg);
        }
        else if(code == WampMsgCode::RESULT)
        {
            id_type requestId = adapters::as<id_type>(arr[1]);
            functor_ptr f = _pending_calls[requestId];
            _pending_calls.erase(requestId);
            if(arr.size() > 3)
            {
                array resultArray = adapters::as<array>(arr[3]);
                f->invoke(resultArray);
            }

        }
    }
    template<typename message>
    void send(message m)
    {
        _sendCallback(s.serialize(m));
    }

};
}

#endif
