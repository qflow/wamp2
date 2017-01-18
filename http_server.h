#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "future_handler.h"

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/read_until.hpp>
#include <string>
#include <beast/http.hpp>
#include <beast/core/placeholders.hpp>
#include <beast/core/streambuf.hpp>
#include <beast/core/to_string.hpp>
#include <fstream>
#include "oauth2_authenticator.h"

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "proxy_config.pb.h"

namespace qflow {
using req_type = beast::http::request<beast::http::string_body>;
using resp_type = beast::http::response<beast::http::string_body>;

class http_session : public std::enable_shared_from_this<http_session>
{
public:
    explicit http_session(tcp::socket socket)
        : socket_(std::move(socket)), strand_(socket_.get_io_service())
    {
    }
    void operator()(const Proxy& proxy)
    {
        auto self(shared_from_this());
        boost::asio::spawn(strand_,
                           [this, self, proxy](boost::asio::yield_context yield)
        {
            try
            {
                beast::streambuf sb;
                req_type req;
                beast::http::async_read(socket_, sb, req, yield);
                std::cout << req;
                tcp::resolver r {socket_.get_io_service()};
                tcp::socket sock {socket_.get_io_service()};
                qflow::uri url("http://www.google.com");
                auto i = r.async_resolve(tcp::resolver::query {url.host(), url.scheme()}, yield);
                boost::asio::async_connect(sock, i, yield);
                if(!url.url_path_query().empty()) 
                    req.url = url.url_path_query();
                std::string host = url.host() +  ":" + boost::lexical_cast<std::string>(sock.remote_endpoint().port());
                req.fields.replace("Host", host);
                std::cout << req;
                beast::http::async_write(sock, req, yield);
                beast::http::response_header header;
                beast::streambuf sb2;
                beast::http::async_read(sock, sb2, header, yield);
                std::cout << header;
                beast::http::async_write(socket_, header, yield);
                boost::asio::async_write(socket_, sb2.data(), yield);
                char data[1024];
                while(int res = sock.async_read_some(boost::asio::buffer(data, 1024), yield))
                {
                    boost::asio::async_write(socket_, boost::asio::buffer(data, res), yield);
                }

                //auto details = qflow::async_oauth2_authenticate(req, socket_, yield);
                //beast::http::async_write(socket_, resp, yield);

                int t = 0;
            }
            catch (boost::system::system_error& e)
            {
                auto message = e.what();
                std::cout << message << "\n\n";
            }
            //socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
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
