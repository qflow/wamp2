#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "json_serializer.h"
#include "random.h"
#include "symbols.h"
#include <chrono>
#include <tuple>
#include <cryptopp/sha.h>
#include <cryptopp/hmac.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>

namespace qflow{

template<typename T, typename Enable = void>
class authenticator
{
public:
    static constexpr const char* KEY = "unknown";
    std::string authid() const
    {
        return "unknown";
    }
    std::string response(std::string /*challenge*/)
    {
        return "unknown";
    }
};
template<>
class authenticator<std::tuple<const char*, const char*>>
{
public:
    static constexpr const char* KEY = KEY_WAMPCRA;
    authenticator()
    {
    }
    using user = std::tuple<const char*, const char*>;
    authenticator(user& u) : _user(u)
    {
    }
    std::string authid() const
    {
        return std::get<0>(_user);
    }
    template<typename T>
    std::string challenge(T token)
    {
        json challenge;
        challenge["authid"] = token["authid"];
        challenge["authprovider"] = "wampcra";
        challenge["authmethod"] = "wampcra";
        std::string nonceStr = std::to_string(random::generate());
        challenge["nonce"] = nonceStr;
        QString timestampStr = QDateTime::currentDateTimeUtc().toString("yyyy-mm-ddThh:mm:zzzZ");
        auto now = std::chrono::system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:");
        ss << std::setfill('0') << std::setw(3) << ms.count();

        challenge["timestamp"] = ss.str();
        challenge["session"] = token["sessionId"];
        return challenge.dump();
    }

    std::string response(std::string challenge)
    {
        const byte* secretData = reinterpret_cast<const byte*>(std::get<1>(_user));
        size_t secretLen = strlen(std::get<1>(_user));
        CryptoPP::HMAC<CryptoPP::SHA256> hmac(secretData, secretLen);

        std::string digest;

        CryptoPP::StringSource foo(challenge, true,
                                   new CryptoPP::HashFilter(hmac,
                                                            new CryptoPP::Base64Encoder (
                                                                new CryptoPP::StringSink(digest), false)));
        return digest;

    }
private:
    user _user;
};
}
#endif
