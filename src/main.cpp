#include <thread>

#include "api.h"

// #define SEND_OPERATION_OPTIMIZATION
// #define LISTEN_OPERATION_OPTIMIZATION

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
  if (size == 0) return;

  uint8_t operation = buffer[0];  // 0 = send, 1 = listen

  if (size == (1 + 32 + 1 + 128)) {
    // multiple messages can be sent in one TCP packet

    handle_client_income(fd, buffer, 1 + 32);
    handle_client_income(fd, buffer + 1 + 32, size - (1 + 32));
    return;
  } else if (operation == 0) {
    if (size < 129) goto ERROR;

    int32_t listener = listener_table->pull(fd, (uint8_t*)buffer + 1);
    if (listener != -1) {
      server.sendData((Server::Connector){listener}, buffer + 1, size - 1);
      std::cout << "Passing through from FD" << fd << " to FD" << listener << std::endl;
    }
#ifdef SEND_OPERATION_OPTIMIZATION
    else
#endif
    {
      table->put_frame((uint8_t*)buffer + 1);
      printf("Putting frame from FD%d with initial %02hhx\n", fd, buffer[1]);
    }
    return;
  } else if (operation == 1) {
    if ((size - 1) % 32 != 0) goto ERROR;

    uint8_t frame[FRAME_SIZE] = {0};

    uint32_t count = (size - 1) / 32;
    for (uint32_t i = 0; i < count; i++) {
      uint32_t offset = 1 + i * 32;

      if (table->get_by_header(frame + 32, (uint8_t*)buffer + offset)) {
        memcpy(frame, buffer + offset, 32);
        server.sendData((Server::Connector){fd}, (char*)frame, FRAME_SIZE);
        printf("Returning a frame directly to FD%d with initial %02hhx\n", fd, (buffer + offset)[0]);
#ifdef LISTEN_OPERATION_OPTIMIZATION
        return;
#else
        break;
#endif
      }
    }

    listener_table->listen(fd, (uint8_t*)buffer + 1, count);
    printf("Client FD%d is listening to frames", fd);
    for (int i = 0; i < count; i++) {
      printf(" %02hhx", buffer[1 + i * 32]);
    }
    printf("\n");
    return;
  }

ERROR:
  std::cout << "incorrect operation " << (int)operation << " with " << size << " bytes" << std::endl;
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
