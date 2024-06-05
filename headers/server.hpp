#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sstream>

#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <cerrno>

using namespace std;

class Server
{
public:
    Server();
    //~Server();
    void    handleConnections();
    bool    isDataAvailable                (int client_fd, int timeout_sec, int timeout_usec);
    void    handleClientMessages           (int client_fd);
    string  addUser                        (string username);
    void    handleMessageReceivers         (string username, string message);
    void    handleSecificMessageReceivers  (string username, int client_fd, string target);

    int         get_fd();
    int         get_socket();
    int         get_port();
    sockaddr_in get_addr();

private:
    struct Client
    {
        std::string username;
        int client_fd;
    };

    int server_fd;
    int server_socket;
    int opt  = 1;
    int port = 50000;

    sockaddr_in                     server_addr;
    mutex                           fd_mutex;
    mutex                           clients_mutex;
    unordered_map<string, Client>   connectedClients;
};

#endif