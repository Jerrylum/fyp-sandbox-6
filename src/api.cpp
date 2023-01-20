#include "api.h"

#include <iostream>
#include <sys/time.h>

void* table_run(void* param) {
  Table* t = (Table*)param;
  t->running = true;

  struct timeval now_time;
  struct timespec out_time;

  std::cout << "table run start" << std::endl;

  while (t->running) {
    pthread_mutex_lock(&t->mutex);
    pthread_cond_timedwait(&t->cond, &t->mutex, &out_time);
    if (!t->running) break;
    pthread_mutex_unlock(&t->mutex);

    gettimeofday(&now_time, NULL);
    if (now_time.tv_sec >= out_time.tv_sec) {
      out_time.tv_sec = now_time.tv_sec + 1;
      std::cout << "table run" << std::endl;
    }
  }

  std::cout << "table run end" << std::endl;

  return nullptr;
}

Table::Table() {
  pthread_create(&deamon_thread, NULL, table_run, this);

  std::cout << "table constructor" << std::endl;
}

Table::~Table() {
  std::cout << "table destructor" << std::endl;

  pthread_mutex_lock(&this->mutex);
  this->running = false;
  pthread_cond_signal(&this->cond);

  for (int i = 0; i < INDEX_COUNT; i++) {
    struct Value *v = data[i];
    while (v != NULL) {
      struct Value *next = v->next;
      delete v;
      v = next;
    }
  }
  
  pthread_mutex_unlock(&this->mutex);

  pthread_cond_destroy(&this->cond);
  pthread_mutex_destroy(&this->mutex);
}

void Table::put_frame(uint8_t *frame) {
  KEY_TYPE key;
  memcpy(&key, frame, KEY_SIZE);
  put(key, frame + KEY_SIZE);
}

void Table::put(KEY_TYPE key, uint8_t *value) {
  KEY_TYPE index = key >> (KEY_SIZE * 8 - INDEX_BIT_SIZE);
  struct Value *head = data[index];

  struct Value *v = new struct Value;
  memcpy(v->value, value, VALUE_SIZE);
  v->next = head;

  data[index] = v;
}

uint8_t Table::get_by_header(uint8_t *dst, uint8_t *header) {
  KEY_TYPE key;
  memcpy(&key, header, KEY_SIZE);

  return get(dst, key, header + KEY_SIZE);
}

uint8_t Table::get(uint8_t *dst, KEY_TYPE key, uint8_t *sec_key) {
  KEY_TYPE index = key >> (KEY_SIZE * 8 - INDEX_BIT_SIZE);
  struct Value *v = data[index];

  while (v != NULL) {
    if (memcmp(v->value, sec_key, SECONDARY_KEY_SIZE) == 0) {
      memcpy(dst, v->value + SECONDARY_KEY_SIZE, SECONDARY_VALUE_SIZE);
      return 1;
    }
    v = v->next;
  }

  return 0;
}

void OrdinaryTable::put_frame(uint8_t *frame) {
  KEY_TYPE key;
  memcpy(&key, frame, KEY_SIZE);
  put(key, frame + KEY_SIZE);
}

void OrdinaryTable::put(KEY_TYPE key, uint8_t *value) {
  struct Value *v = new struct Value;
  memcpy(v->value, value, VALUE_SIZE);
  v->next = NULL;

  data[key] = v;
}

uint8_t OrdinaryTable::get_by_header(uint8_t *dst, uint8_t *header) {
  KEY_TYPE key;
  memcpy(&key, header, KEY_SIZE);

  return get(dst, key, header + KEY_SIZE);
}

uint8_t OrdinaryTable::get(uint8_t *dst, KEY_TYPE key, uint8_t *sec_key) {
  struct Value *v = data[key];

  if (v == NULL) {
    return 0;
  }

  if (memcmp(v->value, sec_key, SECONDARY_KEY_SIZE) == 0) {
    memcpy(dst, v->value + SECONDARY_KEY_SIZE, SECONDARY_VALUE_SIZE);
    return 1;
  }

  return 0;
}
