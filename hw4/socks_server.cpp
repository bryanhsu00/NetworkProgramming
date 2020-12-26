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
#include "router.hpp"

using boost::asio::ip::tcp;
using namespace std;

class server {
   public:
    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context;

    server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), io_context(io_context) {
        // cout << acceptor_.local_endpoint().port() << endl;
        do_accept();
    }

    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    io_context.notify_fork(boost::asio::io_context::fork_prepare);
                    if (fork() == 0) { // child
                        io_context.notify_fork(boost::asio::io_context::fork_child);
                        acceptor_.close();
                        make_shared<router>(move(socket), io_context)->do_parse();
                    }
                    else{ // parent
                        io_context.notify_fork(boost::asio::io_context::fork_parent);
                        socket.close();
                        do_accept();
                    }
                }
            }
        );
    }
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        server s(io_context, atoi(argv[1]));
        cout << "starting !" << endl;
        io_context.run();

    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

// try {
//     // boost::asio::io_service netService;
//     udp::resolver   resolver(netService);
//     udp::resolver::query query(udp::v4(), "google.com", "");
//     udp::resolver::iterator endpoints = resolver.resolve(query);
//     udp::endpoint ep = *endpoints;
//     udp::socket socket(netService);
//     socket.connect(ep);
//     boost::asio::ip::address addr = socket.local_endpoint().address();
//     std::cout << "My IP according to google is: " << addr.to_string() << std::endl;
//  } catch (std::exception& e){
//     std::cerr << "Could not deal with socket. Exception: " << e.what() << std::endl;

//  }