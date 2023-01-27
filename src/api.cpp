#include "api.h"

#include <sys/time.h>

#include <chrono>
#include <thread>

void *table_run(void *param) {
  Table *t = (Table *)param;
  // t->running = true;

  struct timeval now_time;
  struct timespec out_time;

  std::cout << "table run start" << std::endl;

  while (t->running) {
    pthread_mutex_lock(&t->mutex);
    pthread_cond_timedwait(&t->cond, &t->mutex, &out_time);
    pthread_mutex_unlock(&t->mutex);
    if (!t->running) break;

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

void ValueQueue::destroy() {
  for (int i = 0; i < INDEX_COUNT; i++) {
    free_linked_list(current(i));
  }
  free(data);
}

Table::Table() {
  pthread_create(&deamon_thread, NULL, table_run, this);

  queue.init();

  std::cout << "table constructor" << std::endl;
}

Table::~Table() { 
  std::cout << "table destructor" << std::endl;

  this->running = false;
  pthread_cond_signal(&this->cond);
  pthread_join(this->deamon_thread, NULL);

  pthread_mutex_lock(&this->mutex);
  queue.destroy();
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

  struct Value *v = new struct Value;
  memcpy(v->value, value, VALUE_SIZE);

  // TODO multi mutex

  pthread_mutex_lock(&mutex);
  v->next = queue.current(index);
  queue.current(index) = v;
  pthread_mutex_unlock(&mutex);

  // std::cout << "put: " << key << " index:" << index << std::endl;
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

OrdinaryTable::~OrdinaryTable() {
  for (auto it = data.begin(); it != data.end(); it++) {
    free_linked_list(it->second);
  }
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

////////////////////////////////////////

UnorderedMapTable::~UnorderedMapTable() {
  for (auto it = data.begin(); it != data.end(); it++) {
    free_linked_list(it->second);
  }
}

void UnorderedMapTable::put_frame(uint8_t *frame) {
  KEY_TYPE key;
  memcpy(&key, frame, KEY_SIZE);
  put(key, frame + KEY_SIZE);
}

void UnorderedMapTable::put(KEY_TYPE key, uint8_t *value) {
  struct Value *v = new struct Value;
  memcpy(v->value, value, VALUE_SIZE);
  v->next = NULL;

  data[key] = v;
}

uint8_t UnorderedMapTable::get_by_header(uint8_t *dst, uint8_t *header) {
  KEY_TYPE key;
  memcpy(&key, header, KEY_SIZE);

  return get(dst, key, header + KEY_SIZE);
}

uint8_t UnorderedMapTable::get(uint8_t *dst, KEY_TYPE key, uint8_t *sec_key) {
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

////////////////////////////////////////

// using https://github.com/tranvictor/AVLmap

AvlTreeMapTable::~AvlTreeMapTable() {
  for (auto it = data.begin(); it != data.end(); it++) {
    free_linked_list(it->second);
  }
}

void AvlTreeMapTable::put_frame(uint8_t *frame) {
  KEY_TYPE key;
  memcpy(&key, frame, KEY_SIZE);
  put(key, frame + KEY_SIZE);
}

void AvlTreeMapTable::put(KEY_TYPE key, uint8_t *value) {
  struct Value *v = new struct Value;
  memcpy(v->value, value, VALUE_SIZE);
  v->next = NULL;

  data[key] = v;
}

uint8_t AvlTreeMapTable::get_by_header(uint8_t *dst, uint8_t *header) {
  KEY_TYPE key;
  memcpy(&key, header, KEY_SIZE);

  return get(dst, key, header + KEY_SIZE);
}

uint8_t AvlTreeMapTable::get(uint8_t *dst, KEY_TYPE key, uint8_t *sec_key) {
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
