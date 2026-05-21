#ifndef CRYSTAL_MATCH_COMPILER
#define CRYSTAL_MATCH_COMPILER

enum compiler_type_enum {
  GCC_FAMILY = 0,
  CLANG_FAMILY = 1,
  ICX_FAMILY = 2,
  UNKNOWN_FAMILY = 3
};

struct compiler_match_data {
  enum compiler_type_enum type;
  unsigned int priority;
};

struct compiler_match_data identify_compiler(const char* file_path);

#endif
