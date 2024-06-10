#include "../headers/server.hpp"

            // ------------------------------------------------------------------------
            // NAPRAVITI OVAKAV OBLIK SLANJA PRIV PORUKE :  /specify:nemanja - hahahaha
            // ------------------------------------------------------------------------

Server::Server() { 
    try {
        server_addr.sin_family      =               AF_INET;
        server_addr.sin_addr.s_addr =               INADDR_ANY;
        server_addr.sin_port        =               htons(this->port);

        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            throw runtime_error(string("socket error: \n")      + strerror(errno));
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            throw runtime_error(string("setsockopt error: \n")  + strerror(errno));
        }

        if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            throw runtime_error(string("binding error: \n")     + strerror(errno));
        }

        if (listen(server_fd, 256) < 0) {
            throw runtime_error(string("listen error: \n")      + strerror(errno));
        }

        std::cout << "Server up and running" << endl;
        std::thread connectionThread(&Server::handleConnections, this);
        connectionThread.detach();
    }
    catch (const exception &e) {
        cerr << e.what() << endl;
        exit (EXIT_FAILURE);
    }
}

void Server::handleConnections() {
    cout << "connection handle active" << endl;
    try {
        int i = 0;
        while (true) {
            i++;
            struct    sockaddr_in           client_address;
            socklen_t addrlen =             sizeof(client_address);
            std::     string                username;

            int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);
            if (client_fd < 0) {
                cout << "Failed to accept client connection: " << strerror(errno) << endl;
                continue;
            }

            char ip_str[INET_ADDRSTRLEN];
            inet_ntop  (AF_INET, &(client_address.sin_addr), ip_str, INET_ADDRSTRLEN);
            cout <<   " Client connected from " << ip_str << ":"  << ntohs(client_address.sin_port) << endl;

            uint32_t                        username_size;
            int recv_size =                 recv(client_fd, &username_size, sizeof(uint32_t), 0);
            if (recv_size <= 0) {
                cerr << "Failed to receive username size: " << strerror(errno) << endl;
                close(client_fd);
                continue;
            }

            username_size =                 ntohl(username_size);
            char *buffer =                  new char[username_size + 1];
            buffer[username_size] =         '\0';
            int bytesReceived =             recv(client_fd, buffer, username_size, 0);
            if (bytesReceived <= 0)
            {
                cerr << "Failed to receive username: " << strerror(errno) << endl;
                delete[] buffer;
                close (  client_fd);
                continue;
            }

            username.append                (buffer, bytesReceived);
            username             =          username + to_string(i);
            Client                          new_client;
            new_client.username  =          username;
            new_client.client_fd =          client_fd;

            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                this->connectedClients.insert   (std::make_pair(new_client.username, new_client));
            }

            cout << "Client " << username << " added with fd " << client_fd << endl;
            int  client_port = ntohs (client_address.sin_port);
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop     (AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);

            std::thread threadCliMess(&Server::handleClientMessages, this, client_fd);
            threadCliMess.detach();
            delete[] buffer;
        }
    }
    catch (const exception &e) {
        cerr << "handle connection errors: " << e.what() << endl;
        return;
    }
}

void Server::removeClient(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it =    connectedClients.begin();
    while ( it != connectedClients.end()) {
        if (it-> second.client_fd == client_fd) {
            it = connectedClients.erase(it);
        }
        else {
            it++;
        }
    }
}  

void Server::listAllUsers(int client_fd, string sender_username) {
    string user;
    for(const auto &cli : connectedClients) {
        user = cli.first + "\n";
        if(send(client_fd, user.c_str(), user.size(), 0) == -1) {
            cerr << "Failed to send all users to the client...\n";
        } else {
            continue;
        }
   }
}

