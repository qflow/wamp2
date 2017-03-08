#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "future_handler.h"

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>
#include <string>
#include <beast/http.hpp>
//#include <beast/http/parser_v1.hpp>
#include <beast/core/placeholders.hpp>
#include <beast/core/streambuf.hpp>
#include <beast/core/to_string.hpp>
#include <fstream>
#include "oauth2_authenticator.h"

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "proxy_config.pb.h"

using req_type = beast::http::request<beast::http::string_body>;
using resp_type = beast::http::response<beast::http::string_body>;
namespace qflow {

class http_session : public std::enable_shared_from_this<http_session>
{
public:
    explicit http_session(tcp::socket socket)
        : socket_(std::move(socket)), strand_(socket_.get_io_service())
    {
    }
    void operator()(const Proxy& proxy_config)
    {
        auto self(shared_from_this());
        boost::asio::spawn(strand_,
                           [this, self, proxy_config](boost::asio::yield_context yield)
        {
            tcp::socket sock {socket_.get_io_service()};
            req_type req;
            try
            {
                beast::streambuf sb;
                beast::http::async_read(socket_, sb, req, yield);
                tcp::resolver r {socket_.get_io_service()};
                auto i = r.async_resolve(tcp::resolver::query {req.fields["Host"].to_string(), "http"}, yield);
                boost::asio::async_connect(sock, i, yield);
                beast::http::async_write(sock, req, yield);
                /*if(!url.url_path_query().empty())
                    req.url = url.url_path_query();
                std::string host = url.host() +  ":" + boost::lexical_cast<std::string>(sock.remote_endpoint().port());
                req.fields.replace("Host", host);
                std::cout << req;*/
                /*beast::http::response_header header;
                beast::streambuf sb2;
                beast::http::async_read(sock, sb2, header, yield);
                beast::http::async_write(socket_, header, yield);
                boost::asio::async_write(socket_, sb2.data(), yield);*/
                resp_type resp;
                beast::streambuf sb2;
                beast::http::async_read(sock, sb2, resp, yield);
                beast::http::async_write(socket_, resp, yield);
                //auto details = qflow::async_oauth2_authenticate(req, socket_, yield);
                //beast::http::async_write(socket_, resp, yield);
                /*beast::http::parser_v1<false, beast::http::string_body, beast::http::fields> parser;
                beast::streambuf sb2;
                beast::http::async_parse(sock, sb2, parser, yield);
                auto a = parser.get();
                beast::http::async_write(socket_, a, yield);
                std::cout << a;*/
                
            }
            catch (boost::system::system_error& e)
            {
                auto message = e.what();
                std::cout << message << "\n\n";
            }
        });
    }
private:
    tcp::socket socket_;
    boost::asio::io_service::strand strand_;
};

template<typename session>
class tcp_server : boost::asio::coroutine
{
public:
    explicit tcp_server(boost::asio::io_service& io_service,
                        const std::string& address, const std::string& port) :
        address_(address), port_(port), io_service_(io_service)
    {
    }
    template<class ... Types>
    void operator()(Types... args)
    {
        boost::asio::spawn(io_service_,
                           [this, args...](boost::asio::yield_context yield)
        {
            tcp::resolver resolver(io_service_);
            tcp::resolver::query query(address_, port_);
            tcp::acceptor acceptor(io_service_, *resolver.resolve(query));
            acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            for (;;)
            {
                boost::system::error_code ec;   
                tcp::socket socket(io_service_);
                acceptor.async_accept(socket, yield[ec]);
                if (!ec) std::make_shared<session>(std::move(socket))->operator()(args...);
            }
        });

    }

private:
    std::string address_;
    std::string port_;
    boost::asio::io_service& io_service_;
};
}
#endif



