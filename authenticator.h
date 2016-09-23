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

using namespace std::chrono;

namespace qflow{

enum class AUTH_RESULT {ACCEPTED = 1, REJECTED = 0, CONTINUE = 2};
struct token
{
    id_type sessionId;
    std::string authid;
};

class authenticator
{
public:
    virtual std::string challenge(token t) = 0;
    virtual AUTH_RESULT authenticate(const std::string& response, std::string& newChallenge) = 0;
};

template<typename CredentialStore>
class wampcra_authenticator : public authenticator
{
public:
    static constexpr const char* KEY = KEY_WAMPCRA;
    wampcra_authenticator()
    {
    }
    wampcra_authenticator(CredentialStore& cred_store) : _store(cred_store)
    {

    }
    using user = std::tuple<const char*, const char*>;
    wampcra_authenticator(user& u) : _user(u)
    {
    }
    std::string authid() const
    {
        return std::get<0>(_user);
    }
    std::string challenge(token t)
    {
        json challenge;
        challenge["authid"] = t.authid;
        challenge["authprovider"] = "wampcra";
        challenge["authmethod"] = "wampcra";
        std::string nonceStr = std::to_string(random::generate());
        challenge["nonce"] = nonceStr;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:");
        ss << std::setfill('0') << std::setw(3) << ms.count() << "Z";

        challenge["timestamp"] = ss.str();
        challenge["session"] = t.sessionId;
        std::string res = challenge.dump();
        return res;
    }
    AUTH_RESULT authenticate(const std::string& response, std::string& newChallenge)
    {
        /*QMessageAuthenticationCode hash(QCryptographicHash::Sha256);
        WampCraUser* crauser = (WampCraUser*)user.data();
        hash.setKey(crauser->secret().toLatin1());
        QString challengeStr = challenge["challenge"].toString();
        hash.addData(challengeStr.toLatin1());

        QByteArray hashResult = hash.result();
        QByteArray finalSig = hashResult.toBase64();
        if(finalSig == inBuffer) return AUTH_RESULT::ACCEPTED;*/
        return AUTH_RESULT::REJECTED;
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
    CredentialStore _store;
    user _user;
};
}
#endif
