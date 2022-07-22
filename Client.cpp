#include "Client.hpp"
#include "Socket.hpp"
#include "rlncurses.hpp"
#include <iostream>
#include <regex>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <vector>

Client::Client(std::string address) {
    this->address = address;
    init();
}

Client::Client() { init(); }

void Client::init() {
    delete this->socket;
    delete this->meWithInfo;
    isConnectedMutex.lock();
    this->_isConnected = false;
    isConnectedMutex.unlock();
    this->socket = new Socket(AF_INET, SOCK_STREAM, 0);
    meWithInfo = new SocketWithInfo(socket, true);
}

int Client::start() {

    GUI::log("Attempting to connect to " + address + ":" + DEFAULT_PORT);

    socket->setBlocking(false);

    int status = socket->connect(address, DEFAULT_PORT);

    socket->setBlocking(true);

    if (status != 0) {

        std::string errorStr = gai_strerror(status);

        if (errorStr == "Unknown error") {
            errorStr = strerror(status);
        }

        GUI::log("Error connecting to " + address + ":" + DEFAULT_PORT + ": " +
                 errorStr);

        this->stop();

        init();

        return status;
    }

    sendMessage("/whoami");

    isConnectedMutex.lock();
    this->_isConnected = true;
    isConnectedMutex.unlock();

    startListening();

    GUI::log("Client connected on " + address + ":" + DEFAULT_PORT);
    return 0;
}

int Client::start(std::string address) {
    this->address = address;
    return start();
}

std::string Client::readMessage() {
    std::string message;
    this->socket->socketRead(message, MAX_MSG_SIZE + 100);
    return message;
}

int Client::safeReadMessage(std::string &message) {
    return this->socket->socketSafeRead(message, MAX_MSG_SIZE + 100, 1);
}

void Client::sendMessage(std::string message) {
    this->socket->socketWrite(message);
}

void Client::messageServer(std::string message) {
    while (message.length() > MAX_MSG_SIZE) {
        std::string subMessage = message.substr(0, MAX_MSG_SIZE);
        message = message.substr(MAX_MSG_SIZE, message.length());
        this->sendMessage("/m " + subMessage);
    }

    this->sendMessage("/m " + message);
}

int Client::stop() {

    shouldBeListening = false;

    if (listenThread != nullptr) {
        listenThread->join();
    }

    isConnectedMutex.lock();
    this->_isConnected = false;
    isConnectedMutex.unlock();
    this->socket->close();
    return 0;
}
bool Client::isConnected(bool shouldLog) {

    isConnectedMutex.lock();
    bool conn = this->_isConnected;
    isConnectedMutex.unlock();

    if (shouldLog && !conn) {
        GUI::log("You must be connected to use this command!");
    }
    return conn;
}

bool Client::hasChannel(bool shouldLog) {
    if (meWithInfo->channel == "") {
        if (shouldLog) {
            GUI::log("You must be in a channel to use this command!");
        }
        return false;
    }
    return true;
}

void Client::startListening() {
    this->shouldBeListening = true;
    this->listenThread = new std::thread(&Client::_listen, this);
}

bool Client::isMuted() { return meWithInfo->isMuted; }

void Client::_listen() {
    std::string message;
    while (this->shouldBeListening) {

        isConnectedMutex.lock();

        bool conn = this->_isConnected;

        isConnectedMutex.unlock();

        if (conn) {
            if (safeReadMessage(message) >= 0) {
                if (message == "") {
                    GUI::log("Server disconnected!");
                    GUI::log("Closing client...");
                    this->shouldBeListening = false;
                    GUI::GetInstance("")->prepareClose(
                        "Press any key to exit...");
                } else if (message[0] == '/') {
                    std::regex regex("/youare (.+)");
                    std::smatch match;
                    std::regex_search(message, match, regex);
                    if (match.size() > 1) {
                        meWithInfo->nickname = match[1].str();

                        GUI::updatePrompt(meWithInfo);

                        continue;
                    }

                    regex = std::regex("/joined (.+) (.+)");
                    std::regex_search(message, match, regex);

                    if (match.size() > 1) {
                        meWithInfo->isAdmin = match[2].str() == "admin";
                        meWithInfo->channel = match[1].str();

                        GUI::updatePrompt(meWithInfo);

                        GUI::log("Joined channel " + meWithInfo->channel +
                                 " as " +
                                 (meWithInfo->isAdmin ? "admin" : "user") +
                                 " successfully!");
                        continue;
                    }

                    if (message == "/kicked") {

                        meWithInfo->isAdmin = false;
                        meWithInfo->channel = "";

                        GUI::updatePrompt(meWithInfo);

                        GUI::log(
                            "You have been kicked from your current channel!");

                        continue;
                    }

                    if (message == "/muted") {
                        meWithInfo->isMuted = true;
                        GUI::log("You have been muted!");
                        continue;
                    }

                    if (message == "/unmuted") {
                        meWithInfo->isMuted = false;
                        GUI::log("You have been unmuted!");
                        continue;
                    }

                    regex = std::regex("/msg (\\S+) (.+)");
                    std::regex_search(message, match, regex);

                    if (match.size() > 1) {
                        GUI::addToWindow(match[1].str() + ": " +
                                         match[2].str());
                        continue;
                    }

                } else {
                    GUI::addToWindow(message);
                }
            }
        }
    }
}
