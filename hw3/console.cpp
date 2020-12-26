#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string/classification.hpp>  // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp>  // Include for boost::split
#include <boost/algorithm/string.hpp> // include Boost, a C++ library
#include <array>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

using boost::asio::ip::tcp;
using namespace std;

boost::asio::io_context io_context;

class session : public enable_shared_from_this<session> {
   public:
    tcp::socket socket_;
    tcp::endpoint endpoint_;
    enum { max_length = 1048576 };
    char input[max_length];
    char output[max_length];
    vector<string> file;
    string index;

    session(tcp::socket socket, tcp::endpoint endpoint, string filename, string idx)
    : socket_(move(socket)) {
        endpoint_ = endpoint;
        file = get_file_content(filename);
        index = idx;
    }

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
                    if(data.find("%") != std::string::npos) { // contain %
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
        cout << cmd;
        output_command(cmd);
        boost::asio::async_write(
            socket_, boost::asio::buffer(output, strlen(output)),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    do_read();
                }
            }
        );
    }

    void output_shell(string str) {
        escape(str);
        boost::replace_all(str, "\n", "&NewLine;");
        boost::replace_all(str, "\r", "");
        cout << "<script>document.getElementById(\'s" + index + "\').innerHTML += \'" + str + "\';</script>" << flush;
    }

    void output_command(string str) {
        escape(str);
        boost::replace_all(str, "\n", "&NewLine;");
        boost::replace_all(str, "\r", "");
        cout << "<script>document.getElementById(\'s" + index + "\').innerHTML += \'<b>" + str + "</b>\';</script>" << flush;
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
};

int main() {
    try {
        string header = "Content-type: text/html\r\n\r\n";
        cout << header;
        string html = R""""(
            <!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8" />
                <title>NP Project 3 Sample Console</title>
                <link
                rel="stylesheet"
                href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
                integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
                crossorigin="anonymous"
                />
                <link
                href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
                rel="stylesheet"
                />
                <link
                rel="icon"
                type="image/png"
                href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
                />
                <style>
                * {
                    font-family: 'Source Code Pro', monospace;
                    font-size: 1rem !important;
                }
                body {
                    background-color: #212529;
                }
                pre {
                    color: #cccccc;
                }
                b {
                    color: #01b468;
                }
                </style>
            </head>
            <body>
                <table class="table table-dark table-bordered">
                <thead>
                    <tr>
                    <th scope="col">server 0</th>
                    <th scope="col">server 1</th>
                    <th scope="col">server 2</th>
                    <th scope="col">server 3</th>
                    <th scope="col">server 4</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                    <td><pre id="s0" class="mb-0"></pre></td>
                    <td><pre id="s1" class="mb-0"></pre></td>
                    <td><pre id="s2" class="mb-0"></pre></td>
                    <td><pre id="s3" class="mb-0"></pre></td>
                    <td><pre id="s4" class="mb-0"></pre></td>
                    </tr>
                </tbody>
                </table>
            </body>
            </html>
        )"""";
        cout << html;
        vector<string> params;
        string param = string(getenv("QUERY_STRING"));
        boost::split(params, param, boost::is_any_of("&"), boost::token_compress_on);
        for(int i=0; i<params.size(); i+=3) {
            string index = to_string(i / 3);
            string host = params[i].substr(params[i].find("=")+1);
            string port = params[i+1].substr(params[i+1].find("=")+1);
            string file = "test_case/" + params[i+2].substr(params[i+2].find("=")+1);

            if(host != "" && port != "" && file != "") {
                tcp::resolver resolver(io_context);
                tcp::resolver::query query(host, port);
                tcp::resolver::iterator iter = resolver.resolve(query);
                tcp::endpoint endpoint = iter->endpoint();
                tcp::socket socket(io_context);
                make_shared<session>(move(socket), endpoint, file, index)->start();
            }
        }
        io_context.run();
    }
    catch (exception& e) {
        cerr << e.what() << endl;
    }
}