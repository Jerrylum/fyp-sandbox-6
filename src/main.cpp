#include <thread>

#include "api.h"

static uint8_t testing_msg[129];

static void* test_client_thread1(void* param) {
  std::this_thread::sleep_for(std::chrono::milliseconds((long)param));

  const int port = 25000;
  struct hostent* host = gethostbyname("0.0.0.0");

  sockaddr_in sendSockAddr;
  bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
  sendSockAddr.sin_port = htons(port);

  int clientSd = socket(AF_INET, SOCK_STREAM, 0);

  int status = connect(clientSd, (sockaddr*)&sendSockAddr, sizeof(sendSockAddr));
  if (status < 0) return NULL;

  testing_msg[0] = 0x00;

  send(clientSd, (char*)&testing_msg, 129, 0);

  std::this_thread::sleep_for(std::chrono::seconds(3));
  close(clientSd);

  pthread_exit(NULL);
  return NULL;
}

static void* test_client_thread2(void* param) {
  std::this_thread::sleep_for(std::chrono::milliseconds((long)param));

  const int port = 25000;
  struct hostent* host = gethostbyname("0.0.0.0");

  sockaddr_in sendSockAddr;
  bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
  sendSockAddr.sin_port = htons(port);

  int clientSd = socket(AF_INET, SOCK_STREAM, 0);

  int status = connect(clientSd, (sockaddr*)&sendSockAddr, sizeof(sendSockAddr));
  if (status < 0) return NULL;

  testing_msg[0] = 0x01;

  send(clientSd, (char*)&testing_msg, 33, 0);

  uint8_t testing_msg2[128];
  int read_size = recv(clientSd, (char*)&testing_msg2, 128, 0);

  if (read_size == 128 && memcmp(testing_msg2, testing_msg + 1, 128) == 0) {
    std::cout << "Match!" << std::endl;
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));
  close(clientSd);

  pthread_exit(NULL);
  return NULL;
}

static void test_client(long delay, void* (*runner)(void* param)) {
  pthread_t t;
  pthread_create(&t, NULL, runner, (void*)delay);
}

static Server server;
static Table* table;
static ListenerTable* listener_table;

/**
 * This function is called when a new client is connected to the server. It will print the client's file descriptor.
 */
static void handle_server_connection(uint16_t fd) { std::cout << "connect: " << fd << std::endl; }

/**
 * This function is called when the server receives data from a client. It will process the messages from the client.
 * There are two types of messages: Operation 0 is "Send". The client sends a frame to the server. The server will store
 * the frame in the table and send the frame to the client that is listening for the frame. Operation 1 is "Send".
 * client sends a list of headers to the server. The server will store the client's file descriptor and the headers in
 * the listener table. When the server receives a frame that matches one of the headers, it will send the frame to the
 * client. The corresponding pseudo code can be found in figure 13.
 */
static void handle_client_income(uint16_t fd, char* buffer, uint32_t size) {
  uint8_t operation = buffer[0];  // 0 = send, 1 = listen

  if (operation == 0 && size == 129) {
    int32_t listener = listener_table->pull(fd, (uint8_t*)buffer + 1);
    if (listener != -1) {
      server.sendData((Server::Connector){listener}, buffer + 1, size - 1);
      std::cout << "A: " << fd << ", " << listener << std::endl;
    } else {
      table->put_frame((uint8_t*)buffer + 1);
      std::cout << "B: " << fd << std::endl;
    }
  } else if (operation == 1 && (size - 1) % 32 == 0) {
    uint8_t frame[FRAME_SIZE] = {0};

    uint32_t count = (size - 1) / 32;
    for (uint32_t i = 0; i < count; i++) {
      uint32_t offset = 1 + i * 32;

      if (table->get_by_header(frame + 32, (uint8_t*)buffer + offset)) {
        memcpy(frame, buffer + offset, 32);
        server.sendData((Server::Connector){fd}, (char*)frame, FRAME_SIZE);
        std::cout << "C: " << fd << std::endl;
        return;
      }
    }

    listener_table->listen(fd, (uint8_t*)buffer + 1, count);

    std::cout << "D: " << fd << std::endl;
  } else {
    std::cout << "incorrect operation " << operation << std::endl;
  }
}

/**
 * This function is called when a client disconnects from the server. It will remove the client from the listener table.
 */
static void handle_client_disconnect(uint16_t fd) {
  std::cout << "disconnect: " << fd << std::endl;
  listener_table->remove(fd);
}

/**
 * This is the entry point of the program. It first create a table and a listener table instance. Then, configure the
 * server and start the server loop.
 */
int main() {
  // for (int i = 1; i < 129; i++) {
  //   testing_msg[i] = rand();
  // }

  // test_client(2000, test_client_thread1);
  // test_client(1000, test_client_thread2);

  table = new Table();
  listener_table = new ListenerTable();

  server.onConnect(handle_server_connection);
  server.onDisconnect(handle_client_disconnect);
  server.onRecvData(handle_client_income);
  server.init();

  while (true) {
    server.loop();
  }

  delete table;
  delete listener_table;
  return 0;
}
