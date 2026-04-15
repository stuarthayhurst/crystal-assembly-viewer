#ifndef CRYSTAL_RUN_COMPILER
#define CRYSTAL_RUN_COMPILER

#include "detect_compilers.h"
#include "match_compiler.h"

char* run_compiler(const struct compiler_info* compiler_infos, unsigned int compiler_index,
                   const char* user_arguments, const char* source_path, bool* success);

#endif
