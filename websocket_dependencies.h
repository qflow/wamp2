#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>

namespace qflow{
using server = websocketpp::server<websocketpp::config::asio>;
using client = websocketpp::client<websocketpp::config::asio>;
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::ref;
using message_ptr = websocketpp::config::asio::message_type::ptr;
}
