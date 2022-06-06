#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#define DEFAULT_PORT "6697"
#define MAX_MSG_SIZE 4096

#include "Socket.hpp"
#include <string>
#include <vector>

class Server {
  private:
    Socket *socket;
    std::string address;
    std::vector<Socket *> clients;

  public:
    Server(std::string address);
    int start();
    int stop();
    std::string readMessage();
    void sendMessage(std::string message);
};

#endif