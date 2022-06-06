#include "Socket.hpp"
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

Socket::Socket(int domain, int type, int protocol) {
    memset(&addressInfo, 0, sizeof addressInfo);
    socketFD = socket(domain, type, protocol);
    if (socketFD == -1) {
        std::cerr << "Error opening socket: " << strerror(errno) << std::endl;
        exit(errno);
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
            std::cerr << "Error getting address info: " << gai_strerror(status)
                      << std::endl;
            exit(status);
        }
        addressInfo.ai_addrlen = ans->ai_addrlen;
        addressInfo.ai_addr = ans->ai_addr;
        freeaddrinfo(ans);
        status = ::bind(socketFD, addressInfo.ai_addr, addressInfo.ai_addrlen);
    }
    if (status == -1) {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        exit(errno);
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
        int status =
            ::connect(socketFD, (struct sockaddr *)&addr, sizeof(addr));
        if (status == -1) {
            std::cerr << "Error connecting socket: " << strerror(errno)
                      << std::endl;
        }

    } else {
        struct addrinfo *ans;
        int status;
        if ((status = getaddrinfo(ip.c_str(), port.c_str(), &addressInfo,
                                  &ans)) != 0) {
            std::cerr << "Error getting address info: " << gai_strerror(status)
                      << std::endl;
            exit(status);
        }
        addressInfo.ai_addrlen = ans->ai_addrlen;
        addressInfo.ai_addr = ans->ai_addr;
        freeaddrinfo(ans);
        status =
            ::connect(socketFD, addressInfo.ai_addr, addressInfo.ai_addrlen);
    }

    if (status == -1) {
        std::cerr << "Error connecting socket: " << strerror(errno)
                  << std::endl;
        exit(errno);
    }
    return status;
}

int Socket::listen(int maxQueue) {
    int status;
    status = ::listen(socketFD, maxQueue);
    if (status == -1) {
        std::cerr << "Error listening socket: " << strerror(errno) << std::endl;
        exit(errno);
    }
    return status;
}

Socket *Socket::accept() {
    struct sockaddr_storage otherAddr;
    socklen_t otherAddrLen = sizeof otherAddr;
    int newSocketFD =
        ::accept(socketFD, (struct sockaddr *)&otherAddr, &otherAddrLen);
    if (newSocketFD == -1) {
        std::cerr << "Error accepting socket: " << strerror(errno) << std::endl;
        exit(errno);
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
        std::cerr << "Error getting hostname: " << strerror(errno) << std::endl;
        exit(errno);
    }
    newSocket->address = host;
    newSocket->addressInfo.ai_family = otherAddr.ss_family;
    newSocket->addressInfo.ai_addr = (struct sockaddr *)&otherAddr;
    return newSocket;
}
int Socket::socketWrite(std::string message) {
    int status = (int)send(socketFD, message.c_str(), message.size(), 0);
    if (status == -1) {
        std::cerr << "Error writing to socket: " << strerror(errno)
                  << std::endl;
        exit(errno);
    }
    return status;
}

int Socket::socketRead(std::string &buffer, int length) {
    char *buff = new char[length];
    memset(buff, 0, length);
    int status = (int)recv(socketFD, buff, length - 1, 0);
    if (status == -1) {
        std::cerr << "Error reading from socket: " << strerror(errno)
                  << std::endl;
        exit(errno);
    }
    buffer = std::string(buff);
    delete[] buff;
    return status;
}

int Socket::socketSetOpt(int level, int optName, void *optVal) {

    int status = ::setsockopt(socketFD, level, optName, optVal, sizeof optVal);
    if (status == -1) {
        std::cerr << "Error setting socket option: " << strerror(errno)
                  << std::endl;
        exit(errno);
    }
    return status;
}

int Socket::socketGetOpt(int level, int optName, void *optVal) {
    socklen_t lentgh = sizeof optVal;
    int status = ::getsockopt(socketFD, level, optName, optVal, &lentgh);
    if (status == -1) {
        std::cerr << "Error getting socket option: " << strerror(errno)
                  << std::endl;
        exit(errno);
    }
    return status;
}

void Socket::close() { ::close(socketFD); }
