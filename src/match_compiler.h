#ifndef CRYSTAL_MATCH_COMPILER
#define CRYSTAL_MATCH_COMPILER

enum compiler_type_enum {
  GCC, GXX,
  CLANG, CLANGXX,
  UNKNOWN_COMPILER
};

struct compiler_match_data {
  enum compiler_type_enum type;
  unsigned int priority;
};

struct compiler_match_data identify_compiler(const char* file_path);

#endif
