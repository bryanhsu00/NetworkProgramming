#include <memory>
#include <utility>
#include <iomanip>
#include <sstream>
#include <string>
#include <array>
#include <iostream>
#include <fstream>
#include <vector>
#include <regex>
#include <string.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/classification.hpp>  // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp>  // Include for boost::split
#include <boost/algorithm/string.hpp> // include Boost, a C++ library
#include "conn.hpp"
#include "Bind.hpp"

using boost::asio::ip::tcp;
using namespace std;

class router : public enable_shared_from_this<router> {
   public:
    tcp::socket client;
    enum { max_length = 1048576 };
    unsigned char client_buf[max_length];
    boost::asio::io_context& io_context;
    string VN;
    string CD;
    string DSTPORT;
    string DSTIP;
    string domain;

    router(tcp::socket socket, boost::asio::io_context& io_context);

    void do_parse();

    string get_domain(size_t length);

    string get_DSTIP();

    void create_sock();

    vector<string> get_file_content(string filename);

    bool is_valid();

    void send_reject();

};