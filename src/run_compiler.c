#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "run_compiler.h"

#include "detect_compilers.h"
#include "match_compiler.h"

static char* default_arguments_gcc[] = {NULL, "-o", "/dev/stdout", "-S", NULL};
static char* default_arguments_clang[] = {NULL, "-o", "/dev/stdout", "-S", NULL};
static char* default_arguments_unknown[] = {NULL, NULL};

static char** default_compiler_arguments[] = {
  default_arguments_gcc, //gcc and g++
  default_arguments_clang, //clang and clang++
  default_arguments_unknown //Unknown compiler
};

static const unsigned int default_compiler_arguments_length[] = {
  sizeof(default_arguments_gcc) / sizeof(char*), //gcc and g++
  sizeof(default_arguments_clang) / sizeof(char*), //clang and clang++
  sizeof(default_arguments_unknown) / sizeof(char*) //Unknown compiler
};

//Return the default arguments for a given compiler
static char** get_default_arguments(const struct compiler_info* info,
                                    char* source_path) {
  unsigned int compiler_index = 0;
  switch (info->type) {
  case GCC:
  case GXX:
    compiler_index = 0;
    break;
  case CLANG:
  case CLANGXX:
    compiler_index = 1;
    break;
  case UNKNOWN_COMPILER:
    compiler_index = 2;
    break;
  }

  //Set the compiler binary and input path
  char** default_argument_array = default_compiler_arguments[compiler_index];
  default_argument_array[0] = info->path;
  default_argument_array[default_compiler_arguments_length[compiler_index] - 1] = source_path;

  return default_argument_array;
}

//Return the number of default arguments for a given compiler
static unsigned int get_default_argument_count(enum compiler_type_enum compiler_type) {
  switch (compiler_type) {
  case GCC:
  case GXX:
    return default_compiler_arguments_length[0];
  case CLANG:
  case CLANGXX:
    return default_compiler_arguments_length[1];
  case UNKNOWN_COMPILER:
    return default_compiler_arguments_length[2];
  }

  return 0;
}

/*
 - Split the user argument string into an array of individual arguments
   - Respect backslashes and quotes
 - Return the number of elements, writing to user_arguments_array when non-NULL
*/
static unsigned int fill_user_arguments(const char* user_arguments, char** user_arguments_array) {
  unsigned int user_argument_count = 0;
  const unsigned int user_arguments_length = strlen(user_arguments);

  //Keep a buffer to build arguments
  char* element_buffer = malloc(sizeof(char) * user_arguments_length);
  unsigned int element_buffer_next = 0;

  //Track backslash, double quote and single quote states
  bool backslash_ignore = false;
  bool double_quote_ignore = false;
  bool single_quote_ignore = false;

  /*
   - Build elements from characters, saving elements on spaces
   - Backslash will interpret the next character literally
   - Double quotes and single quotes will interpret spaces literally until terminated
  */
  for (unsigned int i = 0; i < user_arguments_length; i++) {
    const char character = user_arguments[i];
    const bool ignore_special_meaning = backslash_ignore || double_quote_ignore || \
                                        single_quote_ignore;
    const bool was_backslash_ignore = backslash_ignore;
    backslash_ignore = false;

    //Handle quoted regions and spaces
    if (!ignore_special_meaning) {
      if (character == ' ') {
        //Write out the element buffer if it's not empty and a destination was given
        if (element_buffer_next != 0) {
          if (user_arguments_array != NULL) {
            user_arguments_array[user_argument_count] = strndup(element_buffer, element_buffer_next);
          }

          element_buffer_next = 0;
          user_argument_count++;
        }

        continue;
      } else if (character == '"') {
        double_quote_ignore = true;
        continue;
      } else if (character == '\'') {
        single_quote_ignore = true;
        continue;
      }
    }

    //Ignore the next character's special meaning unless it was escaped
    if (!was_backslash_ignore && character == '\\') {
      backslash_ignore = true;
      continue;
    } 

    //Handle terminating quotes
    if (double_quote_ignore && !was_backslash_ignore && character == '"') {
      double_quote_ignore = false;
      continue;
    } else if (single_quote_ignore && !was_backslash_ignore && character == '\'') {
      single_quote_ignore = false;
      continue;
    }

    //Write the character to the buffer
    element_buffer[element_buffer_next] = character;
    element_buffer_next++;
  }

  //Write out final argument, if required
  const bool ignore = backslash_ignore || double_quote_ignore || single_quote_ignore;
  if (!ignore) {
    if (element_buffer_next != 0) {
      if (user_arguments_array != NULL) {
        user_arguments_array[user_argument_count] = strndup(element_buffer, element_buffer_next);
      }

      user_argument_count++;
    }
  } else {
    //A special character sequence wasn't terminated, send a warning
    if (user_arguments_array != NULL) {
      fprintf(stderr, "Unterminated sequence in compiler options string\n");
    }
  }

  free(element_buffer);
  return user_argument_count;
}

/*
 - Allocate and fill and array with the individual arguments in a string
 - Return the array and write its length to user_argument_count
*/
static char** parse_user_arguments(const char* user_arguments, unsigned int* user_argument_count) {
  const unsigned int argument_count = fill_user_arguments(user_arguments, NULL);
  char** user_argument_array = malloc(sizeof(char*) * argument_count);
  fill_user_arguments(user_arguments, user_argument_array);

  *user_argument_count = argument_count;
  return user_argument_array;
}

//Free an argument array and its contents
static void free_argument_array(char** argument_array, unsigned int argument_count) {
  for (unsigned int i = 0; i < argument_count; i++) {
    free(argument_array[i]);
  }

  free(argument_array);
}

