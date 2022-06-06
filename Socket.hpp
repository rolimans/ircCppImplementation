#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <netdb.h>
#include <string>

class Socket {
  private:
    int socketFD;
    std::string address;
    std::string port;
    struct addrinfo addressInfo;

  public:
    Socket(int domain, int type, int protocol);
    int bind(std::string ip, std::string port);
    int connect(std::string ip, std::string port);
    int listen(int maxQueue);
    Socket *accept();
    int socketWrite(std::string msg);
    int socketRead(std::string &buffer, int length);
    int socketSetOpt(int level, int optName, void *optVal);
    int socketGetOpt(int level, int optName, void *optVal);
    void close();
};
#endif