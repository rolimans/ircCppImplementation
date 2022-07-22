#include "Socket.hpp"
#include "rlncurses.hpp"
#include "util.hpp"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

Socket::Socket(int domain, int type, int protocol) {
    memset(&addressInfo, 0, sizeof addressInfo);
    socketFD = socket(domain, type, protocol);
    if (socketFD == -1) {
        safeExitFailure("Error opening socket: " + std::string(strerror(errno)),
                        errno);
    }
    addressInfo.ai_family = domain;
    addressInfo.ai_socktype = type;
    addressInfo.ai_protocol = protocol;

    port = "";
    address = "";
}

int Socket::bind(std::string ip, std::string port) {
    this->address = ip;
    this->port = port;
    int status;

    if (addressInfo.ai_family == AF_UNIX) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof addr);
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, ip.c_str(), sizeof(addr.sun_path) - 1);
        status = ::bind(socketFD, (struct sockaddr *)&addr, sizeof addr);
    } else {
        struct addrinfo *ans;
        addressInfo.ai_flags = AI_PASSIVE;
        if ((status = getaddrinfo(ip.c_str(), port.c_str(), &addressInfo,
                                  &ans)) != 0) {
            safeExitFailure("Error getting address info: " +
                                std::string(gai_strerror(status)),
                            status);
        }
        addressInfo.ai_addrlen = ans->ai_addrlen;
        addressInfo.ai_addr = ans->ai_addr;
        freeaddrinfo(ans);
        status = ::bind(socketFD, addressInfo.ai_addr, addressInfo.ai_addrlen);
    }
    if (status == -1) {
        safeExitFailure("Error binding socket: " + std::string(strerror(errno)),
                        errno);
    }
    return status;
}

int Socket::connect(std::string ip, std::string port) {
    this->address = ip;
    this->port = port;
    int status = 0;

    if (addressInfo.ai_family == AF_UNIX) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof addr);
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, ip.c_str(), sizeof(addr.sun_path) - 1);
        status = ::connect(socketFD, (struct sockaddr *)&addr, sizeof(addr));

    } else {

        struct addrinfo *ans;

        if ((status = getaddrinfo(ip.c_str(), port.c_str(), &addressInfo,
                                  &ans)) != 0) {
            if (status == EAI_NONAME) {
                return status;
            }
            safeExitFailure("Error getting address info: " +
                                std::string(gai_strerror(status)),
                            status);
        }
        addressInfo.ai_addrlen = ans->ai_addrlen;
        addressInfo.ai_addr = ans->ai_addr;
        freeaddrinfo(ans);
        status =
            ::connect(socketFD, addressInfo.ai_addr, addressInfo.ai_addrlen);

        if (status == -1) {
            if (errno == EINPROGRESS) {
                SocketWithInfo *tmpWithInfo = new SocketWithInfo(this, true);

                std::vector<SocketWithInfo *> writes(1);
                writes[0] = tmpWithInfo;

                int nWrites = Socket::select(nullptr, &writes, nullptr, 5);

                std::cout << "nWrites: " << nWrites << std::endl;

                if (nWrites < 0 && errno != EINTR) {
                    status = -1;
                } else if (nWrites > 0) {
                    int error;
                    socklen_t len = sizeof(error);

                    if (getsockopt(socketFD, SOL_SOCKET, SO_ERROR, &error,
                                   &len) == -1) {
                        safeExitFailure("Error getting socket option: " +
                                            std::string(strerror(errno)),
                                        errno);
                    }

                    if (error != 0) {
                        if (error == ECONNREFUSED) {
                            status = ECONNREFUSED;
                        } else {
                            status = -1;
                            errno = error;
                        }
                    } else {
                        status = 0;
                    }

                } else {
                    status = ETIMEDOUT;
                }

                delete tmpWithInfo;
            } else if (errno == ECONNREFUSED) {
                status = ECONNREFUSED;
            }
        }
    }

    if (status == -1) {
        safeExitFailure(
            "Error connecting socket: " + std::string(strerror(errno)), errno);
    }
    return status;
}