void Server::handleClientMessages(int client_fd) {
     char buffer[1024];
     try {
        while (true) {
            string private_trigger      =       "/specify";
            string list_users_trigger   =       "/ls";
            int  bytesReceived =                recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if ( bytesReceived <= 0 )
            {
                if (bytesReceived == 0) {
                    std::cout << "Client disconnected" << std::endl;
                } else {
                    std::cerr << "Receive error: " << strerror(errno) << std::endl;
                }
                close(client_fd);
                removeClient(client_fd);
                break;
            }

            buffer[bytesReceived] =     '\0';
            std::string                 received_message(buffer);
            std::string                 sender_username;

            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (const auto &client : connectedClients) {
                    if (client.second.client_fd == client_fd) {
                        sender_username = client.first;
                        break;
                    }
                }
            }

            if (sender_username.empty()) {
                std::cerr << "Client not found in connectedClients" << std::endl;
                continue;
            }

            auto trim = [](std::string str) {
                str.erase(str.find_last_not_of(" \n\r\t") + 1);
                return str;
            };
            received_message = trim(received_message);

            std::cout << "Received message: '"  <<        received_message   << "'" <<                         std::endl;
            std::cout << "Private trigger: '"   <<        private_trigger    << "'" <<                         std::endl;
            std::cout << "list_users_trigger: '"<<        list_users_trigger << "'" <<                         std::endl;
            std::cout << "Comparison result: "  << strcmp(received_message.c_str(), private_trigger.c_str())<< std::endl;

            if (strcmp(received_message.c_str(), list_users_trigger.c_str()) == 0) {
                    cout << "Client wants list of all active users!\n";
                    string userList_message = "\n*** Here is the list of all the active users ***\n";
                    if (send(client_fd, userList_message.c_str(), userList_message.size(), 0) == -1) {
                        cerr << "Failed to notify client about incoming usernames...\n" << strerror(errno);
                    } else {
                        std::lock_guard<std::mutex> lock(clients_mutex);
                        std::thread usersListThread(&Server::listAllUsers, this, client_fd,sender_username);
                        usersListThread.detach();
                        continue;
                    }
                } 

            if (strcmp(received_message.c_str(), private_trigger.c_str()) == 0) {
                std::cout << "Client " << sender_username << " wants a private chat!" << std::endl;

                char private_message_buffer[1024];
                int  privatebytesReceived = recv(client_fd, private_message_buffer, sizeof(buffer) - 1, 0);
                if ( privatebytesReceived <= 0 )
                {
                    if (privatebytesReceived == 0) {
                        std::cout << "Client disconnected" << std::endl;
                    }
                    else {
                        std::cerr << "Receive error: " << strerror(errno) << std::endl;
                    }
                    close(client_fd);
                    removeClient(client_fd);
                    break;
                } else {
                    private_message_buffer[privatebytesReceived] = '\0';
                    std::thread SpecificSenderThread(&Server::handleSecificMessageReceivers, this, sender_username, client_fd, private_message_buffer);
                    SpecificSenderThread.detach();
                    continue;;
                }
            }

            std::cout <<"Client " << sender_username << " has sent: " << buffer << std::endl;
            std::thread  SenderThread(&Server::handleMessageReceivers, this, sender_username, buffer);
            SenderThread.detach();
        }
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        std::cout << "Server message handler exception" << std::endl;
    }
}

void Server::handleMessageReceivers(string username, string message) {
    try {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto &cli : connectedClients) {
            if (cli.first != username) {
                string  combo_message =     username + " : " + message;
                ssize_t bytes_sent    =     send(cli.second.client_fd, combo_message.c_str(), combo_message.size(), 0);
                if (    bytes_sent    == -1) {
                        cerr << "Failed to send message to client " << cli.first << ": " << strerror(errno) << endl;
                }
                else {
                        cout <<  combo_message << " sent to client " << cli.first << " (fd: " << cli.second.client_fd << ")\n";
                }
            }
        }
    }
    catch (const exception e) {
        cerr << e.what() << endl;
    }
}

bool Server::findUser(string target_receiver) {
    std::lock_guard<std::mutex> lock(clients_mutex); 
    return connectedClients.find(target_receiver) != connectedClients.end();
}

void Server::handleSecificMessageReceivers(string username, int client_fd, string target) {
    try {
        std::cout << "Received message: '"  << username  << "'" << std::endl;
        std::cout << "Private trigger: '"   << client_fd << "'" << std::endl;
        std::cout << "Private message buffer: '"   << target    << "'" << std::endl;
        
        string target_receiver;
        string delimiter = " : ";
        size_t pos       = target.find(delimiter);
        target_receiver  = target.substr(0, pos);
        string message   = target.substr(pos + delimiter.length());
            if(findUser(target_receiver)) {
                std::lock_guard<std::mutex>   lock(clients_mutex);
                for(const auto &cli : connectedClients) {
                    if (cli.first != username && cli.first == target_receiver) {
                        string  combo_message =          username + " has privately sent u : " + message;
                        ssize_t bytes_sent    =          send(cli.second.client_fd, combo_message.c_str(), combo_message.size(), 0);
                        if (    bytes_sent    == -1) {
                                cerr << "Failed to send private message to " << cli.first << ": " << strerror(errno) << endl;
                        } else {
                                cout << "Private message sent to client " << cli.first << " (fd: " << cli.second.client_fd << ")\n";
                        }
                    }
                }
                string acknowledgement = "Private message sent to " + target_receiver;
                send(client_fd, acknowledgement.c_str(), acknowledgement.size(), 0);
            } else {
                string error = "There is no user " + target_receiver;
                cout << error << endl;
                send(client_fd, error.c_str(), error.size(), 0);
            }
        }
    catch (const exception e) {
        cerr << e.what() << endl;
        cout << " server receiver handler";
    }
}
