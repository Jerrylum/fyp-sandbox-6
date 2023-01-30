#include <thread>

#include "api.h"

static void* test_client_thread(void* param) {
  std::this_thread::sleep_for(std::chrono::milliseconds((long)param));

  std::cout << "Start connecting to the server!" << std::endl;

  // create a message buffer
  char msg[1500];

  const int port = 25000;
  struct hostent* host = gethostbyname("0.0.0.0");

  sockaddr_in sendSockAddr;
  bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
  sendSockAddr.sin_port = htons(port);

  int clientSd = socket(AF_INET, SOCK_STREAM, 0);

  int status = connect(clientSd, (sockaddr*)&sendSockAddr, sizeof(sendSockAddr));
  if (status < 0) {
    std::cout << "Error connecting to socket!" << std::endl;
    return NULL;
  }
  std::cout << "Connected to the server!" << std::endl;

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // int bytesRead, bytesWritten = 0;
  // struct timeval start1, end1;
  // gettimeofday(&start1, NULL);
  // while (1) {
  //   std::cout << ">";
  //   std::string data;
  //   getline(cin, data);
  //   memset(&msg, 0, sizeof(msg));  // clear the buffer
  //   strcpy(msg, data.c_str());
  //   if (data == "exit") {
  //     send(clientSd, (char*)&msg, strlen(msg), 0);
  //     break;
  //   }
  //   bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0);
  //   cout << "Awaiting server response..." << endl;
  //   memset(&msg, 0, sizeof(msg));  // clear the buffer
  //   bytesRead += recv(clientSd, (char*)&msg, sizeof(msg), 0);
  //   if (!strcmp(msg, "exit")) {
  //     cout << "Server has quit the session" << endl;
  //     break;
  //   }
  //   cout << "Server: " << msg << endl;
  // }
  // gettimeofday(&end1, NULL);
  close(clientSd);

  std::cout << "Connection closed" << std::endl;

  pthread_exit(NULL);

  return NULL;
}

static void test_client(long delay) {
  pthread_t t;
  pthread_create(&t, NULL, test_client_thread, (void*)delay);
}

static Server server;
static Table* table;

static void handle_server_connection(uint16_t fd) { std::cout << "connect: " << fd << std::endl; }

static void handle_client_income(uint16_t fd, char* buffer, uint32_t size) {
  if ((size - 1) % 32 != 0) {
    std::cout << "incorrect income with size " << size << std::endl;  
    return;
  }

  std::cout << "income: " << size << std::endl;

  uint8_t operation = buffer[0]; // 0 = send, 1 = listen

  if (operation == 0) {
    if (0) {

    } else {
      table->put_frame((uint8_t*)buffer + 1);
    }
  } else if (operation == 1) {
    uint8_t frame[FRAME_SIZE] = {0};

    uint32_t count = (size - 1) / 32;
    for (uint32_t i = 0; i < count; i++) {
      uint32_t offset = 1 + i * 32;

      if (table->get_by_header(frame + 32, (uint8_t *)buffer + offset)) {
        memcpy(frame, buffer + offset, 32);
        server.sendData((Server::Connector){fd}, (char*)frame, FRAME_SIZE);
      } else {

      }
    }
  } else {
    std::cout << "incorrect operation " << operation << std::endl;
  }

}

static void handle_client_disconnect(uint16_t fd) { std::cout << "disconnect: " << fd << std::endl; }

int main() {
  test_client(1000);
  test_client(2000);
  test_client(3000);

  test_client(8000);

  table = new Table();

  server.onConnect(handle_server_connection);
  server.onDisconnect(handle_client_disconnect);
  server.onRecvData(handle_client_income);
  server.init();

  // actual main loop
  while (true) {
    server.loop();
  }

  delete table;

  // hello("CMake");

  // OrdinaryTable t = OrdinaryTable();

  // uint8_t frame[FRAME_SIZE] = {0};
  // getrandom(frame, FRAME_SIZE, 0);

  // t.put_frame(frame);

  // uint8_t dst[SECONDARY_VALUE_SIZE] = {0};
  // t.get_by_header(dst, frame);

  // printf("\nFra: ");

  // for (int i = 0; i < FRAME_SIZE; i++) {
  //   printf("%02x ", frame[i]);
  // }

  // printf("\nDST: ");

  // // print dst
  // for (int i = 0; i < SECONDARY_VALUE_SIZE; i++) {
  //   printf("%02x ", dst[i]);
  // }

  return 0;
}
