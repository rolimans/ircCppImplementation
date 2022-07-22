#include "Server.hpp"
#include "Socket.hpp"
#include "rlncurses.hpp"
#include <algorithm>
#include <iostream>
#include <regex>
#include <string>
#include <sys/socket.h>
#include <unordered_map>
#include <vector>

Server::Server(std::string address) {
    this->clients = std::unordered_map<std::string, SocketWithInfo *>();
    this->channels = std::unordered_map<std::string, Channel *>();
    this->address = address;
    this->socket = new Socket(AF_INET, SOCK_STREAM, 0);
    int optValue = 1;
    socket->socketSetOpt(SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optValue);
}
int Server::start() {

    this->meWithInfo = new SocketWithInfo(socket, false);

    socket->bind(address, DEFAULT_PORT);

    this->shouldBeRunning = true;

    GUI::log("Server started on " + address + ":" + DEFAULT_PORT);

    GUI::log("Waiting for client connection!");

    this->acceptClients();
    this->listenClients();

    return 0;
}

std::string Server::readMessage(Socket *client) {
    std::string message;
    client->socketRead(message, MAX_MSG_SIZE + 100);
    return message;
}

void Server::sendMessage(std::string message, SocketWithInfo *client) {
    client->socket->socketWrite(message);
}

void Server::messageClient(std::string message, SocketWithInfo *client,
                           std::string prefix) {
    while (message.length() > MAX_MSG_SIZE) {
        std::string subMessage = message.substr(0, MAX_MSG_SIZE);
        message = message.substr(MAX_MSG_SIZE, message.length());
        sendMessage(prefix + subMessage, client);
    }
    sendMessage(prefix + message, client);
}

void Server::multicastMessage(std::string message, std::string channel,
                              std::string prefix) {
    if (channels.find(channel) == channels.end()) {
        return;
    }
    Channel *channelObj = channels[channel];
    for (auto client : channelObj->users) {
        messageClient(message, client.second, prefix);
    }
}

int Server::stop() {
    this->shouldBeAccepting = false;
    this->shouldBeListening = false;
    this->shouldBeRunning = false;
    if (this->acceptThread != nullptr) {
        this->acceptThread->join();
    }
    if (this->listenThread != nullptr) {
        this->listenThread->join();
    }
    this->closeClients();
    this->socket->close();
    delete this->meWithInfo;
    return 0;
}

void Server::acceptClients() {
    this->shouldBeAccepting = true;
    this->acceptThread = new std::thread(&Server::_accept, this);
}

void Server::listenClients() {
    this->shouldBeListening = true;
    this->listenThread = new std::thread(&Server::_listen, this);
}

void Server::closeClients() {
    std::lock_guard<std::mutex> lock(this->clientsMutex);

    std::vector<SocketWithInfo *> clientsToClose =
        std::vector<SocketWithInfo *>();

    for (auto client : this->clients) {
        clientsToClose.push_back(client.second);
    }

    for (auto client : clientsToClose) {
        client->socket->close();
    }
}

void Server::closeClient(SocketWithInfo *client) {

    clients.erase(client->nickname);

    if (client->channel != "") {
        auto clientChannel = channels[client->channel];
        clientChannel->users.erase(client->nickname);
    }

    client->socket->socketShutdown(SHUT_RDWR);
    client->socket->close();
}

void Server::_accept() {

    this->socket->listen(10);

    while (this->shouldBeAccepting) {

        std::vector<SocketWithInfo *> reads(1);
        reads[0] = meWithInfo;

        if (Socket::select(&reads, nullptr, nullptr, 1) == 0) {
            continue;
        }

        Socket *client = this->socket->accept();
        SocketWithInfo *clientWithInfo = new SocketWithInfo(client, true);
        clientWithInfo->nickname = this->getNextNickname();
        this->clientsMutex.lock();
        this->clients[clientWithInfo->nickname] = clientWithInfo;
        this->clientsMutex.unlock();
        GUI::log(clientWithInfo->nickname + " connected!");
        GUI::log("Client count: " + std::to_string((int)this->clients.size()));
    }
}

void Server::_listen() {
    while (this->shouldBeListening) {

        std::vector<SocketWithInfo *> reads = std::vector<SocketWithInfo *>();

        this->clientsMutex.lock();
        for (auto client : this->clients) {
            reads.push_back(client.second);
        }
        this->clientsMutex.unlock();

        if (reads.size() == 0) {
            continue;
        }

        if (this->socket->select(&reads, nullptr, nullptr, 1) == 0) {
            continue;
        }
        for (size_t i = 0; i < reads.size(); i++) {
            std::string message = this->readMessage(reads[i]->socket);
            this->handleMessage(reads[i], message);
        }
    }
}

