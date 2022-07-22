#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <netdb.h>
#include <string>
#include <vector>

class Socket;

struct SocketWithInfo {
    std::string nickname;
    Socket *socket;
    bool isClient;
    bool isAdmin = false;
    bool isMuted = false;
    std::string channel = "";
    SocketWithInfo(Socket *socket, bool isClient);
};

class Socket {
  private:
    std::string address;
    std::string port;
    struct addrinfo addressInfo;

  public:
    int socketFD;

    Socket(int domain, int type, int protocol);
    int bind(std::string ip, std::string port);
    int connect(std::string ip, std::string port);
    int listen(int maxQueue);
    Socket *accept();
    int socketWrite(std::string msg);
    int socketRead(std::string &buffer, int length);
    int socketSafeRead(std::string &buffer, int length, int timeout);
    int socketSetOpt(int level, int optName, void *optVal);
    int socketGetOpt(int level, int optName, void *optVal);
    int setBlocking(bool blocking);
    int socketShutdown(int how);
    void close();
    static int select(std::vector<SocketWithInfo *> *reads,
                      std::vector<SocketWithInfo *> *writes,
                      std::vector<SocketWithInfo *> *excepts, int timeout);
    std::string getIpAddress();
};

#endif