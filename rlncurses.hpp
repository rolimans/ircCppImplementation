// Readline + ncurses implementation from
// https://github.com/ulfalizer/readline-and-ncurses Adapted by Eduardo Amaral

#ifndef _RLNCURSES_HPP_
#define _RLNCURSES_HPP_

#include "Socket.hpp"
#include "util.hpp"
#include <curses.h>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Checks errors for (most) ncurses functions. CHECK_NCURSES(fn, x, y, z) is a
// checked version of fn(x, y, z).
#define CHECK_NCURSES(fn, ...)                                                 \
    if (fn(__VA_ARGS__) == ERR)                                                \
        this->exitFailing(#fn "(" #__VA_ARGS__ ") failed!", EXIT_FAILURE);

#define CHECK_NCURSES_VOID(fn)                                                 \
    if (fn() == ERR)                                                           \
        this->exitFailing(#fn " failed!", EXIT_FAILURE);

class GUI {
  public:
    using argsT = std::vector<std::string>;
    using commandFnT = std::function<int(const argsT &)>;
    using messageFnT = std::function<int(std::string)>;
    void addCommand(std::string, commandFnT);
    std::vector<std::string> getCommands();
    int executeCommand(std::string);
    void exitFailing(std::string, int);
    GUI(GUI &other) = delete;
    void operator=(const GUI &) = delete;
    static GUI *GetInstance(std::string);
    static void addToWindow(std::string);
    static void log(std::string);
    void init();
    void enableMessaging(messageFnT);
    void close();
    void run();
    void prepareClose(std::string message);
    static void updatePrompt(SocketWithInfo *);

  private:
    static GUI *singleton;
    static std::mutex singletonMutex;

    GUI(std::string);

    ~GUI() {}

    // Message window
    WINDOW *contentWindow;
    // Command (readline) window
    WINDOW *commandWindow;
    // Suggestion window
    WINDOW *suggestionWindow;

    // String displayed in the message window
    std::string contentString;
    // Input character for readline
    unsigned char input;
    // Used to signal "no more input" after feeding a character to readline
    bool isInputAvailable = false;
    // Keeps track of the terminal mode so we can reset the terminal if needed
    // on errors
    bool isInGUI = false;
    bool shouldClose = false;
    bool isRunning = false;
    std::string commandString;
    // GNU readline function types
    using commandCompletionFunction = char **(const char *, int, int);
    using commandIteratorFunction = char *(const char *, int);
    // GNU Readline completion functions
    static commandCompletionFunction commandCompletion;
    static commandIteratorFunction commandIterator;
    // GNU Command Interfaces
    using commandMap = std::unordered_map<std::string, GUI::commandFnT>;
    commandMap commands;

    GUI::messageFnT messageFn = nullptr;
    size_t strnwidth(const char *, size_t, size_t);
    size_t strwidth(const char *, size_t);
    static int readlineInputAvailable();
    static int readlineGetc(FILE *);
    void redisplayMessage(bool);
    void setSuggestions(std::string);
    static void handleCommand(char *);
    void windowRedisplay(bool);
    static void readlineRedisplay();
    void resize();
    void initNCurses();
    void initReadline();
    void initSignalHandler();
    void closeReadline();
    void closeNCurses();
    void closeSignalHandler();
    void forwardToReadline(char);
    char readFromGUI();
};

#endif