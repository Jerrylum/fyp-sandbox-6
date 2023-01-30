// Gydo194

/*
 * File:   Server.h
 * Author: gydo194
 *
 * Created on March 7, 2018, 9:09 PM
 */

#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>  //sockaddr, socklen_t
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#define SERVER_DEBUG

#define INPUT_BUFFER_SIZE (256 + 1)  // Byte, 8 headers in total
#define DEFAULT_PORT (25000)

class Server {
 public:
  Server();
  Server(int port);
  Server(const Server &orig);
  virtual ~Server();

  struct Connector {
    uint16_t source_fd;
  };

  void shutdown();
  void init();
  void loop();

  void onConnect(void (*ncc)(uint16_t fd));
  void onRecvData(void (*rc)(uint16_t fd, char *buffer, uint32_t size));
  void onDisconnect(void (*dc)(uint16_t fd));

  uint16_t sendData(Connector conn, const char *messageBuffer, uint32_t size);
  uint16_t sendData(Connector conn, char *messageBuffer, uint32_t size);

 private:
  // fd_set file descriptor sets for use with FD_ macros
  fd_set master_fd_set;
  fd_set temp_fd_set;

  // unsigned integer to keep track of maximum fd value, required for select()
  uint16_t max_fd;

  // socket file descriptors
  int master_fd;  // master socket which receives new connections
  int temp_fd;    // temporary socket file descriptor which holds new clients

  // client connection data
  struct sockaddr_storage client_addr;
  struct sockaddr_in serv_addr;
  char input_buffer[INPUT_BUFFER_SIZE];

  void (*newConnectionCallback)(uint16_t fd);
  void (*receiveCallback)(uint16_t fd, char *buffer, uint32_t size);
  void (*disconnectCallback)(uint16_t fd);

  // function prototypes
  void setup(int port);
  void initializeSocket();
  void bindSocket();
  void startListen();
  void handleNewConnection();
  void recvDataFromConnection(int fd);
};

#endif /* SERVER_H */
