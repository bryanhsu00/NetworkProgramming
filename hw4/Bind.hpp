#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <stdio.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include <boost/algorithm/string/classification.hpp> // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp> // Include for boost::split

using boost::asio::ip::tcp;
using namespace std;

class Bind : public enable_shared_from_this<Bind> {
   public:
    tcp::socket client;
    tcp::acceptor acceptorr;
    enum { max_length = 1048576 };
    unsigned char client_buf[max_length];
    unsigned char server_buf[max_length];
    unsigned short port;

    Bind(tcp::socket client,
        string DSTPORT,
        string DSTIP,
        boost::asio::io_context& io_context
    );

    void do_accept();

    void bind_reply();

};

class BindSocket : public enable_shared_from_this<BindSocket> {
   public:
    tcp::socket client;
    tcp::socket server;
    enum { max_length = 1048576 };
    unsigned char client_buf[max_length];
    unsigned char server_buf[max_length];
    unsigned short port;

    BindSocket(tcp::socket client,
        tcp::socket server,
        unsigned short port
    );

    void bind_reply();

    void read_server();

    void read_client();

    void write_server(size_t length);

    void write_client(size_t length);

};