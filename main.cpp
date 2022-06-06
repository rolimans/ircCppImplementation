#include "Client.hpp"
#include "Server.hpp"
#include "util.hpp"
#include <iostream>

using namespace std;
int main() {

    cout << "Do you want to run a server or a client? (s/c)" << endl;
    char answer;
    cin >> answer;
    if (answer == 's') {
        Server *server = new Server("localhost");
        server->start();
        while (true) {
            string message = server->readMessage();
            cout << "Message received: " << message << endl;
            if (message == "exit") {
                server->stop();
                break;
            }
            server->sendMessage("ECHO - " + message);
            cout << "Message echoed" << endl;
        }
        server->stop();
    } else if (answer == 'c') {
        Client *client = new Client("localhost");
        client->start();
        while (true) {
            string message;
            cout << "Enter message: ";
            message = readLineCpp(stdin);
            client->sendMessage(message);
            cout << "Message sent!" << endl;
            if (message == "exit") {
                break;
            }
            cout << "Message received: " << client->readMessage() << endl;
        }
        client->stop();
    }

    return 0;
}