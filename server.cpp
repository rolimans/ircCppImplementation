#include "Server.hpp"
#include "rlncurses.hpp"
#include "util.hpp"
#include <iostream>
#include <thread>

using namespace std;
int main() {

    Server *server = new Server("*");
    GUI *gui = GUI::GetInstance("IRC Server> ");

    gui->init();

    server->start();

    std::thread *serverThread = new std::thread([server, gui]() {
        while (server->isRunning())
            ;
    });

    gui->run();

    server->shouldBeRunning = false;

    serverThread->join();

    server->stop();

    gui->close();

    return 0;
}