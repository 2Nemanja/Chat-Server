#include "../headers/client.hpp"

Client::Client()
{
    try {

        if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            throw runtime_error(string("socket error: \n") + strerror(errno));
        }

        if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            throw runtime_error(string("setsockopt error: \n") + strerror(errno));
        }

        client_addr.sin_family=            AF_INET;
        client_addr.sin_port  =            htons(this->port);

        if (inet_pton(AF_INET, "127.0.0.1", &client_addr.sin_addr) <= 0) {
            throw runtime_error(string("invalid address: \n") + strerror(errno));
        }

        if ((connect(client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr))) < 0) {
            throw runtime_error(string("connection error: \n") + strerror(errno));
        }
        cout <<                             "Enter username >> ";
        string                              username;
        cin >>                              username;
        uint32_t username_size =            htonl(username.size());
        send(client_fd, &username_size, sizeof(uint32_t), 0);
        send(client_fd, username.c_str(), username.size(), 0);

        std::thread communicationThread(&Client::communicate, this, client_fd);
        communicationThread.detach();

        std::thread receiverThread(&Client::Receiver, this, client_fd);
        receiverThread.detach();

    }
    catch (const exception &e) {
        cerr << e.what() << endl;
        cout << "client greska";
    }
}

void Client::Receiver(int client_fd)
{
    try {
        while (true) {
            char buffer[1024];
            int  bytesReceived = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if  (bytesReceived <= 0)
            {
                if (bytesReceived == 0) {
                    cout << "Server disconnected" << endl;
                    close(client_fd);
                    exit(EXIT_FAILURE);
                }
                else {
                    cerr << "Receive error: " << strerror(errno) << endl;
                }
                close(client_fd);
                break;
            }
            buffer[bytesReceived] = '\0';
            cout << buffer << endl;
        }
    }
    catch (const exception e) {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}
void Client::communicate(int client_fd)
{
    try {
        while (true) {
            char message[1024];
            std::string private_trigger = "/specify";
            std::cin.getline(message, sizeof(message));

            if (std::string(message) == private_trigger) {
                if (send(client_fd, message, strlen(message), 0) == -1) {
                    std::cerr << "Failed to send private trigger: " << strerror(errno) << std::endl;
                    break;
                }

                char target_username[256];
                char private_message[1024];

                std::cout << "Who do you want to write to? ";
                std::cin.getline(target_username, sizeof(target_username));

                std::cout << "Say something to " << target_username << ": ";
                std::cin.getline(private_message, sizeof(private_message));

                std::string combo_private_message = std::string(target_username) + " : " + std::string(private_message);
                std::cout << "Private message: " << combo_private_message << std::endl;

                if (send(client_fd, combo_private_message.c_str(), combo_private_message.length(), 0) == -1) {
                    std::cerr << "Failed to send private message: " << strerror(errno) << std::endl;
                    break;
                }
            } else {
                if (send(client_fd, message, strlen(message), 0) == -1) {
                    std::cerr << "Failed to send message: " << strerror(errno) << std::endl;
                    break;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
