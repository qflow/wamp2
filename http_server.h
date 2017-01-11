#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "future_handler.h"

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>
#include <string>
#include <boost/array.hpp>
#include <beast/http.hpp>
#include <beast/core/placeholders.hpp>
#include <beast/core/streambuf.hpp>
#include "oauth2_authenticator.h"

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
    void operator()()
    {
        auto self(shared_from_this());
        boost::asio::spawn(strand_,
                           [this, self](boost::asio::yield_context yield)
        {
            try
            {
                beast::streambuf sb;
                req_type req;
                beast::http::async_read(socket_, sb, req, yield);
                auto details = qflow::async_oauth2_authenticate(req, socket_, yield);
                int t = 0;
            }
            catch (boost::system::system_error& e)
            {
                auto message = e.what();
            }
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
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
    void operator()()
    {
        boost::asio::spawn(io_service_,
                           [this](boost::asio::yield_context yield)
        {
            tcp::resolver resolver(io_service_);
            tcp::resolver::query query(address_, port_);
            tcp::acceptor acceptor(io_service_, *resolver.resolve(query));

            for (;;)
            {
                boost::system::error_code ec;
                tcp::socket socket(io_service_);
                acceptor.async_accept(socket, yield[ec]);
                if (!ec) std::make_shared<session>(std::move(socket))->operator()();
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
