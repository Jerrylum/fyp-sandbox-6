/*
 * Server.cpp
 *
 * EventServer is a simple C++ TCP socket server implementation,
 * built to simplify socket programming (and to serve as an example to anyone who wants to learn it!)
 * It can interface with the rest of your program using three callback functions.
 * - onConnect, which fires when a new client connects. the client's fd is passed.
 * - onDisconnect, which fires when a client disconnects. passes fd.
 * - onRecvData, fires when input is received from a client. passes fd and char*
 *
 * define SERVER_DEBUG to spew out some juicy debug data!
 */

#include "tcp_server.h"

#ifdef SERVER_DEBUG

#define DEBUG(x...) printf(x);

#else

#define DEBUG(x...)

#endif

Server::Server() { setup(DEFAULT_PORT); }

Server::Server(int port) { setup(port); }

Server::Server(const Server &orig) {}

Server::~Server() {
  DEBUG("[SERVER] [DESTRUCTOR] Destroying Server...\n");
  close(master_fd);
}

void Server::setup(int port) {
  master_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (master_fd < 0) {
    perror("[SERVER] [ERROR] Socket creation failed");
  }

  FD_ZERO(&master_fd_set);
  FD_ZERO(&temp_fd_set);

  memset(&serv_addr, 0, sizeof(serv_addr));  // bzero
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
  serv_addr.sin_port = htons(port);
}

void Server::initializeSocket() {
  int opt_value = 1;
  if (setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt_value, sizeof(int)) < 0) {
    perror("[SERVER] [ERROR] setsockopt() failed");
    shutdown();
  }
}

void Server::bindSocket() {
  if (bind(master_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("[SERVER] [ERROR] bind() failed");
  }

  FD_SET(master_fd, &master_fd_set);  // insert the master socket file-descriptor into the master fd-set
  max_fd = master_fd;                 // set the current known maximum file descriptor count
}

void Server::startListen() {
  if (listen(master_fd, 3) < 0) {
    perror("[SERVER] [ERROR] listen() failed");
  }
}

void Server::shutdown() {
  int close_ret = close(master_fd);
  DEBUG("[SERVER] [DEBUG] [SHUTDOWN] closing master fd..  ret '%d'.\n", close_ret);
}

void Server::handleNewConnection() {
  DEBUG("[SERVER] [CONNECTION] handling new connection\n");

  socklen_t addrlen = sizeof(client_addr);
  temp_fd = accept(master_fd, (struct sockaddr *)&client_addr, &addrlen);

  if (temp_fd < 0) {
    perror("[SERVER] [ERROR] accept() failed");
    return;
  }

  FD_SET(temp_fd, &master_fd_set);
  // increment the maximum known file descriptor (select() needs it)
  if (temp_fd > max_fd) {
    max_fd = temp_fd;
    DEBUG("[SERVER] incrementing max_fd to %hu.\n", max_fd);
  }

  DEBUG("[SERVER] [CONNECTION] New connection on socket fd '%d'.\n", temp_fd);
  newConnectionCallback(temp_fd);  // call the callback
}

void Server::recvDataFromConnection(int fd) {
  int n_bytes_recv = recv(fd, input_buffer, INPUT_BUFFER_SIZE, 0);
  if (n_bytes_recv <= 0) {
    if (n_bytes_recv != 0) {
      perror("[SERVER] [ERROR] recv() failed");
    }
    disconnectCallback((uint16_t)fd);
    close(fd);                   // close connection to client
    FD_CLR(fd, &master_fd_set);  // clear the client fd from fd set
    return;
  }

  DEBUG("[SERVER] [RECV] Income from client %hu!\n", fd);
  receiveCallback(fd, input_buffer, n_bytes_recv);
}

void Server::loop() {
  DEBUG("[SERVER] [MISC] max fd = '%hu', calling select()\n", max_fd);

  temp_fd_set = master_fd_set;                                   // copy fd_set for select()
  if (select(max_fd + 1, &temp_fd_set, NULL, NULL, NULL) < 0) {  // blocks until activity
    perror("[SERVER] [ERROR] select() failed");
    shutdown();
  }

  // loop the fd_set and check which socket has interactions available
  for (int i = 0; i <= max_fd; i++) {
    if (FD_ISSET(i, &temp_fd_set)) {  // if the socket has activity pending
      if (i == master_fd) {
        handleNewConnection();  // new connection on master socket
      } else {
        recvDataFromConnection(i);  // existing connection has new data
      }
    }
  }
}

void Server::init() {
  initializeSocket();
  bindSocket();
  startListen();
}

void Server::onRecvData(void (*rc)(uint16_t fd, char *buffer, uint32_t size)) { receiveCallback = rc; }

void Server::onConnect(void (*ncc)(uint16_t)) { newConnectionCallback = ncc; }

void Server::onDisconnect(void (*dc)(uint16_t)) { disconnectCallback = dc; }

uint16_t Server::sendData(Connector conn, char *buffer, uint32_t size) {
  return send(conn.source_fd, buffer, size, 0);
}

uint16_t Server::sendData(Connector conn, const char *buffer, uint32_t size) {
  return send(conn.source_fd, buffer, size, 0);
}
