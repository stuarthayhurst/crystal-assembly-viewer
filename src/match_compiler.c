#include <stdlib.h>
#include <string.h>

#include "match_compiler.h"

#include "string_utils.h"

enum match_enum {
  //Only matches files with an identical name
  EXACT_MATCH,

  //Matches any file that starts with the compiler's name
  PREFIX_MATCH,

  //Matches any file that ends with the compiler's name
  SUFFIX_MATCH,

  //Matches any file that starts with the compiler's name, and is immediately followed by a version 
  REQUIRES_VERSION,

  //Matches any file ends starts with the compiler's name and a version
  ENDS_REQUIRES_VERSION
};

struct compiler_match {
  const char* compiler_string;
  enum match_enum mode;
  enum compiler_type_enum type;
  unsigned int priority;
};

/*
 - Matching rules for various compilers
 - Lower numbers have higher priorities
 - Priorities:
   - 0: Explicitly versioned compilers
   - 1: Explicitly versioned compiler with long form / alternative names
   - 2: Unversioned exact matches
   - 3: Unversioned exact matches with long form / alternative names
   - 4: Unknown compiler
*/
#define UNKNOWN_PRIORITY 4
static struct compiler_match compiler_match_info[12] = {
  {"gcc", EXACT_MATCH, GCC, 2}, {"gcc-", REQUIRES_VERSION, GCC, 0},
  {"g++", EXACT_MATCH, GXX, 2}, {"g++-", REQUIRES_VERSION, GXX, 0},
  {"-linux-gnu-gcc", SUFFIX_MATCH, GCC, 3}, {"-linux-gnu-gcc-", ENDS_REQUIRES_VERSION, GCC, 1},
  {"-linux-gnu-g++", SUFFIX_MATCH, GXX, 3}, {"-linux-gnu-g++-", ENDS_REQUIRES_VERSION, GXX, 1},
  {"clang", EXACT_MATCH, CLANG, 2}, {"clang-", REQUIRES_VERSION, CLANG, 0},
  {"clang++", EXACT_MATCH, CLANGXX, 3}, {"clang++-", REQUIRES_VERSION, CLANGXX, 1}
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

//Remove any terminating digits from a string, return true if any digits were removed
static bool zero_terminating_digits(char* string) {
  bool has_removed_digit = false;

  //Remove terminating digits until none remain
  const unsigned int path_length = strlen(string);
  for (int i = path_length - 1; i >= 0; i--) {
    if (string[i] >= '0' && string[i] <= '9') {
      string[i] = '\0';
      has_removed_digit = true;
    } else {
      break;
    }
  }

  return has_removed_digit;
}

/*
 - Use matching rules in compiler_match_info to identify a compiler based on its path
 - Returns the enum for the compiler if identified, UNKNOWN_COMPILER otherwise
   - Also returns the priority of the match
*/
struct compiler_match_data identify_compiler(const char* file_path) {
  //Extract the filename from the path
  const char* file_name = strrchr(file_path, '/') + 1;

  //Try and match each known compiler sequentially
  for (unsigned int i = 0; i < compiler_match_count; i++) {
    const struct compiler_match* compiler_match_ptr = &compiler_match_info[i];

    //Attempt to match the compiler using the name and match mode
    bool matched = false;
    switch (compiler_match_ptr->mode) {
    case EXACT_MATCH:
      matched = string_matches(compiler_match_ptr->compiler_string, file_name);
      break;
    case PREFIX_MATCH:
      matched = string_starts_with(file_name, compiler_match_ptr->compiler_string);
      break;
    case SUFFIX_MATCH:
      matched = string_ends_with(file_name, compiler_match_ptr->compiler_string);
      break;
    case REQUIRES_VERSION:
      matched = string_starts_with(file_name, compiler_match_ptr->compiler_string);
      if (strlen(file_name) > strlen(compiler_match_ptr->compiler_string)) {
        matched &= string_is_numerical(file_name + strlen(compiler_match_ptr->compiler_string));
      } else {
        matched = false;
      }
      break;
    case ENDS_REQUIRES_VERSION:
      //Check for terminating version
      char* file_name_copy = strdup(file_name);
      if (!zero_terminating_digits(file_name_copy)) {
        matched = false;
        free(file_name_copy);
        break;
      }

      matched = string_ends_with(file_name_copy, compiler_match_ptr->compiler_string);
      free(file_name_copy);
      break;
    }

    //Break when a match is found
    if (matched) {
      struct compiler_match_data match_data = {compiler_match_ptr->type,
                                               compiler_match_ptr->priority};
      return match_data;
    }
  } 

  struct compiler_match_data unknown_match_data = {UNKNOWN_COMPILER, UNKNOWN_PRIORITY};
  return unknown_match_data;
}
