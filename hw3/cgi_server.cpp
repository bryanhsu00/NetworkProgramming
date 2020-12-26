#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <boost/algorithm/string/classification.hpp>  // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp>  // Include for boost::split
#include <boost/algorithm/string.hpp> // include Boost, a C++ library
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <fstream>

using boost::asio::ip::tcp;
using namespace std;

// g++ cgi_server.cpp -o cgi_server -lws2_32 -lwsock32 -std=c++14

boost::asio::io_context io_context;

class client : public enable_shared_from_this<client> {
   public:
    tcp::socket socket_;
    // tcp::socket* client_;
    shared_ptr<tcp::socket> client_;
    tcp::endpoint endpoint_;
    enum { max_length = 1048576 };
    char input[max_length];
    char output[max_length];
    vector<string> file;
    string index;
    string last_index;

    void start() {
        auto self(shared_from_this());
        socket_.async_connect(
            endpoint_,
            [this, self](const boost::system::error_code& error){
                if(!error) {
                    do_read();
                }
            }
        );
    }

    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(input, max_length),
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    string data = string(input);
                    memset(input, '\0', max_length);
                    memset(output, '\0', max_length);
                    output_shell(data);
                    if(data.find("%") != std::string::npos) { // contain % it means i can write to remote
                        do_write();
                    }
                    else{
                        do_read();
                    }
                }
            }
        );
    }

    void do_write() {
        auto self(shared_from_this());
        string cmd = file[0];
        file.erase(file.begin());
        strcpy(output, cmd.c_str());
        output_command(cmd);
        boost::asio::async_write(
            socket_, boost::asio::buffer(output, strlen(output)),
            [this, self, cmd](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    do_read();
                    if (cmd.find("exit") != string::npos) {
                        socket_.close();
                        cout << "close remote" << endl;
                        // cout << client_.use_count() << endl;
                        if(client_.use_count() == 2){
                            client_->close();
                        }
                    }
                    else {
                        do_read();
                    }
                }
            }
        );
    }

    void output_shell(string str) {
        auto self(shared_from_this());
        char buf[max_length];
        escape(str);
        boost::replace_all(str, "\n", "&NewLine;");
        boost::replace_all(str, "\r", "");
        string msg = "<script>document.getElementById(\'s" + index + "\').innerHTML += \'" + str + "\';</script>";
        strcpy(buf, msg.c_str());
        boost::asio::async_write(
            *client_, boost::asio::buffer(buf, strlen(buf)),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (ec) {
                    cerr << ec << endl;
                }
            }
        );
    }

    void output_command(string str) {
        auto self(shared_from_this());
        char buf[max_length];
        escape(str);
        boost::replace_all(str, "\n", "&NewLine;");
        boost::replace_all(str, "\r", "");
        string msg = "<script>document.getElementById(\'s" + index + "\').innerHTML += \'<b>" + str + "</b>\';</script>";
        strcpy(buf, msg.c_str());
        boost::asio::async_write(
            *client_, boost::asio::buffer(buf, strlen(buf)),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (ec) {
                    cerr << ec << endl;
                }
            }
        );
    }

    void escape(string& str) {
        boost::replace_all(str, "&", "&amp;");
        boost::replace_all(str, "\"", "&quot;");
        boost::replace_all(str, "\'", "&apos;");
        boost::replace_all(str, "<", "&lt;");
        boost::replace_all(str, ">", "&gt;");
    }

    vector<string> get_file_content(string filename) {
        vector<string> result;
        string myText;
        ifstream MyReadFile(filename);
        while (getline (MyReadFile, myText)) {
            result.push_back(myText+"\n");
        }
        MyReadFile.close(); 
        return result;
    }

    client(tcp::socket socket, tcp::endpoint endpoint, string filename, string idx, shared_ptr<tcp::socket> cli)
    : socket_(move(socket)), client_(cli) {
        endpoint_ = endpoint;
        file = get_file_content(filename);
        index = idx;
    }
};

