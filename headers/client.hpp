#ifndef CLIENT_HPP
#define CLIENT_HPP

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
#include <cstring>
#include <cstdlib>

using namespace std;

class Client
{
private:
    int client_socket;
    int port =              50000;
    int opt  =              1;
    sockaddr_in             client_addr;

public:
    Client();
    int                     client_fd;
    string                  username;
    mutex                   client_mutex;
    mutex                   fd_mutex;

    void connectToServer();
    void communicate    (int file_descriptor);
    bool isDataAvailable(int client_fd, int timeout_sec, int timeout_usec);
    void Receiver       (int file_descriptor);
};

#endif