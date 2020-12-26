#include "conn.hpp"

conn::conn(tcp::socket client, tcp::socket server, tcp::endpoint endpoint)
    : client(move(client)), server(move(server)) {
    this->endpoint = endpoint;
    memset(client_buf, 0x00, max_length);
    memset(server_buf, 0x00, max_length);
}

void conn::conn_connect() {
    // cout << "conn_connect" << endl;
    auto self(shared_from_this());
    server.async_connect(
        endpoint,
        [this, self](const boost::system::error_code& ec){
            if(!ec) {
                conn_reply();
            }
            else{
                // cout << "Connect error" << endl;
                // cout << ec.message() << endl;
            }
        }
    );
}

void conn::conn_reply() {
    // cout << "conn_reply" << endl;
    auto self(shared_from_this());
    unsigned char reply[8] = {0, 90, 0, 0, 0, 0, 0, 0};
    memcpy(client_buf, reply, 8);
    boost::asio::async_write(
        client, boost::asio::buffer(client_buf, 8),
        [this, self](boost::system::error_code ec, size_t /*length*/) {
            if (!ec) {
                read_client();
                read_server();
            }
        }
    );
}

void conn::read_client() {
    // cout << "read_client" << endl;
    auto self(shared_from_this());
    client.async_read_some(
        boost::asio::buffer(client_buf, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                write_server(length);
            }
            else{
                // cout << ec.message() << endl;
                client.close();
            }
        }
    );
}

void conn::write_server(size_t length) {
    // cout << "write_server" << endl;
    auto self(shared_from_this());
    boost::asio::async_write(
        server, boost::asio::buffer(client_buf, length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                read_client();
            }
            else{
                // cout << ec.message() << endl;
                client.close();
            }
        }
    );
}

void conn::read_server() {
    // cout << "read_server" << endl;
    auto self(shared_from_this());
    server.async_read_some(
        boost::asio::buffer(server_buf, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                write_client(length);
            }
            else{
                // cout << ec.message() << endl;
                server.close();
            }
        }
    );
}

void conn::write_client(size_t length) {
    // cout << "write_client" << endl;
    auto self(shared_from_this());
    boost::asio::async_write(
        client, boost::asio::buffer(server_buf, length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                read_server();
            }
            else {
                // cout << ec.message() << endl;
                server.close();
            }
        }
    );
}
