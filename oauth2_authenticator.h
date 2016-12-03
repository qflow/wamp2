#ifndef OAUTH2_AUTHENTICATOR
#define OAUTH2_AUTHENTICATOR

#include "http_request.h"
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <beast/core/streambuf.hpp>
#include <nlohmann/json.hpp>

using namespace nlohmann;

namespace qflow {
enum class oauth2_errors
{
    authorization_code_expired = 100,
    invalid_verification_code_format = 101,
    continue_required = 1201,
    not_authenticated = 1101,
    authenticated = 0
};

class oauth2_category_impl :
    public boost::system::error_category
{
public:
    const char *name() const noexcept {
        return "oauth2";
    }
    std::string message(int ev) const
    {
        switch(static_cast<oauth2_errors>(ev))
        {
        case oauth2_errors::continue_required :
            return "continue required";
        case oauth2_errors::authenticated :
            return "authenticated";
        case oauth2_errors::not_authenticated :
            return "not authenticated";
        case oauth2_errors::authorization_code_expired :
            return "This authorization code has expired.";
        case oauth2_errors::invalid_verification_code_format:
            return "Invalid verification code format";
        }
        return "undefined";
    }
};
static const boost::system::error_category& oauth2_category = qflow::oauth2_category_impl();
boost::system::error_code make_error_code(qflow::oauth2_errors e)
{
    return boost::system::error_code(
               static_cast<int>(e), oauth2_category);
}
}
namespace boost {
namespace system {
template<> struct is_error_code_enum<qflow::oauth2_errors> : public std::true_type {};
}
}


namespace qflow{

struct oauth2_details
{
    std::string redirect_url;
    std::string code_exchange_url;

};

template<class CompletionToken, class RequestType>
auto async_oauth2_authenticate(const RequestType& req,
                               boost::asio::ip::tcp::socket& sock, CompletionToken&& token)
{
    beast::async_completion<CompletionToken, void(boost::system::error_code)> completion(token);
    boost::asio::spawn(sock.get_io_service(),
                       [&sock, &req, handler = completion.handler](boost::asio::yield_context yield)
    {
        qflow::uri url(req.url);
        auto ec = make_error_code(oauth2_errors::authenticated);
        try {
            if(url.contains_query_item("code"))
            {
                std::string vurl_str = "https://graph.facebook.com/v2.8/oauth/access_token?client_id=1067375046723087&redirect_uri=http://localhost:1234/test/resource&client_secret=8be865cb794eb995acf8b01443116a3e&code=" + url.query_item_value("code");

                beast::http::request<beast::http::empty_body> token_req;
                token_req.method = "GET";
                token_req.version = 11;

                auto token_res = async_send_request(vurl_str, token_req, sock.get_io_service(), yield);
                auto parsed = json::parse(token_res.body);
                auto err_it=parsed.find("error");
                if(err_it != parsed.end())
                {
                    auto err = err_it.value();
                    std::string message = err["message"];
                    int code = err["code"];
                    ec = make_error_code(static_cast<oauth2_errors>(code));
                }
                if(parsed.find("access_token") != parsed.end())
                {
                    std::string access_token = parsed["access_token"];
                    std::string inspect_str = "https://graph.facebook.com/debug_token?input_token=" + access_token + "&access_token=1067375046723087|8be865cb794eb995acf8b01443116a3e";
                    auto details = async_send_request(inspect_str, token_req, sock.get_io_service(), yield);
                    std::string me_str = "https://graph.facebook.com/v2.8/me?access_token=" + access_token;
                    auto me = async_send_request(me_str, token_req, sock.get_io_service(), yield);
                    int t=0;
                    
                }

                int i=0;
            }
            else
            {
                beast::http::response<beast::http::empty_body> res;
                res.version = req.version;
                res.status = 303;
                res.fields.insert("Server", "http_async_server");
                res.fields.insert("Location", "https://www.facebook.com/v2.8/dialog/oauth?client_id=1067375046723087&redirect_uri=http://localhost:1234" + url.path());
                beast::http::prepare(res);
                beast::http::async_write(sock, res, yield);
                ec = make_error_code(oauth2_errors::continue_required);
            }
        }
        catch (boost::system::system_error& e)
        {
            ec = e.code();
        }
        boost::asio::asio_handler_invoke(std::bind(handler, ec), &handler);
    });
    return completion.result.get();
}

}


#endif
