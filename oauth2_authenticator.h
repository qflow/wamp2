#ifndef OAUTH2_AUTHENTICATOR
#define OAUTH2_AUTHENTICATOR

#include "uri.h"

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/system/error_code.hpp>
#include <beast/http.hpp>
#include <beast/core/placeholders.hpp>
#include <beast/core/streambuf.hpp>

namespace qflow {
enum aouth2_errors
{
    continue_required = 1201,
    not_authenticated = 1101,
    authenticated = 1001
};

class oauth2_category :
    public boost::system::error_category
{
public:
    const char *name() const noexcept {
        return "oauth2";
    }
    std::string message(int ev) const
    {
        switch(ev)
        {
        case aouth2_errors::continue_required :
            return "continue required";
        case aouth2_errors::authenticated :
            return "authenticated";
        case aouth2_errors::not_authenticated :
            return "not authenticated";
        }
        return "undefined";
    }
};

template<class CompletionToken, class RequestType>
auto async_oauth2_authenticate(const RequestType& req,
                               boost::asio::ip::tcp::socket& sock, CompletionToken&& token)
{
    typename boost::asio::handler_type<CompletionToken,void(boost::system::error_code)>::type           handler(std::forward<CompletionToken>(token));
    boost::asio::async_result<decltype(handler)> result(handler);
    qflow::uri url(req.url);
    if(url.contains_query_item("code"))
    {
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
        beast::http::async_write(sock, res, [&handler, &sock](boost::system::error_code ec) {
            sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            handler(ec);
        });
    }
    return result.get();
}

}
boost::system::error_code make_error_code(qflow::aouth2_errors e)
{
    return boost::system::error_code(
               static_cast<int>(e), qflow::oauth2_category());
}
namespace boost {
namespace system {
template<> struct is_error_code_enum<qflow::aouth2_errors> : public std::true_type {};
}
}

#endif
