#include "Client.hpp"
#include "rlncurses.hpp"
#include "util.hpp"
#include <iostream>
#include <readline/history.h>

using namespace std;
int main() {

    Client *client = new Client();
    GUI *gui = GUI::GetInstance("IRC Client> ");

    gui->init();

    add_history("/connect localhost");

    gui->addCommand("/connect", [client, gui](const GUI::argsT &args) {
        if (args.size() != 2) {
            gui->addToWindow("Usage: /connect <address>");
            return 1;
        }
        client->start(args[1]);
        return 0;
    });

    gui->addCommand("/ping", [client, gui](const GUI::argsT &args) {
        if (client->isConnected(true)) {
            client->sendMessage(args[0]);
        }
        return 0;
    });

    gui->addCommand("/nickname", [client, gui](const GUI::argsT &args) {
        if (args.size() != 2) {
            gui->addToWindow("Usage: /nickname <nickname>");
            return 1;
        }

        if (client->isConnected(true)) {
            client->sendMessage("/nickname " + args[1]);
        }

        return 0;
    });

    gui->addCommand("/join", [client, gui](const GUI::argsT &args) {
        if (args.size() != 2) {
            gui->addToWindow("Usage: /join <channel>");
            return 1;
        }

        if (client->isConnected(true)) {
            client->sendMessage("/join " + args[1]);
        }

        return 0;
    });

    gui->addCommand("/mute", [client, gui](const GUI::argsT &args) {
        if (args.size() != 2) {
            gui->addToWindow("Usage: /mute <nickname>");
            return 1;
        }

        if (client->isConnected(true) && client->hasChannel(true)) {
            client->sendMessage("/mute " + args[1]);
        }

        return 0;
    });

    gui->addCommand("/unmute", [client, gui](const GUI::argsT &args) {
        if (args.size() != 2) {
            gui->addToWindow("Usage: /unmute <nickname>");
            return 1;
        }

        if (client->isConnected(true) && client->hasChannel(true)) {
            client->sendMessage("/unmute " + args[1]);
        }

        return 0;
    });

    gui->addCommand("/whois", [client, gui](const GUI::argsT &args) {
        if (args.size() != 2) {
            gui->addToWindow("Usage: /whois <nickname>");
            return 1;
        }

        if (client->isConnected(true) && client->hasChannel(true)) {
            client->sendMessage("/whois " + args[1]);
        }

        return 0;
    });

    gui->addCommand("/kick", [client, gui](const GUI::argsT &args) {
        if (args.size() != 2) {
            gui->addToWindow("Usage: /kick <nickname>");
            return 1;
        }

        if (client->isConnected(true) && client->hasChannel(true)) {
            client->sendMessage("/kick " + args[1]);
        }

        return 0;
    });

    gui->enableMessaging([client, gui](std::string message) {
        if (!client->isConnected(false)) {
            GUI::log("You must be connected to send messages!");
            return 0;
        }
        if (!client->hasChannel(false)) {
            GUI::log("You must be in a channel to send messages!");
            return 0;
        }

        if (client->isMuted()) {
            GUI::log("You are muted in this channel!");
            return 0;
        }

        client->messageServer(message);

        return 0;
    });

    gui->run();

    client->stop();

    gui->close();

    return 0;
}