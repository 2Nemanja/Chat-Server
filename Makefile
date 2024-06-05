GCC = 					g++
CFLAGS = 				-Wall -Wextra 
SRC = 					main_server.cpp src/server.cpp
SRC_CLIENT =			main_client.cpp src/client.cpp 
SERVER_OUT = 			server
CLIENT_OUT = 			client

all: 					server client

server: $(SRC)
	$(GCC) $(CFLAGS) $^ -o $(SERVER_OUT)

client: $(SRC_CLIENT)
	$(GCC) $(CFLAGS) $^ -o $(CLIENT_OUT)

clean_server:
	rm -rf $(SERVER_OUT)

clean_client:
	rm -rf $(CLIENT_OUT)

clean: 					clean_server clean_client
