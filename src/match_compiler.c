#include <string.h>

#include "match_compiler.h"

enum match_enum {
  //Only matches files with an identical name
  EXACT_MATCH,

  //Matches any file that starts with the compiler's name
  PREFIX_MATCH,

  //Matches any file that starts with the compiler's name, and is immediately followed by a version 
  REQUIRES_VERSION
};

struct compiler_match {
  const char* compiler_prefix;
  enum match_enum mode;
  enum compiler_type_enum type;
};

static struct compiler_match compiler_match_info[8] = {
  {"gcc", EXACT_MATCH, GCC}, {"gcc-", REQUIRES_VERSION, GCC},
  {"g++", EXACT_MATCH, GXX}, {"g++-", REQUIRES_VERSION, GXX},
  {"clang", EXACT_MATCH, CLANG}, {"clang-", REQUIRES_VERSION, CLANG},
  {"clang++", EXACT_MATCH, CLANGXX}, {"clang++-", REQUIRES_VERSION, CLANGXX}
};
static const unsigned int compiler_match_count = \
  sizeof(compiler_match_info) / sizeof(struct compiler_match);

static bool string_starts_with(const char* path, const char* prefix) {
  return (strstr(path, prefix) == path);
}

static bool string_matches(const char* path, const char* prefix) {
  return (strcmp(path, prefix) == 0);
}

static bool string_is_numerical(const char* string) {
  bool is_numerical = false;
  while (*string != '\0') {
    if (*string >= '0' && *string <= '9') {
      is_numerical = true;
    } else {
      return false;
    }

    string++;
  }

  return is_numerical;
}

/*
 - Use matching rules in compiler_match_info to identify a compiler based on its path
 - Returns the enum for the compiler if identified, UNKNOWN_COMPILER otherwise
*/
enum compiler_type_enum identify_compiler(const char* file_path) {
  //Extract the filename from the path
  const char* file_name = strrchr(file_path, '/') + 1;

  //Try and match each known compiler sequentially
  for (unsigned int i = 0; i < compiler_match_count; i++) {
    const struct compiler_match* compiler_match_ptr = &compiler_match_info[i];

    //Attempt to match the compiler using the name and match mode
    bool matched = false;
    switch (compiler_match_ptr->mode) {
    case EXACT_MATCH:
      matched = string_matches(compiler_match_ptr->compiler_prefix, file_name);
      break;
    case PREFIX_MATCH:
      matched = string_starts_with(file_name, compiler_match_ptr->compiler_prefix);
      break;
    case REQUIRES_VERSION:
      matched = string_starts_with(file_name, compiler_match_ptr->compiler_prefix);
      if (strlen(file_name) > strlen(compiler_match_ptr->compiler_prefix)) {
        matched &= string_is_numerical(file_name + strlen(compiler_match_ptr->compiler_prefix));
      } else {
        matched = false;
      }
      break;
    }

    //Break when a match is found
    if (matched) {
      return compiler_match_ptr->type;
    }
  } 

  return UNKNOWN_COMPILER;
}
