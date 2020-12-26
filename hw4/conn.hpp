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

using boost::asio::ip::tcp;
using namespace std;

class conn : public enable_shared_from_this<conn> {
   public:
    tcp::socket client;
    tcp::socket server;
    tcp::endpoint endpoint;
    enum { max_length = 1048576 };
    unsigned char client_buf[max_length];
    unsigned char server_buf[max_length];

    conn(tcp::socket client, 
        tcp::socket server, 
        tcp::endpoint endpoint
    );

    void start();

    void read_server();

    void read_client();

    void write_server(size_t length);

    void write_client(size_t length);

    void conn_reply();

    void conn_connect();

    // void printInfo(unsigned char* data, size_t length);

};