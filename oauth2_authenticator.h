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
    error = 2,
    finished = 0
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
        case oauth2_errors::finished :
            return "authentication finished";
        case oauth2_errors::error :
            return "error";
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
    std::string inspect_url;
};
struct oauth2_result
{
    std::string access_token;
    std::string user_id;
    std::string error;
};

template<class CompletionToken, class RequestType>
auto async_oauth2_authenticate(const RequestType& req,
                               boost::asio::ip::tcp::socket& sock, CompletionToken&& token)
{
    beast::async_completion<CompletionToken, void(boost::system::error_code, oauth2_result)> completion(token);
    boost::asio::spawn(sock.get_io_service(),
                       [&sock, &req, handler = completion.handler](boost::asio::yield_context yield)
    {
        auto ec = make_error_code(oauth2_errors::finished);
        oauth2_result res;
        RequestType new_req = req;
        try {
            qflow::uri url(new_req.url);
            std::string host = new_req.fields["Host"].to_string();
            if(!url.contains_query_item("code"))
            {
                std::string redirect_uri = "http://" + host + new_req.url;
                std::string redirect_uri_encoded = uri::url_encode(redirect_uri);
                beast::http::response<beast::http::empty_body> res;
                res.version = new_req.version;
                res.status = 303;
                res.fields.insert("Server", "http_async_server");
                res.fields.insert("Location", "https://www.facebook.com/v2.8/dialog/oauth?client_id=1067375046723087&redirect_uri=" + redirect_uri);
                beast::http::prepare(res);
                beast::http::async_write(sock, res, yield);
                
                beast::streambuf sb;
                beast::http::async_read(sock, sb, new_req, yield);
                
                url = uri(new_req.url);
                host = new_req.fields["Host"].to_string();
            }
            
            if(url.contains_query_item("code"))
            {
                std::string redirect_uri = "http://" + host + url.path();
                std::string redirect_uri_encoded = uri::url_encode(redirect_uri);
                std::string vurl_str = "https://graph.facebook.com/v2.8/oauth/access_token?client_id=1067375046723087&redirect_uri=http://" + host + new_req.url + "&client_secret=8be865cb794eb995acf8b01443116a3e&code=" + url.query_item_value("code");

                beast::http::request<beast::http::empty_body> token_req;
                token_req.method = "GET";
                token_req.version = 11;

                auto token_res = async_send_request(vurl_str, token_req, sock.get_io_service(), yield);
                auto parsed = json::parse(token_res.body);
                auto err_it=parsed.find("error");
                if(err_it != parsed.end())
                {
                    res.error = token_res.body;
                }
                if(parsed.find("access_token") != parsed.end())
                {
                    std::string access_token = parsed["access_token"];
                    std::string inspect_str = "https://graph.facebook.com/debug_token?input_token=" + access_token + "&access_token=1067375046723087|8be865cb794eb995acf8b01443116a3e";
                    auto details = async_send_request(inspect_str, token_req, sock.get_io_service(), yield);
                    auto parsed = json::parse(details.body);
                    auto data=parsed["data"];
                    std::string user_id = data["user_id"]; 
                    res.access_token = access_token;
                    res.user_id = user_id;
                }
            }
        }
        catch (boost::system::system_error& e)
        {
            ec = e.code();
        }
        boost::asio::asio_handler_invoke(std::bind(handler, ec, res), &handler);
    });
    return completion.result.get();
}

}


#endif
