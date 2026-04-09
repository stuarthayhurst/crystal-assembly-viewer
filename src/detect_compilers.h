#ifndef CRYSTAL_DETECT_COMPILERS
#define CRYSTAL_DETECT_COMPILERS

char** detect_unique_compilers(unsigned int* compiler_count);

void free_compiler_array(char** compiler_paths, unsigned int compiler_count);

#endif
