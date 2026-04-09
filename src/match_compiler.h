#ifndef CRYSTAL_MATCH_COMPILER
#define CRYSTAL_MATCH_COMPILER

enum compiler_type_enum {
  GCC, GXX,
  CLANG, CLANGXX,
  UNKNOWN_COMPILER
};

enum compiler_type_enum identify_compiler(const char* file_path);

#endif
