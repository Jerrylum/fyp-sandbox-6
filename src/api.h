#pragma once

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <map>
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


#define FRAME_SIZE (128)
#define FRAME_HEADER_SIZE (32)
#define KEY_TYPE uint32_t
#define KEY_SIZE sizeof(KEY_TYPE)
#define SECONDARY_KEY_SIZE (FRAME_HEADER_SIZE - KEY_SIZE)
#define VALUE_SIZE (FRAME_SIZE - KEY_SIZE)
#define SECONDARY_VALUE_SIZE (VALUE_SIZE - SECONDARY_KEY_SIZE)

#define INDEX_BIT_SIZE (19)
#define INDEX_COUNT (1 << INDEX_BIT_SIZE)

struct Value {
  uint8_t value[VALUE_SIZE];
  struct Value *next;
};

class Table {
 private:
  struct Value *data[INDEX_COUNT] = {};
  bool running = false;
  
  pthread_t deamon_thread;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

  friend void* table_run(void* param);

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
  OrdinaryTable() {};
  ~OrdinaryTable() {};
  
  void put_frame(uint8_t frame[FRAME_SIZE]);
  void put(KEY_TYPE key, uint8_t value[VALUE_SIZE]);
  uint8_t get_by_header(uint8_t dst[SECONDARY_VALUE_SIZE], uint8_t header[FRAME_HEADER_SIZE]);
  uint8_t get(uint8_t dst[SECONDARY_VALUE_SIZE], KEY_TYPE key, uint8_t sec_key[SECONDARY_KEY_SIZE]);
};
