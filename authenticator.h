#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "symbols.h"
#include "random.h"
#include <memory>
#include <chrono>
#include <tuple>
#include <unordered_map>

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
    virtual std::unique_ptr<authenticator> clone() const = 0;
};

class credential_store
{
public:
    virtual std::string get_credential(std::string id) const = 0;
};

template<typename T>
class credential_store_adapter : public credential_store
{
public:
    credential_store_adapter(T&&)
    {

    }
    std::string get_credential(std::string) const
    {
        return std::string();
    }
};
template<>
class credential_store_adapter<std::unordered_map<std::string, std::string>>
{
public:
    credential_store_adapter(std::unordered_map<std::string, std::string>&& cred_store) : _store(cred_store)
    {

    }
    std::string get_credential(std::string id) const
    {
        return _store.at(id);
    }

private:
    std::unordered_map<std::string, std::string> _store;
};


class wampcra_authenticator : public authenticator
{
public:
    static constexpr const char* KEY = KEY_WAMPCRA;
    wampcra_authenticator()
    {
    }
    template<typename CredentialStore>
    wampcra_authenticator(CredentialStore&& cred_store)
    {
        _store = std::make_shared<credential_store_adapter<CredentialStore>>(cred_store);
    }
    wampcra_authenticator(std::shared_ptr<credential_store> cred_store)
    {
        _store = cred_store;
    }
    using user = std::tuple<const char*, const char*>;
    wampcra_authenticator(user& u) : _user(u)
    {
    }
    std::string authid() const
    {
        return std::get<0>(_user);
    }
    std::string challenge(token t);
    AUTH_RESULT authenticate(const std::string& response, std::string&);

    std::string response(std::string challenge);
    std::unique_ptr<authenticator> clone() const
    {
        return std::make_unique<wampcra_authenticator>(*this);
    }

private:
    std::shared_ptr<credential_store> _store;
    user _user;
    std::string _authid;
    std::string _challenge;
};
}
#endif
