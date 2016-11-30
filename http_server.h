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
using handler_type = std::function<void(const std::shared_ptr<req_type>&, 
                                        std::shared_ptr<resp_type>&)>;
                                    

class http_server : boost::asio::coroutine
{
public:
    explicit http_server(boost::asio::io_service& io_service,
                         const std::string& address, const std::string& port, 
                         handler_type handler) : handler_(handler)
    {
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(address, port);
        acceptor_.reset(new tcp::acceptor(io_service, *resolver.resolve(query)));
    }

#include <boost/asio/yield.hpp>
    void operator()(boost::system::error_code ec = boost::system::error_code())
    {
        if (!ec)
        {
            reenter (this)
            {
                do
                {
                    socket_.reset(new tcp::socket(acceptor_->get_io_service()));
                    yield acceptor_->async_accept(*socket_, *this);
                    fork http_server(*this)();
                } while (is_parent());

                request_ = std::make_shared<req_type>();
                sb_ = std::make_shared<beast::streambuf>();
                yield beast::http::async_read(*socket_, *sb_, *request_, *this);
                
                yield qflow::async_oauth2_authenticate(*request_, *socket_, *this);
                
                int i=0;
                
                
                response_ = std::make_shared<resp_type>();
                //handler_(request_, response_);
                //beast::http::prepare(*response_);
                //yield beast::http::async_write(*socket_, *response_, *this);
                //socket_->shutdown(tcp::socket::shutdown_both, ec);
            }
        }
    }
#include <boost/asio/unyield.hpp>

private:
        typedef boost::asio::ip::tcp tcp;
        handler_type handler_;
        std::shared_ptr<tcp::acceptor> acceptor_;
        std::shared_ptr<tcp::socket> socket_;
        std::shared_ptr<req_type> request_;
        std::shared_ptr<resp_type> response_;
        std::shared_ptr<beast::streambuf> sb_;

    };
}
#endif
