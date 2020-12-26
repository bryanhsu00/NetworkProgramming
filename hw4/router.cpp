#include "router.hpp"

router::router(tcp::socket socket, boost::asio::io_context& io_context)
     : client(move(socket)), io_context(io_context) {}

void router::do_parse() {
    auto self(shared_from_this());
    client.async_read_some(
        boost::asio::buffer(client_buf, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                VN = to_string(client_buf[0]);
                CD = to_string(client_buf[1]);
                DSTPORT = to_string(((int)client_buf[2] << 8) + (int)client_buf[3]);
                DSTIP = get_DSTIP();
                domain = get_domain(length);
                create_sock();
            }
        }
    );
}

bool router::is_valid() {
    vector<string> rules = get_file_content("socks.conf"); // 140.114.*.*
    for(int i=0; i<rules.size(); i++) {
        vector<string> rule_split;
        boost::split(rule_split, rules[i], boost::is_any_of(" "), boost::token_compress_on);
        if((rule_split[1] == "c" && CD == "1") || (rule_split[1] == "b" && CD == "2")) {
            string client_ip = client.remote_endpoint().address().to_string();
            string pattern = rule_split[2];
            boost::replace_all(pattern, ".", "\\.");
            boost::replace_all(pattern, "*", ".*");
            regex reg(pattern);
            if(regex_match(client_ip, reg)) {
                return true;
            }
        }
    }
    return false;
}

void router::send_reject() {
    auto self(shared_from_this());
    unsigned char reply[8] = {0, 91, 0, 0, 0, 0, 0, 0};
    memcpy(client_buf, reply, 8);
    boost::asio::async_write(
        client, boost::asio::buffer(client_buf, 8),
        [this, self](boost::system::error_code ec, size_t /*length*/) {
            if (!ec) {
                client.close();
            }
        }
    );
}

void router::create_sock() {
    if(CD == "1") {
        string host;
        if(domain != "")
            host = domain;
        else
            host = DSTIP;
        tcp::resolver resolver(io_context);
        tcp::resolver::query query(host, DSTPORT);
        tcp::resolver::iterator iter = resolver.resolve(query);
        tcp::endpoint endpoint = iter->endpoint();
        tcp::socket server(io_context);
        if(is_valid()) {
            cout << "<S_IP>: " << client.remote_endpoint().address().to_string() << endl
            << "<S_PORT>: " << client.remote_endpoint().port() << endl
            << "<D_IP>: " << endpoint.address().to_string() << endl
            << "<D_PORT>: " << DSTPORT << endl
            << "<Command>: " << "CONNECT" << endl
            << "<Reply>: " << "Accept" << endl 
            << endl;
            make_shared<conn>(move(client), move(server), endpoint)->conn_connect();
        }
        else {
            cout << "<S_IP>: " << client.remote_endpoint().address().to_string() << endl
            << "<S_PORT>: " << client.remote_endpoint().port() << endl
            << "<D_IP>: " << endpoint.address().to_string() << endl
            << "<D_PORT>: " << DSTPORT << endl
            << "<Command>: " << "CONNECT" << endl
            << "<Reply>: " << "Reject" << endl
            << endl;
            send_reject();
        }
    }
    else if(CD == "2") {
        if(is_valid()) {
            cout << "<S_IP>: " << client.remote_endpoint().address().to_string() << endl
            << "<S_PORT>: " << client.remote_endpoint().port() << endl
            << "<D_IP>: " << DSTIP << endl
            << "<D_PORT>: " << DSTPORT << endl
            << "<Command>: " << "BIND" << endl
            << "<Reply>: " << "Accept" << endl 
            << endl;
            make_shared<Bind>(move(client), DSTPORT, DSTIP, io_context)->bind_reply();
        }
        else {
            cout << "<S_IP>: " << client.remote_endpoint().address().to_string() << endl
            << "<S_PORT>: " << client.remote_endpoint().port() << endl
            << "<D_IP>: " << DSTIP << endl
            << "<D_PORT>: " << DSTPORT << endl
            << "<Command>: " << "BIND" << endl
            << "<Reply>: " << "Reject" << endl 
            << endl;
            send_reject();
        }
    }
}

vector<string> router::get_file_content(string filename) {
    vector<string> result;
    string myText;
    ifstream MyReadFile(filename);
    while (getline (MyReadFile, myText)) {
        result.push_back(myText+"\n");
    }
    MyReadFile.close(); 
    return result;
}

string router::get_domain(size_t length) {
    vector<int> null_position;
    for(int i=8; i<length; i++) {
        if(client_buf[i] == 0x00){
            null_position.push_back(i);
        }
    }
    if(null_position.size() == 1) {
        return "";
    }
    else{
        string domain = "";
        int start = null_position[0]+1;
        int end = null_position[1];
        for(int i=start; i<end; i++){
            domain += client_buf[i];
        }
        return domain;
    }
}

string router::get_DSTIP() {
    string DSTIP = "";
    for(int i=4; i<8; i++) {
        DSTIP += to_string(client_buf[i]);
        if(i != 7) 
            DSTIP += ".";
    }
    return DSTIP;
}
