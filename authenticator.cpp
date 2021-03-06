#include "random.h"
#include "authenticator.h"
#include "json_serializer.h"
#include <cryptopp/sha.h>
#include <cryptopp/hmac.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace std::chrono;

namespace qflow{

std::string wampcra_authenticator::challenge(token t)
{
    json challenge;
    challenge["authid"] = t.authid;
    _authid = t.authid;
    challenge["authprovider"] = "wampcra";
    challenge["authmethod"] = "wampcra";
    std::string nonceStr = std::to_string(random::generate());
    challenge["nonce"] = nonceStr;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    auto in_time_t = system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:");
    ss << std::setfill('0') << std::setw(3) << ms.count() << "Z";

    challenge["timestamp"] = ss.str();
    challenge["session"] = t.sessionId;
    std::string res = challenge.dump();
    _challenge = res;
    return res;
}
AUTH_RESULT wampcra_authenticator::authenticate(const std::string& response, std::string&)
{
    auto secret = _store->get_credential(_authid);
    const byte* secretData = reinterpret_cast<const byte*>(secret.c_str());
    CryptoPP::HMAC<CryptoPP::SHA256> hmac(secretData, secret.size());
    std::string digest;
    CryptoPP::StringSource foo(_challenge, true,
                               new CryptoPP::HashFilter(hmac,
                                                        new CryptoPP::Base64Encoder (
                                                            new CryptoPP::StringSink(digest), false)));

    if(response == digest) return AUTH_RESULT::ACCEPTED;
    return AUTH_RESULT::REJECTED;
}
std::string wampcra_authenticator::response(std::string challenge)
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

}
