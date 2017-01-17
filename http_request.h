#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "uri.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <beast/http.hpp>
#include <beast/core/async_completion.hpp>

namespace qflow{
using tcp = boost::asio::ip::tcp;

template<class CompletionToken, class RequestType>
auto async_send_request(const std::string& url_str, const RequestType& req,
                        boost::asio::io_service& service, CompletionToken&& token)
{
    using ResponseType = beast::http::response<beast::http::streambuf_body>;
    beast::async_completion<CompletionToken, void(boost::system::error_code, ResponseType)> completion(token);
    boost::asio::spawn(service,
                       [handler = completion.handler, &service, &req, url_str](boost::asio::yield_context yield)
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
        boost::asio::asio_handler_invoke(std::bind(handler, ec, std::move(resp)), &handler);
    });

    return completion.result.get();
}
    
}
#endif
