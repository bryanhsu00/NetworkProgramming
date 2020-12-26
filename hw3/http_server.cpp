#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <boost/algorithm/string/classification.hpp>  // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp>  // Include for boost::split
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using boost::asio::ip::tcp;
using namespace std;

class session : public enable_shared_from_this<session> {
   public:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];

    session(tcp::socket socket) : socket_(move(socket)) {}

    void start() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    string data_str(data_);
                    map<string, string> map_;
                    vector<string> sentences, words;
                    boost::split(sentences, data_str, boost::is_any_of("\r\n"), boost::token_compress_on);
                    boost::split(words, sentences[0], boost::is_any_of(" "), boost::token_compress_on);

                    map_["REQUEST_METHOD"] = words[0];
                    map_["REQUEST_URI"] = words[1];
                    map_["SERVER_PROTOCOL"] = words[2];
                    map_["HTTP_HOST"] = sentences[1].substr(sentences[1].find(" ")+1);
                    map_["SERVER_ADDR"] = socket_.local_endpoint().address().to_string();
                    map_["SERVER_PORT"] = to_string(socket_.local_endpoint().port());
                    map_["REMOTE_ADDR"] = socket_.remote_endpoint().address().to_string();;
                    map_["REMOTE_PORT"] = to_string(socket_.remote_endpoint().port());;

                    string path = map_["REQUEST_URI"];
                    path = path.substr(1);
                    if(words[1].find("?") != string::npos) {
                        path = path.substr(0, path.find("?"));
                        map_["QUERY_STRING"] = words[1].substr(words[1].find("?")+1);
                    }
                    else {
                        map_["QUERY_STRING"] = "";
                    }
                    do_script(map_, path);
                }
            }
        );
    }

    void do_script(map<string, string> map_, string path) {
        auto self(shared_from_this());
        strcpy(data_, "HTTP/1.0 200 OK\r\n");
        socket_.async_write_some(
            boost::asio::buffer(data_, strlen(data_)),
            [this, self, map_, path](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    pid_t pid = fork();
                    if (pid == 0) {
                        set_env(map_);
                        dup2(socket_.native_handle(), STDIN_FILENO);
                        dup2(socket_.native_handle(), STDOUT_FILENO);
                        if(execvp(path.c_str(), NULL) == -1) {
                            cerr << "Error: " << path << endl;
                        }
                    }
                    else {
                        socket_.close();
                    }
                }
            }
        );
    }

    void set_env(map<string, string> map_) {
        clearenv();
        for (auto& m : map_)
            setenv(m.first.c_str(), m.second.c_str(), 1);
    }

};

class server {
   public:
    tcp::acceptor acceptor_;

    server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    cout << "receive connect !" << endl;
                    make_shared<session>(move(socket))->start();
                }
                do_accept();
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

        cerr << "starting ~" << endl;

        io_context.run();
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}