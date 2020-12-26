#include "Bind.hpp"

Bind::Bind(tcp::socket client, string DSTPORT, string DSTIP, boost::asio::io_context& io_context)
    : client(move(client)), acceptorr(io_context, tcp::endpoint(tcp::v4(), 0)) {
    memset(client_buf, 0x00, max_length);
    memset(server_buf, 0x00, max_length);
}

void Bind::bind_reply() {
    auto self(shared_from_this());
    port = acceptorr.local_endpoint().port();
    unsigned short p1 = port / 256;
    unsigned short p2 = port % 256;
    unsigned char reply[8] = {0, 90, p1, p2, 0, 0, 0, 0};
    memcpy(client_buf, reply, 8);
    boost::asio::async_write(
        client, boost::asio::buffer(client_buf, 8),
        [this, self](boost::system::error_code ec, size_t /*length*/) {
            if (!ec) {
                do_accept();
            }
            else{
                // cout << "bind_reply" << endl;
                // cout << ec.message() << endl;
            }
        }
    );
}

void Bind::do_accept() {
    auto self(shared_from_this());
    acceptorr.async_accept(
        [this, self](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                make_shared<BindSocket>(move(client), move(socket), port)->bind_reply();
                acceptorr.close();
            }
            else {
                // cout << "do_accept" << endl;
                // cout << ec.message() << endl;
            }
        }
    );
}

BindSocket::BindSocket(tcp::socket client, tcp::socket server, unsigned short port) 
    : client(move(client)), server(move(server)) {
        this->port = port;
    }

void BindSocket::bind_reply() {
    auto self(shared_from_this());
    unsigned char p1 = port / 256;
    unsigned char p2 = port % 256;
    unsigned char reply[8] = {0, 90, p1, p2, 0, 0, 0, 0};
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

void BindSocket::read_client() {
    auto self(shared_from_this());
    client.async_read_some(
        boost::asio::buffer(client_buf, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                write_server(length);
            }
            else{
                // cout << "read_client" << endl;
                // cout << ec.message() << endl;
                client.close();
            }
        }
    );
}

void BindSocket::write_server(size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(
        server, boost::asio::buffer(client_buf, length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                read_client();
            }
            else{
                // cout << "write_server" << endl;
                // cout << ec.message() << endl;
                client.close();
            }
        }
    );
}

void BindSocket::read_server() {
    auto self(shared_from_this());
    server.async_read_some(
        boost::asio::buffer(server_buf, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                write_client(length);
            }
            else{
                // cout << "read_server" << endl;
                // cout << ec.message() << endl;
                server.close();
            }
        }
    );
}

void BindSocket::write_client(size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(
        client, boost::asio::buffer(server_buf, length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                read_server();
            }
            else {
                // cout << "write_client" << endl;
                // cout << ec.message() << endl;
                server.close();
            }
        }
    );
}
