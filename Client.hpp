#ifndef _CLIENT_HPP_
#define _CLIENT_HPP_

#define DEFAULT_PORT "6697"
#define MAX_MSG_SIZE 4096

#include "Socket.hpp"
#include <mutex>
#include <string>
#include <thread>

class Client {
  private:
    Socket *socket;
    std::string address;
    SocketWithInfo *meWithInfo;
    bool _isConnected = false;
    std::mutex isConnectedMutex;
    bool shouldBeListening = false;
    void _listen();
    std::thread *listenThread;
    void init();

  public:
    Client(std::string address);
    Client();
    int start(std::string address);
    int start();
    void startListening();
    int stop();
    bool isConnected(bool);
    std::string readMessage();
    int safeReadMessage(std::string &message);
    void sendMessage(std::string message);
    void messageServer(std::string message);
    bool hasChannel(bool);
    bool isMuted();
};

#endif