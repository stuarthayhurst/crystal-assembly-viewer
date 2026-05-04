#ifndef CRYSTAL_DETECT_COMPILERS
#define CRYSTAL_DETECT_COMPILERS

#include "match_compiler.h"

struct compiler_info {
  char* path;
  enum compiler_type_enum type;
  unsigned int priority;
};

struct compiler_info* detect_unique_compilers(unsigned int* compiler_count);

void free_compiler_array(struct compiler_info* compiler_paths, unsigned int compiler_count);

#endif
