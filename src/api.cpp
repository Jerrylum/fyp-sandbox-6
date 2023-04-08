#include "api.h"

#include <sys/time.h>

#include <chrono>
#include <thread>

ListenerTable::ListenerTable() {
  by_requests = (ListenRequest **)malloc(sizeof(ListenRequest *) * INDEX_COUNT);
  by_listeners = (ListenRequest **)malloc(sizeof(ListenRequest *) * 65536 * MAXIMUM_REQUEST);

  for (int i = 0; i < INDEX_COUNT; i++) {
    by_requests[i] = NULL;
  }
  for (int i = 0; i < 65536 * MAXIMUM_REQUEST; i++) {
    by_listeners[i] = NULL;
  }
}

ListenerTable::~ListenerTable() {
  for (int i = 0; i < INDEX_COUNT; i++) {
    ListenRequest *request = by_requests[i];
    while (request != NULL) {
      ListenRequest *next = request->next;
      free(request);
      request = next;
    }
  }

  free(by_requests);
  free(by_listeners);
}

/**
 * @brief Return the listener fd if there is a listener for the header
 *
 * How it works:
 *
 */
int32_t ListenerTable::pull(uint16_t fd, uint8_t header[FRAME_HEADER_SIZE]) {
  uint16_t listener = 0;

  KEY_TYPE key = *(uint32_t *)header;
  KEY_TYPE index = key >> (KEY_SIZE * 8 - INDEX_BIT_SIZE);

  struct ListenRequest *request = by_requests[index];
  while (request != NULL) {
    if (request->listener == fd || memcmp(request->header, header, FRAME_HEADER_SIZE) != 0) {
      request = request->next;
      continue;
    }

    listener = request->listener;

    // remove from by_listeners
    for (uint8_t i = 0; i < MAXIMUM_REQUEST; i++) {
      if (by_listeners[listener * MAXIMUM_REQUEST + i] == request) {
        by_listeners[listener * MAXIMUM_REQUEST + i] = NULL;
      }
    }

    remove_from_bucket(request);
    return listener;
  }

  return -1;
}

/**
 * It first removes all the listening requests for the fd. Then, it adds the new listening requests to the table.
 */
void ListenerTable::listen(uint16_t fd, uint8_t *headers, uint8_t count) {
  remove(fd);

  for (int i = 0; i < count && i < MAXIMUM_REQUEST; i++) {
    struct ListenRequest *request = new ListenRequest();
    memcpy(request->header, headers + i * FRAME_HEADER_SIZE, FRAME_HEADER_SIZE);
    request->listener = fd;

    insert_to_bucket(request);

    by_listeners[fd * MAXIMUM_REQUEST + i] = request;
  }
}

/**
 * It removes all the listening requests for the fd.
*/
void ListenerTable::remove(uint16_t fd) {
  for (uint8_t i = 0; i < MAXIMUM_REQUEST; i++) {
    remove_from_bucket(by_listeners[fd * MAXIMUM_REQUEST + i]);
    by_listeners[fd * MAXIMUM_REQUEST + i] = NULL;
  }
}

void ListenerTable::remove_from_bucket(ListenRequest *request) {
  if (request == NULL) return;

  if (request->previous != NULL) {
    request->previous->next = request->next;
  } else {
    KEY_TYPE key = *(uint32_t *)request->header;
    KEY_TYPE index = key >> (KEY_SIZE * 8 - INDEX_BIT_SIZE);
    by_requests[index] = request->next;
  }

  delete request;
}

void ListenerTable::insert_to_bucket(ListenRequest *request) {
  KEY_TYPE key = *(uint32_t *)request->header;
  KEY_TYPE index = key >> (KEY_SIZE * 8 - INDEX_BIT_SIZE);

  struct ListenRequest *head = by_requests[index];
  if (head == NULL) {
    request->previous = NULL;
    request->next = NULL;
    by_requests[index] = request;
  } else {
    request->previous = NULL;
    request->next = head;
    head->previous = request;
    by_requests[index] = request;
  }
}

void *table_run(void *param) {
  Table *t = (Table *)param;

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

    t->queue.renew(&t->mutex);

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

void ValueQueue::renew(pthread_mutex_t *mutex) {
  pthread_mutex_lock(mutex);

  garbage_index = (garbage_index + 1) % GENERATION_COUNT;
  uint32_t next_index = (current_index + 1) % GENERATION_COUNT;
  memcpy(&data[next_index * INDEX_COUNT], &data[current_index * INDEX_COUNT], sizeof(struct Value *) * INDEX_COUNT);
  std::cout << "renew: " << current_index << " -> " << next_index << std::endl;
  current_index = next_index;

  pthread_mutex_unlock(mutex);

  for (int i = 0; i < INDEX_COUNT; i++) {
    struct Value *garbage_v = last(i);
    if (garbage_v == NULL) continue;

    free_linked_list(garbage_v->next);
    garbage_v->next = NULL;
  }
}

struct Value *&ValueQueue::current(uint32_t offset) { return data[current_index * INDEX_COUNT + offset]; }

struct Value *&ValueQueue::last(uint32_t offset) { return data[garbage_index * INDEX_COUNT + offset]; }

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
}

uint8_t Table::get_by_header(uint8_t *dst, uint8_t *header) {
  KEY_TYPE key;
  memcpy(&key, header, KEY_SIZE);

  return get(dst, key, header + KEY_SIZE);
}

uint8_t Table::get(uint8_t *dst, KEY_TYPE key, uint8_t *sec_key) {
  KEY_TYPE index = key >> (KEY_SIZE * 8 - INDEX_BIT_SIZE);
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
