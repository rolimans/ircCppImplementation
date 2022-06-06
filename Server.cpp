#include "Server.hpp"
#include "Socket.hpp"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <vector>

Server::Server(std::string address) {
    this->clients = std::vector<Socket *>();
    this->address = address;
    this->socket = new Socket(AF_INET, SOCK_STREAM, 0);
    int optValue = 1;
    socket->socketSetOpt(SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optValue);
}
int Server::start() {
    socket->bind(address, DEFAULT_PORT);
    std::cout << "Server started on " << address << ":" << DEFAULT_PORT
              << std::endl;
    std::cout << "Waiting for client connection!" << std::endl;
    socket->listen(3);
    Socket *client = socket->accept();
    this->clients.push_back(client);
    std::cout << "Client connected on " << address << ":" << DEFAULT_PORT
              << std::endl;
    return 0;
}

std::string Server::readMessage() {
    std::string message;
    this->clients[0]->socketRead(message, MAX_MSG_SIZE + 1);
    return message;
}

void Server::sendMessage(std::string message) {
    while (message.length() > MAX_MSG_SIZE) {
        std::string subMessage = message.substr(0, MAX_MSG_SIZE);
        message = message.substr(MAX_MSG_SIZE, message.length());
        this->clients[0]->socketWrite(subMessage);
    }
    this->clients[0]->socketWrite(message);
}

int Server::stop() {
    this->clients[0]->close();
    this->socket->close();
    return 0;
}