/*
 - Combine default arguments and user arguments into one array
 - Contained strings are pointers to the originals, so only the returned array should be freed
   - The contents should only be freed in the originals
*/
static char** combine_argument_arrays(char** default_argument_array,
                                      unsigned int default_argument_count,
                                      char** user_argument_array,
                                      unsigned int user_argument_count) {
  //Allocate space for the arguments
  const unsigned int default_size = sizeof(char*) * default_argument_count;
  const unsigned int user_size = sizeof(char*) * user_argument_count;
  char** combined_argument_array = malloc(default_size + user_size + sizeof(char*));

  //Copy the arguments in and add a NULL pointer
  memcpy(combined_argument_array, default_argument_array, default_size);
  memcpy(combined_argument_array + default_argument_count, user_argument_array, user_size);
  combined_argument_array[default_argument_count + user_argument_count] = NULL;

  return combined_argument_array;
}

//Read all of a pipe into a newly allocated buffer
static char* read_stdpipe(int pipe, const char* pipe_name) {
  //Modify the pipe to be non-blocking
  int flags = fcntl(pipe, F_GETFL, 0);
  if (flags < 0) {
    fprintf(stderr, "Failed to query file descriptor flags for %s (%d)\n", pipe_name, errno);
  }

  if (fcntl(pipe, F_SETFL, flags | O_NONBLOCK) < 0) {
    fprintf(stderr, "Failed to set file descriptor flags for %s (%d)\n", pipe_name, errno);
  }

  //Read bytes and resize output until everything has been read
  char* output = NULL;
  unsigned int output_length = 0;
  while (true) {
    char buffer[4096];

    int count = read(pipe, buffer, sizeof(buffer));
    if (count == 0 || (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
      //Finished reading
      break;
    } else if (count < 0) {
      fprintf(stderr, "Failed to read output from %s (%d)\n", pipe_name, errno);
      break;
    }

    //Resize the output buffer
    const unsigned int new_content_length = count;
    output = realloc(output, output_length + new_content_length + 1);

    //Copy the new data into the buffer
    memcpy(output + output_length, buffer, new_content_length);
    output_length += new_content_length;
  }

  output[output_length] = '\0';
  return output;
}

/*
 - Compile source_path with the specified compiler and provided user arguments
 - Returns a string of the assembly to be displayed, or any error output
*/
char* run_compiler(const struct compiler_info* compiler_infos, unsigned int compiler_index,
                   const char* user_arguments, const char* source_path) {
  //Fetch compiler information and arguments
  const struct compiler_info* info = &compiler_infos[compiler_index];
  char* source_path_copy = strdup(source_path);
  char** default_arguments_array = get_default_arguments(info, source_path_copy);
  const unsigned int default_argument_count = get_default_argument_count(info->type);
  if (default_arguments_array == NULL) {
    free(source_path_copy);
    return NULL;
  }

  //Parse the user arguments into an array
  unsigned int user_argument_count = 0;
  char** user_arguments_array = parse_user_arguments(user_arguments, &user_argument_count);

  //Combine the default arguments and user arguments
  char** combined_arguments = combine_argument_arrays(default_arguments_array,
                                                      default_argument_count,
                                                      user_arguments_array,
                                                      user_argument_count);

  //Create a pipe for stdout
  int outPipe[2];
  if (pipe(outPipe) < 0) {
    fprintf(stderr, "Failed to create pipe for stdout (%d)\n", errno);
  }

  //Create a pipe for stderr
  int errPipe[2];
  if (pipe(errPipe) < 0) {
    fprintf(stderr, "Failed to create pipe for stderr (%d)\n", errno);
  }

  //Compile the source
  pid_t pid = fork();
  bool ran_compiler = false;
  bool compiled = false;
  if (pid < 0) {
    //Failed to fork, warn about this
    fprintf(stderr, "Failed to fork (%d)\n", errno);
    ran_compiler = false;
  } else if (pid == 0) {
    //Redirect stdout
    if (dup2(outPipe[1], STDOUT_FILENO) < 0) {
      fprintf(stderr, "Failed to redirect stdout (%d)\n", errno);
      _exit(1);
    }

    //Redirect stderr
    if (dup2(errPipe[1], STDERR_FILENO) < 0) {
      fprintf(stderr, "Failed to redirect stderr (%d)\n", errno);
      _exit(1);
    }

    //Run the chosen compiler with the processed arguments
    execv(info->path, combined_arguments);
    fprintf(stderr, "Failed to exec '%s' (%d)\n", info->path, errno);
    _exit(2);
  } else {
    //Wait for the compiler to finish
    int status = 0;
    waitpid(pid, &status, 0);
    if (status == 0) {
      //Compiler ran successfully
      ran_compiler = true;
      compiled = true;
    } else if (status == 1) {
      //Compiler didn't run
      ran_compiler = false;
      compiled = false;
    } else {
      //Assume that the compiler ran but failed to compile
      ran_compiler = true;
      compiled = false;
    }
  }

  //Read the compiler's output
  char* output = NULL;
  if (ran_compiler && !compiled) {
    output = read_stdpipe(errPipe[0], "stderr");
  } else if (ran_compiler && compiled) {
    output = read_stdpipe(outPipe[0], "stdout");
  }

  //Clean up the pipes
  if (close(outPipe[0]) < 0) {
    fprintf(stderr, "Failed to close pipe for stdout (%d)\n", errno);
  }

  if (close(errPipe[0]) < 0) {
    fprintf(stderr, "Failed to close pipe for stderr (%d)\n", errno);
  }

  //Free the processed arguments
  free(source_path_copy);
  free(combined_arguments);
  free_argument_array(user_arguments_array, user_argument_count);

  return output;
}
