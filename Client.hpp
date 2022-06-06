#ifndef _CLIENT_HPP_
#define _CLIENT_HPP_

#define DEFAULT_PORT "6697"
#define MAX_MSG_SIZE 4096

#include "Socket.hpp"
#include <string>

class Client {
  private:
    Socket *socket;
    std::string address;

  public:
    Client(std::string address);
    int start();
    int stop();
    std::string readMessage();
    void sendMessage(std::string message);
};

#endif