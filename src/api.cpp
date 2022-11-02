#include "api.h"

#include <iostream>

table::table() { std::cout << "table constructor" << std::endl; }

table::~table() {
  std::cout << "table destructor" << std::endl;

  for (int i = 0; i < INDEX_COUNT; i++) {
    struct value *v = data[i];
    while (v != NULL) {
      struct value *next = v->next;
      delete v;
      v = next;
    }
  }
}

void table::put_frame(uint8_t *frame) {
  KEY_TYPE key;
  memcpy(&key, frame, KEY_SIZE);
  put(key, frame + KEY_SIZE);
}

void table::put(KEY_TYPE key, uint8_t *value) {

  KEY_TYPE index = key >> (KEY_SIZE * 8 - INDEX_BIT_SIZE);
  struct value *head = data[index];

  struct value *v = new struct value;
  memcpy(v->value, value, VALUE_SIZE);
  v->next = head;

  data[index] = v;
}

uint8_t table::get_by_header(uint8_t *dst, uint8_t *header) {
  KEY_TYPE key;
  memcpy(&key, header, KEY_SIZE);

  return get(dst, key, header + KEY_SIZE);
}

uint8_t table::get(uint8_t *dst, KEY_TYPE key, uint8_t *sec_key) {
  KEY_TYPE index = key >> (KEY_SIZE * 8 - INDEX_BIT_SIZE);
  struct value *v = data[index];

  while (v != NULL) {
    if (memcmp(v->value, sec_key, SECONDARY_KEY_SIZE) == 0) {
      memcpy(dst, v->value + SECONDARY_KEY_SIZE, SECONDARY_VALUE_SIZE);
      return 1;
    }
    v = v->next;
  }

  return 0;
}