int Socket::listen(int maxQueue) {
    int status;
    status = ::listen(socketFD, maxQueue);
    if (status == -1) {
        safeExitFailure("Error listening on socket: " +
                            std::string(strerror(errno)),
                        errno);
    }
    return status;
}

Socket *Socket::accept() {
    struct sockaddr_storage otherAddr;
    socklen_t otherAddrLen = sizeof otherAddr;
    int newSocketFD =
        ::accept(socketFD, (struct sockaddr *)&otherAddr, &otherAddrLen);
    if (newSocketFD == -1) {
        safeExitFailure(
            "Error accepting socket: " + std::string(strerror(errno)), errno);
    }
    Socket *newSocket =
        new Socket(addressInfo.ai_family, addressInfo.ai_socktype,
                   addressInfo.ai_protocol);
    newSocket->socketFD = newSocketFD;
    newSocket->port = port;

    char host[NI_MAXHOST];
    int status = getnameinfo((struct sockaddr *)&otherAddr, otherAddrLen, host,
                             sizeof host, NULL, 0, NI_NUMERICHOST);
    if (status == -1) {
        safeExitFailure(
            "Error getting hostname: " + std::string(strerror(errno)), errno);
    }
    newSocket->address = host;
    newSocket->addressInfo.ai_family = otherAddr.ss_family;
    newSocket->addressInfo.ai_addr = (struct sockaddr *)&otherAddr;
    return newSocket;
}
int Socket::socketWrite(std::string message) {

    int error = 0;
    socklen_t len = sizeof(error);
    int ret = getsockopt(socketFD, SOL_SOCKET, SO_ERROR, &error, &len);

    if (ret != 0 || error != 0) {
        return -2;
    }

    int status = (int)send(socketFD, message.c_str(), message.size(), 0);

    if (status == -1) {
        safeExitFailure(
            "Error writing to socket: " + std::string(strerror(errno)), errno);
    }
    return status;
}

int Socket::socketRead(std::string &buffer, int length) {
    char *buff = new char[length];
    memset(buff, 0, length);
    int status = (int)recv(socketFD, buff, length - 1, 0);
    if (status == -1) {
        safeExitFailure("Error reading from socket: " +
                            std::string(strerror(errno)),
                        errno);
    }

    buffer = std::string(buff);
    delete[] buff;
    return status;
}

int Socket::socketSafeRead(std::string &buffer, int length, int timeout) {
    std::vector<SocketWithInfo *> reads;
    auto meWithInfo = new SocketWithInfo(this, false);
    reads.push_back(meWithInfo);
    int count = Socket::select(&reads, nullptr, nullptr, timeout);

    delete meWithInfo;

    if (count < 1) {

        buffer = "";
        return -1;
    }

    char *buff = new char[length];
    memset(buff, 0, length);
    int status = (int)recv(socketFD, buff, length - 1, 0);
    if (status < 0) {
        safeExitFailure("Error reading from socket: " +
                            std::string(strerror(errno)),
                        errno);
    }
    buffer = std::string(buff);
    delete[] buff;
    return status;
}

int Socket::socketSetOpt(int level, int optName, void *optVal) {

    int status = ::setsockopt(socketFD, level, optName, optVal, sizeof optVal);
    if (status == -1) {
        safeExitFailure("Error setting socket option: " +
                            std::string(strerror(errno)),
                        errno);
    }
    return status;
}

int Socket::socketGetOpt(int level, int optName, void *optVal) {
    socklen_t lentgh = sizeof optVal;
    int status = ::getsockopt(socketFD, level, optName, optVal, &lentgh);
    if (status == -1) {
        safeExitFailure("Error getting socket option: " +
                            std::string(strerror(errno)),
                        errno);
    }
    return status;
}

