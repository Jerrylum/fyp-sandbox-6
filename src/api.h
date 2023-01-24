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

struct Value {
  uint8_t value[VALUE_SIZE];
  struct Value *next;
};

struct ValueQueue {
  struct Value **data;
  uint32_t current_index = 0;
  uint32_t garbage_index = 1;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  void init();

  void renew();

  struct Value *&current(uint32_t offset = 0) { return data[current_index * INDEX_COUNT + offset]; }

  struct Value *&last(uint32_t offset = 0) { return data[garbage_index * INDEX_COUNT + offset]; }
};

class Table {
 private:
  struct ValueQueue queue;
  bool running = false;

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
  ~OrdinaryTable(){};

  void put_frame(uint8_t frame[FRAME_SIZE]);
  void put(KEY_TYPE key, uint8_t value[VALUE_SIZE]);
  uint8_t get_by_header(uint8_t dst[SECONDARY_VALUE_SIZE], uint8_t header[FRAME_HEADER_SIZE]);
  uint8_t get(uint8_t dst[SECONDARY_VALUE_SIZE], KEY_TYPE key, uint8_t sec_key[SECONDARY_KEY_SIZE]);
};
