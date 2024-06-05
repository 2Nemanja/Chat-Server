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
            cout << connectedClients.size() << endl;

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
void   Server::handleClientMessages(int client_fd) {
    try {
        while (true) {
            char buffer[1024];
            std::string private_trigger =       "/specify";
            int  bytesReceived =                recv(client_fd, buffer, sizeof(buffer) - 1, 0);

            if ( bytesReceived <= 0 )
            {
                if (bytesReceived == 0) {
                    std::cout << "Client disconnected" << std::endl;
                }
                else {
                    std::cerr << "Receive error: " << strerror(errno) << std::endl;
                }

                close(client_fd);

                {
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

            std::cout << "Received message: '"  <<        received_message << "'" <<                            std::endl;
            std::cout << "Private trigger: '"   <<        private_trigger  << "'" <<                            std::endl;
            std::cout << "Comparison result: "  << strcmp(received_message.c_str(), private_trigger.c_str()) << std::endl;

            if (strcmp(received_message.c_str(), private_trigger.c_str()) == 0) {
                std::cout << "Client " << sender_username << " wants a private chat!" << std::endl;
                std::thread SpecificSenderThread(&Server::handleSecificMessageReceivers, this, sender_username, client_fd, buffer);
                SpecificSenderThread.detach();
                break;
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

void Server::handleSecificMessageReceivers(string username, int client_fd, string target) {
    try {
        std::cout << "Received message: '"  << username  << "'" << std::endl;
        std::cout << "Private trigger: '"   << client_fd << "'" << std::endl;
        std::cout << "Private trigger: '"   << target    << "'" << std::endl;

        while (true) {
            char     message[1024];
            int      bytesReceived = recv(client_fd, message, sizeof(message) - 1, 0);
            cout << "primili smo porukicu" << endl;
            if (     bytesReceived <= 0 ) {
                if ( bytesReceived == 0) {
                     cout << "Client disconnected mid private chat..." << endl;
                }
                else {
                    cerr << "Receive error: " << strerror(errno) << endl;
                }
                close(client_fd);
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    auto it   =  connectedClients.begin();
                    while (it != connectedClients.end()) {
                        if(it->  second.client_fd == client_fd) {
                           it =  connectedClients.erase(it);
                        }
                        else {
                            ++it;
                        }
                    }
                }
                break;
            }
            message[bytesReceived] = '\0';

            std::lock_guard<std::mutex>   lock(clients_mutex);
            for(const auto &cli : connectedClients) {
                if (cli.first != username && cli.first == target) {
                    string  combo_message =          username + " has privately sent u : " + message;
                    ssize_t bytes_sent    =          send(cli.second.client_fd, combo_message.c_str(), combo_message.size(), 0);
                    if (    bytes_sent    == -1) {
                            cerr << "Failed to send private message to " << cli.first << ": " << strerror(errno) << endl;
                    }
                    else {
                        cout << "Private message sent to client " << cli.first << " (fd: " << cli.second.client_fd << ")\n";
                    }
                }
            }
        }
    }
    catch (const exception e) {
        cerr << e.what() << endl;
        cout << " server receiver handler";
    }
}