void Socket::close() { ::close(socketFD); }

int Socket::setBlocking(bool blocking) {
    long status = fcntl(socketFD, F_GETFL, NULL);

    if (status < 0) {
        safeExitFailure("Error getting socket flags to set to nonblocking: " +
                            std::string(strerror(errno)),
                        errno);
    }

    if (blocking) {
        status &= ~O_NONBLOCK;
    } else {
        status |= O_NONBLOCK;
    }

    status = fcntl(socketFD, F_SETFL, status);

    if (status < 0) {
        safeExitFailure("Error setting socket flags to nonblocking: " +
                            std::string(strerror(errno)),
                        errno);
    }
    return (int)status;
}

int Socket::socketShutdown(int how) {
    int status = ::shutdown(socketFD, how);

    if (status < 0) {
        safeExitFailure("Error shutting down socket: " +
                            std::string(strerror(errno)),
                        errno);
    }
    return status;
}

int Socket::select(std::vector<SocketWithInfo *> *reads,
                   std::vector<SocketWithInfo *> *writes,
                   std::vector<SocketWithInfo *> *excepts, int timeout) {
    // int id = reads->at(0)->socketFD;

    struct timeval timeValue;
    fd_set readFDs;
    fd_set writeFDs;
    fd_set exceptFDs;

    timeValue.tv_sec = timeout;
    timeValue.tv_usec = 0;

    FD_ZERO(&readFDs);

    FD_ZERO(&writeFDs);

    FD_ZERO(&exceptFDs);

    int maxSock = 0;

    if (reads != nullptr) {
        for (size_t i = 0; i < reads->size(); i++) {
            // ERROR IS HERE
            int sockInt = reads->at(i)->socket->socketFD;

            if (sockInt > maxSock) {
                maxSock = sockInt;
            }
            FD_SET(sockInt, &readFDs);
        }
    }

    if (writes != nullptr) {
        for (size_t i = 0; i < writes->size(); i++) {
            int sockInt = writes->at(i)->socket->socketFD;
            if (sockInt > maxSock) {
                maxSock = sockInt;
            }
            FD_SET(sockInt, &writeFDs);
        }
    }

    if (excepts != nullptr) {
        for (size_t i = 0; i < excepts->size(); i++) {
            int sockInt = excepts->at(i)->socket->socketFD;
            if (sockInt > maxSock) {
                maxSock = sockInt;
            }
            FD_SET(sockInt, &exceptFDs);
        }
    }

    int result =
        ::select(maxSock + 1, &readFDs, &writeFDs, &exceptFDs, &timeValue);

    if (result < 0) {
        safeExitFailure("Error in select: " + std::string(strerror(errno)),
                        errno);
    }
    if (reads != nullptr) {
        for (int i = (int)reads->size() - 1; i >= 0; i--) {
            if (!FD_ISSET(reads->at(i)->socket->socketFD, &readFDs)) {
                reads->erase(reads->begin() + i);
            }
        }
    }
    if (writes != nullptr) {
        for (int i = (int)writes->size() - 1; i >= 0; i--) {
            if (!FD_ISSET(writes->at(i)->socket->socketFD, &writeFDs)) {
                writes->erase(writes->begin() + i);
            }
        }
    }
    if (excepts != nullptr) {
        for (int i = (int)excepts->size() - 1; i >= 0; i--) {
            if (!FD_ISSET(excepts->at(i)->socket->socketFD, &exceptFDs)) {
                excepts->erase(excepts->begin() + i);
            }
        }
    }
    return result;
}

std::string Socket::getIpAddress() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getsockname(socketFD, (struct sockaddr *)&addr, &len) < 0) {
        safeExitFailure("Error getting socket address: " +
                            std::string(strerror(errno)),
                        errno);
    }
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    return std::string(ip);
}

SocketWithInfo::SocketWithInfo(Socket *socket, bool isClient) {
    this->socket = socket;
    this->isClient = isClient;
}