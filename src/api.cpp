#include "api.h"

#include <sys/time.h>

#include <chrono>

void *table_run(void *param) {
  Table *t = (Table *)param;
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
    if (now_time.tv_sec < out_time.tv_sec) continue;
    out_time.tv_sec = now_time.tv_sec + GENERATION_RENEW_SEC;

    // auto start = std::chrono::high_resolution_clock::now();

    t->queue.renew();

    // auto end = std::chrono::high_resolution_clock::now();

    // std::cout << "renew time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
    //           << std::endl;

    std::cout << "table run" << std::endl;
  }

  std::cout << "table run end" << std::endl;

  return nullptr;
}

static void free_linked_list(struct Value *v) {
  while (v != NULL) {
    struct Value *next = v->next;
    delete v;
    v = next;
  }
}

void ValueQueue::init() {
  data = (Value **)malloc(sizeof(struct Value *) * GENERATION_COUNT * INDEX_COUNT);
  // check if malloc failed
  for (int i = 0; i < GENERATION_COUNT * INDEX_COUNT; i++) data[i] = NULL;
}

void ValueQueue::renew() {
  pthread_mutex_lock(&mutex);

  garbage_index = (garbage_index + 1) % GENERATION_COUNT;
  uint32_t next_index = (current_index + 1) % GENERATION_COUNT;
  memcpy(&data[next_index * INDEX_COUNT], &data[current_index * INDEX_COUNT], sizeof(struct Value *) * INDEX_COUNT);
  std::cout << "renew: " << current_index << " -> " << next_index << std::endl;
  current_index = next_index;

  pthread_mutex_unlock(&mutex);

  
  for (int i = 0; i < INDEX_COUNT; i++) {
    struct Value *garbage_v = last(i);
    if (garbage_v == NULL) continue;

    free_linked_list(garbage_v->next);
    garbage_v->next = NULL;
  }
}

Table::Table() {
  pthread_create(&deamon_thread, NULL, table_run, this);

  queue.init();

  std::cout << "table constructor" << std::endl;
}

Table::~Table() {
  std::cout << "table destructor" << std::endl;

  pthread_mutex_lock(&this->mutex);
  this->running = false;
  pthread_cond_signal(&this->cond);

  for (int i = 0; i < INDEX_COUNT; i++) {
    free_linked_list(queue.current(i));
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
  struct Value *head = queue.current(index);

  struct Value *v = new struct Value;
  memcpy(v->value, value, VALUE_SIZE);
  v->next = head;

  // std::cout << "put: " << key << " index:" << index << std::endl;

  // pthread_mutex_lock(&mutex);

  queue.current(index) = v;

  // pthread_mutex_unlock(&mutex);
}

uint8_t Table::get_by_header(uint8_t *dst, uint8_t *header) {
  KEY_TYPE key;
  memcpy(&key, header, KEY_SIZE);

  return get(dst, key, header + KEY_SIZE);
}

uint8_t Table::get(uint8_t *dst, KEY_TYPE key, uint8_t *sec_key) {
  KEY_TYPE index = key >> (KEY_SIZE * 8 - INDEX_BIT_SIZE);
  // std::cout << "search: " << key << " index:" << index << std::endl;
  struct Value *v = queue.current(index);
  struct Value *garbage = queue.last(index);

  while (v != garbage) {
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
