#include "utils.h"

int calc_serial_type_size(const size_t serial_type,
                          size_t *const content_size) {
  if (serial_type == 0) {
    *content_size = 0;
  }
  if (serial_type == 1) {
    *content_size = 1;
  }
  if (serial_type == 2) {
    *content_size = 2;
  }
  if (serial_type == 3) {
    *content_size = 3;
  }
  if (serial_type == 4) {
    *content_size = 4;
  }
  if (serial_type == 5) {
    *content_size = 6;
  }
  if (serial_type == 6) {
    *content_size = 8;
  }
  if (serial_type == 7) {
    *content_size = 8;
  }
  if (serial_type == 8) {
    *content_size = 0;
  }
  if (serial_type == 9) {
    *content_size = 0;
  }
  if (serial_type == 10 || serial_type == 11) {
    return -1;
  }
  if (serial_type >= 12 && serial_type % 2 == 0) {
    *content_size = (serial_type - 12) / 2;
  }
  if (serial_type >= 13 && serial_type % 2 != 0) {
    *content_size = (serial_type - 13) / 2;
  }
  return 0;
}