// ----------------

class session : public enable_shared_from_this<session> {
   public:
    tcp::socket socket_;
    enum { max_length = 1048576 };
    char input[max_length];
    char output[max_length];
    vector<string> file;
    map<string, string> map_;

    session(tcp::socket socket) : socket_(move(socket)) {}

    void do_cmds() {
        shared_ptr<tcp::socket> ptrr(&socket_);
        vector<string> params;
        string param = map_["QUERY_STRING"];
        boost::split(params, param, boost::is_any_of("&"), boost::token_compress_on);
        for(int i=0; i<params.size(); i+=3) {
            string index = to_string(i / 3);
            string host = params[i].substr(params[i].find("=")+1);
            string port = params[i+1].substr(params[i+1].find("=")+1);
            string file = "test_case/" + params[i+2].substr(params[i+2].find("=")+1);
            if(host != "" && port != "" && file != "") {
                boost::asio::ip::tcp::resolver resolver(io_context);
                tcp::resolver::query query(host, port);
                tcp::resolver::iterator iter = resolver.resolve(query);
                tcp::endpoint endpoint = iter->endpoint();
                tcp::socket socket(io_context);
                make_shared<client>(move(socket), endpoint, file, index, ptrr)->start();
            }
        }
        try {
            io_context.run();
        }
        catch (exception& e) {
            cerr << "Exception: " << e.what() << "\n";
        }
    }

    vector<string> get_file_content(string filename) {
        vector<string> result;
        string myText;
        ifstream MyReadFile(filename);
        while (getline (MyReadFile, myText)) {
            result.push_back(myText+"\n");
        }
        MyReadFile.close(); 
        return result;
    }

    void do_write_html() {
        auto self(shared_from_this());
        string cmd = file[0];
        file.erase(file.begin());
        strcpy(output, cmd.c_str());
        boost::asio::async_write(
            socket_, boost::asio::buffer(output, strlen(output)),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    if(file.size() != 0){
                        do_write_html();
                    }
                    else if(file.size() == 0 && map_["REQUEST_URI"].find("console.cgi") != string::npos){
                        do_cmds();
                    }
                }
            }
        );
    }

    void start() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(input, max_length),
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    string data_str(input);
                    vector<string> sentences, words;
                    boost::split(sentences, data_str, boost::is_any_of("\r\n"), boost::token_compress_on);
                    boost::split(words, sentences[0], boost::is_any_of(" "), boost::token_compress_on);

                    map_["REQUEST_METHOD"] = words[0];
                    map_["REQUEST_URI"] = words[1];
                    if(words[1].find("?") != string::npos) {
                        map_["QUERY_STRING"] = words[1].substr(words[1].find("?")+1);
                    }
                    else {
                        map_["QUERY_STRING"] = "";
                    }

                    if(map_["REQUEST_URI"].find("panel.cgi") != string::npos) {
                        file = get_file_content("panel.html");
                        strcpy(output, "HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n");
                        boost::asio::async_write(
                            socket_, boost::asio::buffer(output, strlen(output)),
                            [this, self](boost::system::error_code ec, size_t /*length*/) {
                                if (!ec) {
                                    if(file.size() != 0){
                                        do_write_html();
                                    }
                                }
                            }
                        );
                    }
                    else if(map_["REQUEST_URI"].find("console.cgi") != string::npos) {
                        file = get_file_content("console.html");
                        strcpy(output, "HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n");
                        boost::asio::async_write(
                            socket_, boost::asio::buffer(output, strlen(output)),
                            [this, self](boost::system::error_code ec, size_t /*length*/) {
                                if (!ec) {
                                    if(file.size() != 0){
                                        do_write_html();
                                    }
                                }
                            }
                        );
                    }
                    else{
                        socket_.close();
                    }
                }
            }
        );
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

        // boost::asio::io_context io_context;

        server s(io_context, atoi(argv[1]));

        cerr << "starting ~" << endl;

        io_context.run();
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
