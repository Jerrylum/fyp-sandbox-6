#pragma once

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <cmath>
#include <iostream>
#include <map>
#include <unordered_map>

#include "avlmap.h"
#include "tcp_server.h"

#define FRAME_SIZE (128)
#define FRAME_HEADER_SIZE (32)
#define KEY_TYPE uint32_t
#define KEY_SIZE sizeof(KEY_TYPE)
#define SECONDARY_KEY_SIZE (FRAME_HEADER_SIZE - KEY_SIZE)
#define VALUE_SIZE (FRAME_SIZE - KEY_SIZE)
#define SECONDARY_VALUE_SIZE (VALUE_SIZE - SECONDARY_KEY_SIZE)

#define INDEX_BIT_SIZE (19)
#define INDEX_COUNT (1 << INDEX_BIT_SIZE)

#define GENERATION_COUNT 6
#define GENERATION_RENEW_SEC 15

#define MAXIMUM_REQUEST 8

struct ListenRequest {
  struct ListenRequest *next;  // order shouldn't matter
  struct ListenRequest *previous;
  uint8_t header[FRAME_HEADER_SIZE];
  uint16_t listener;

  ListenRequest() {
    next = NULL;
    previous = NULL;
  }
};

/**
 * Listener Table is a new data structure that is used to store the listeners.
 *
 * It is a hash table with two buckets.
 */
class ListenerTable {
 private:
  struct ListenRequest **by_requests;
  struct ListenRequest **by_listeners;

  void remove_from_bucket(ListenRequest *request);
  void insert_to_bucket(ListenRequest *request);

 public:
  ListenerTable();
  ~ListenerTable();

  int32_t pull(uint16_t fd, uint8_t header[FRAME_HEADER_SIZE]);
  void listen(uint16_t fd, uint8_t *headers, uint8_t count);
  void remove(uint16_t fd);
};

/**
 * Node
 */
struct Value {
  uint8_t value[VALUE_SIZE];
  struct Value *next;
};

/**
 * Value Queue is a circular queue of Value pointers with a garbage collector.
 */
struct ValueQueue {
  struct Value **data;
  uint32_t current_index = 0;
  uint32_t garbage_index = 1;

  void init();

  void renew(pthread_mutex_t *mutex);

  struct Value *&current(uint32_t offset = 0);

  struct Value *&last(uint32_t offset = 0);

  void destroy();
};

class Table {
 private:
  struct ValueQueue queue;
  bool running = true;

  pthread_t deamon_thread;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

  friend void *table_run(void *param);

 public:
  Table();
  ~Table();

  void put_frame(uint8_t frame[FRAME_SIZE]);
  void put(KEY_TYPE key, uint8_t value[VALUE_SIZE]);
  uint8_t get_by_header(uint8_t dst[SECONDARY_VALUE_SIZE], uint8_t header[FRAME_HEADER_SIZE]);
  uint8_t get(uint8_t dst[SECONDARY_VALUE_SIZE], KEY_TYPE key, uint8_t sec_key[SECONDARY_KEY_SIZE]);
};

// extend table
class OrdinaryTable {
 private:
  std::map<KEY_TYPE, struct Value *> data;

 public:
  OrdinaryTable(){};
  ~OrdinaryTable();

  void put_frame(uint8_t frame[FRAME_SIZE]);
  void put(KEY_TYPE key, uint8_t value[VALUE_SIZE]);
  uint8_t get_by_header(uint8_t dst[SECONDARY_VALUE_SIZE], uint8_t header[FRAME_HEADER_SIZE]);
  uint8_t get(uint8_t dst[SECONDARY_VALUE_SIZE], KEY_TYPE key, uint8_t sec_key[SECONDARY_KEY_SIZE]);
};

// extend table
class UnorderedMapTable {
 private:
  std::unordered_map<KEY_TYPE, struct Value *> data;

 public:
  UnorderedMapTable(){};
  ~UnorderedMapTable();

  void put_frame(uint8_t frame[FRAME_SIZE]);
  void put(KEY_TYPE key, uint8_t value[VALUE_SIZE]);
  uint8_t get_by_header(uint8_t dst[SECONDARY_VALUE_SIZE], uint8_t header[FRAME_HEADER_SIZE]);
  uint8_t get(uint8_t dst[SECONDARY_VALUE_SIZE], KEY_TYPE key, uint8_t sec_key[SECONDARY_KEY_SIZE]);
};

// extend table
class AvlTreeMapTable {
 private:
  avl_tree<KEY_TYPE, struct Value *> data;

 public:
  AvlTreeMapTable(){};
  ~AvlTreeMapTable();

  void put_frame(uint8_t frame[FRAME_SIZE]);
  void put(KEY_TYPE key, uint8_t value[VALUE_SIZE]);
  uint8_t get_by_header(uint8_t dst[SECONDARY_VALUE_SIZE], uint8_t header[FRAME_HEADER_SIZE]);
  uint8_t get(uint8_t dst[SECONDARY_VALUE_SIZE], KEY_TYPE key, uint8_t sec_key[SECONDARY_KEY_SIZE]);
};
