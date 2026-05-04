#include <string.h>

#include "string_utils.h"

bool string_ends_with(const char* path, const char* suffix) {
  const unsigned int path_length = strlen(path);
  const unsigned int suffix_length = strlen(suffix);

  if (path_length < suffix_length) {
    return false;
  }

  return (memcmp(path + path_length - suffix_length, suffix, suffix_length) == 0);
}
