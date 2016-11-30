#ifndef OAUTH2_AUTHENTICATOR
#define OAUTH2_AUTHENTICATOR

#include "uri.h"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/system/error_code.hpp>
#include <beast/http.hpp>
#include <beast/core/placeholders.hpp>
#include <beast/core/streambuf.hpp>
#include <beast/core/async_completion.hpp>
#include <beast/core/bind_handler.hpp>

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



using tcp = boost::asio::ip::tcp;

template<class CompletionToken, class RequestType>
auto async_send_request(const std::string url_str, const RequestType& req,
                        boost::asio::io_service& service, CompletionToken&& token)
{
    using ResponseType = beast::http::response<beast::http::string_body>;
    beast::async_completion<CompletionToken, void(boost::system::error_code, ResponseType)> completion(token);
    boost::asio::spawn(service,
                       [&service, &req, handler = std::move(completion.handler), url_str](boost::asio::yield_context yield)
    {
        ResponseType resp;
        boost::system::error_code ec;
        try {
            RequestType new_req = req;
            tcp::resolver r {service};
            tcp::socket sock {service};
            qflow::uri url(url_str);
            auto i = r.async_resolve(tcp::resolver::query {url.host(), url.scheme()}, yield);
            boost::asio::async_connect(sock, i, yield);
            new_req.url = url.url_path_query();
            std::string host = url.host() +  ":" + boost::lexical_cast<std::string>(sock.remote_endpoint().port());
            new_req.fields.insert("Host", host);
            new_req.fields.insert("User-Agent", "qflow");
            beast::http::prepare(new_req);

            if(url.scheme() == "https")
            {
                boost::asio::ssl::context ctx {boost::asio::ssl::context::sslv23};
                boost::asio::ssl::stream<tcp::socket&> stream {sock, ctx};
                stream.set_verify_mode(boost::asio::ssl::verify_none);
                stream.async_handshake(boost::asio::ssl::stream_base::client, yield);
                beast::http::async_write(stream, new_req, yield);
                beast::streambuf sb;
                beast::http::async_read(stream, sb, resp, yield);
            }
            if(url.scheme() == "http")
            {
                beast::http::async_write(sock, new_req, yield);
                beast::streambuf sb;
                beast::http::async_read(sock, sb, resp, yield);
            }
            sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both);

        }
        catch (boost::system::system_error& e)
        {
            ec = e.code();
        }
        service.post(beast::bind_handler(std::move(handler), ec, std::move(resp)));
    });

    return completion.result.get();
}

template<class CompletionToken, class RequestType>
auto async_oauth2_authenticate(const RequestType& req,
                               boost::asio::ip::tcp::socket& sock, CompletionToken&& token)
{
    auto completion = std::make_shared<beast::async_completion<CompletionToken, void(boost::system::error_code)>>(token);
    qflow::uri url(req.url);
    if(url.contains_query_item("code"))
    {
        std::string vurl_str = "https://graph.facebook.com/v2.8/oauth/access_token?client_id=1067375046723087&redirect_uri=http://localhost:1234/test/resource&client_secret=8be865cb794eb995acf8b01443116a3e&code=" + url.query_item_value("code");

        beast::http::request<beast::http::empty_body> req;
        req.method = "GET";
        req.version = 11;

        async_send_request(vurl_str, req, sock.get_io_service(),
        [completion, &sock](boost::system::error_code ec, beast::http::response<beast::http::string_body> resp) {
            sock.get_io_service().post(beast::bind_handler(completion->handler, ec));
        });

    }
    else
    {
        beast::http::response<beast::http::empty_body> res;
        res.version = req.version;
        res.status = 303;
        res.fields.insert("Server", "http_async_server");
        res.fields.insert("Location", "https://www.facebook.com/v2.8/dialog/oauth?client_id=1067375046723087&redirect_uri=http://localhost:1234" + url.path());
        beast::http::prepare(res);
        beast::http::async_write(sock, res, [completion, &sock](boost::system::error_code ec) {
            sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            sock.get_io_service().post(beast::bind_handler(completion->handler, ec));
            
        });
    }
    return completion->result.get();
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