void Server::handleMessage(SocketWithInfo *client, std::string message) {

    if (message == "") {
        this->clients.erase(client->nickname);
        GUI::log(client->nickname + " disconnected!");
        GUI::log("Client count: " + std::to_string((int)this->clients.size()));
        this->closeClient(client);
        return;
    } else if (message[0] == '/') {
        if (message == "/whoami") {
            this->sendMessage("/youare " + client->nickname, client);
        } else if (message == "/ping") {
            this->sendMessage("pong", client);
            GUI::log(client->nickname + " pinged!");
        } else {
            std::regex regex = std::regex("/nickname (.+)");

            std::smatch match;
            std::regex_match(message, match, regex);

            if (match.size() > 1) {

                GUI::log(client->nickname + " asked to change nickname to " +
                         match[1].str());

                std::string newNickname = match[1];
                if (nickNameAvailable(newNickname)) {
                    if (newNickname.size() > 50) {
                        GUI::log("Nickname change failed: Nickname too long!");
                        this->sendMessage("Nickname too long!", client);
                        return;
                    } else {
                        GUI::log(client->nickname + " changed nickname to " +
                                 newNickname);

                        if (client->channel != "") {
                            auto userChannel = this->channels[client->channel];
                            userChannel->users.erase(client->nickname);
                            userChannel->users[newNickname] = client;
                            if (client->isAdmin) {
                                userChannel->admin = newNickname;
                            }
                        }

                        this->clientsMutex.lock();
                        this->clients.erase(client->nickname);
                        client->nickname = newNickname;
                        this->clients[newNickname] = client;
                        this->clientsMutex.unlock();
                        this->sendMessage("/youare " + newNickname, client);
                    }
                } else {
                    GUI::log("Nickname change failed: " + newNickname +
                             " is already in use!");
                    this->sendMessage(
                        "Nickname: " + newNickname + " already taken!", client);
                }
                return;
            }

            regex = std::regex("/join (.+)");

            std::regex_match(message, match, regex);

            if (match.size() > 1) {

                std::string newChannel = match[1];

                std::regex isValidChannelName =
                    std::regex("^([#&][^\\x07\\x2C\\s]+)$");

                if (!std::regex_match(newChannel, isValidChannelName) ||
                    newChannel.size() > 200) {
                    GUI::log("Channel join failed: Invalid channel name "
                             "according with RFC 1459!");
                    this->sendMessage(
                        "Invalid channel name according with RFC 1459!",
                        client);
                    return;
                }

                GUI::log(client->nickname + " asked to join " + newChannel);

                if (client->isAdmin) {
                    GUI::log("Channel join failed: " + client->nickname +
                             " is an admin and can't leave his channel!");
                    this->sendMessage(
                        "You can't leave a channel you administrate!", client);
                    return;
                }

                Channel *channel;

                if (client->channel != "") {
                    channels[client->channel]->users.erase(client->nickname);
                    client->isMuted = false;
                    client->isAdmin = false;
                }

                if (!channelExists(newChannel)) {
                    channel = new Channel();
                    channel->name = newChannel;
                    channel->admin = client->nickname;
                    this->channels[newChannel] = channel;
                    client->isAdmin = true;
                } else {
                    channel = this->channels[newChannel];
                }

                channel->users[client->nickname] = client;
                client->channel = newChannel;

                GUI::log(client->nickname + " joined " + newChannel + " as " +
                         (client->isAdmin ? "admin" : "user"));

                this->sendMessage("/joined " + newChannel + " " +
                                      (client->isAdmin ? "admin" : "user"),
                                  client);
                return;
            }

            regex = std::regex("/mute (.+)");

            std::regex_match(message, match, regex);

            if (match.size() > 1) {

                std::string target = match[1];
                if (target == client->nickname) {
                    GUI::log("Mute failed: Cannot mute yourself!");
                    this->sendMessage("Cannot mute yourself!", client);
                    return;
                }

                if (!client->isAdmin) {
                    GUI::log("Mute failed: You are not an admin!");
                    this->sendMessage(
                        "You must be a channel admin to mute someone!", client);
                    return;
                }

                auto userChannel = channels[client->channel];

                if (userChannel->users.find(target) ==
                    userChannel->users.end()) {
                    GUI::log("Mute failed: " + target +
                             " is not in the channel!");
                    this->sendMessage(target + " is not in the channel!",
                                      client);
                    return;
                }

                auto targetClient = userChannel->users[target];

                if (targetClient->isMuted) {
                    GUI::log("Mute failed: " + target + " is already muted!");
                    this->sendMessage(target + " is already muted!", client);
                    return;
                }

                targetClient->isMuted = true;

                sendMessage("/muted", targetClient);

                GUI::log(client->nickname + " muted " + target);
                this->sendMessage(target + " is now muted!", client);

                return;
            }

            regex = std::regex("/unmute (.+)");

            std::regex_match(message, match, regex);

            if (match.size() > 1) {

                std::string target = match[1];
                if (target == client->nickname) {
                    GUI::log("Unmute failed: Cannot unmute yourself!");
                    this->sendMessage("Cannot unmute yourself!", client);
                    return;
                }

                if (!client->isAdmin) {
                    GUI::log("Unmute failed: You are not an admin!");
                    this->sendMessage(
                        "You must be a channel admin to unmute someone!",
                        client);
                    return;
                }

                auto userChannel = channels[client->channel];

                if (userChannel->users.find(target) ==
                    userChannel->users.end()) {
                    GUI::log("Unmute failed: " + target +
                             " is not in the channel!");
                    this->sendMessage(target + " is not in the channel!",
                                      client);
                    return;
                }

                auto targetClient = userChannel->users[target];

                if (!targetClient->isMuted) {
                    GUI::log("Unmute failed: " + target +
                             " is already unmuted!");
                    this->sendMessage(target + " is already unmuted!", client);
                    return;
                }

                targetClient->isMuted = false;

                sendMessage("/unmuted", targetClient);

                GUI::log(client->nickname + " unmuted " + target);
                this->sendMessage(target + " is now unmuted!", client);

                return;
            }

            regex = std::regex("/whois (.+)");

            std::regex_match(message, match, regex);

            if (match.size() > 1) {

                std::string target = match[1];
                if (target == client->nickname) {
                    GUI::log("Whois failed: Cannot whois yourself!");
                    this->sendMessage("Cannot whois yourself!", client);
                    return;
                }

                if (!client->isAdmin) {
                    GUI::log("Whois failed: You are not an admin!");
                    this->sendMessage(
                        "You must be a channel admin to whois someone!",
                        client);
                    return;
                }

                auto userChannel = channels[client->channel];

                if (userChannel->users.find(target) ==
                    userChannel->users.end()) {
                    GUI::log("Whois failed: " + target +
                             " is not in the channel!");
                    this->sendMessage(target + " is not in the channel!",
                                      client);
                    return;
                }

                auto targetClient = userChannel->users[target];

                std::string ipAddress = targetClient->socket->getIpAddress();

                GUI::log(client->nickname + " whois " + target);

                this->sendMessage(
                    target + " is connected from " + ipAddress + "!", client);

                return;
            }

            regex = std::regex("/kick (.+)");

            std::regex_match(message, match, regex);

            if (match.size() > 1) {

                std::string target = match[1];
                if (target == client->nickname) {
                    GUI::log("Kick failed: Cannot kick yourself!");
                    this->sendMessage("Cannot kick yourself!", client);
                    return;
                }

                if (!client->isAdmin) {
                    GUI::log("Kick failed: You are not an admin!");
                    this->sendMessage(
                        "You must be a channel admin to kick someone!", client);
                    return;
                }

                auto userChannel = channels[client->channel];

                if (userChannel->users.find(target) ==
                    userChannel->users.end()) {
                    GUI::log("Kick failed: " + target +
                             " is not in the channel!");
                    this->sendMessage(target + " is not in the channel!",
                                      client);
                    return;
                }

                auto targetClient = userChannel->users[target];

                sendMessage("/kicked", targetClient);

                userChannel->users.erase(target);
                targetClient->channel = "";
                targetClient->isAdmin = false;
                targetClient->isMuted = false;

                GUI::log(client->nickname + " kicked " + target);
                this->sendMessage(target + " is now kicked!", client);

                return;
            }

            regex = std::regex("/m (.+)");

            std::regex_match(message, match, regex);

            if (match.size() > 1) {

                std::string msg = match[1];

                if (msg.length() > MAX_MSG_SIZE + 100) {
                    GUI::log("Message failed: Message is too long!");
                    this->sendMessage("Message is too long!", client);
                    return;
                }

                if (client->channel == "") {
                    GUI::log("Message failed: You are not in a channel!");
                    this->sendMessage(
                        "You must be in a channel to send messages!", client);
                    return;
                }

                if (client->isMuted) {
                    GUI::log("Message failed: You are muted!");
                    this->sendMessage("You can't send messages while muted!",
                                      client);
                    return;
                }

                GUI::log(client->nickname + "@" + client->channel + " : " +
                         msg);

                multicastMessage(msg, client->channel,
                                 "/msg " + client->nickname + " ");

                return;
            }

            return;
        }
    }
    return;

    GUI::log("Garbage from " + client->nickname + ": " + message);
}

std::string Server::getNextNickname() {
    std::string nickname;
    do {
        nickname = "Client_" + std::to_string(this->nicknameCounter++);
    } while (!this->nickNameAvailable(nickname));

    return nickname;
}
bool Server::nickNameAvailable(std::string nickname) {
    return this->clients.find(nickname) == this->clients.end();
}

bool Server::channelExists(std::string channel) {
    return this->channels.find(channel) != this->channels.end();
}

bool Server::isRunning() { return this->shouldBeRunning; }