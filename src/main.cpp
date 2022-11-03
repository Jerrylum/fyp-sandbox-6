#include "api.h"

int main() {
  // hello("CMake");

  ordinary_table t = ordinary_table();

  uint8_t frame[FRAME_SIZE] = {0};
  getrandom(frame, FRAME_SIZE, 0);

  t.put_frame(frame);

  uint8_t dst[SECONDARY_VALUE_SIZE] = {0};
  t.get_by_header(dst, frame);

  printf("\nFra: ");

  for (int i = 0; i < FRAME_SIZE; i++) {
    printf("%02x ", frame[i]);
  }

  printf("\nDST: ");

  // print dst
  for (int i = 0; i < SECONDARY_VALUE_SIZE; i++) {
    printf("%02x ", dst[i]);
  }

  return 0;
}

