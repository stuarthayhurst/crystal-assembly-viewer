#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "detect_compilers.h"

#include "match_compiler.h"

//Find the next empty element to write to, handling NULL
static char** get_next_slot(char** compiler_array, unsigned int detected_compiler_count) {
  char** compiler_array_next = compiler_array + detected_compiler_count;
  if (compiler_array == NULL) {
    compiler_array_next = NULL;
  }

  return compiler_array_next;
}

struct file_id {
  ino_t inode;
  dev_t device;
};

//Fill a structure with enough information to uniquely identify a file
static bool get_file_id(const char* path, struct file_id* id) {
  struct stat stat_buf;
  if (stat(path, &stat_buf) == -1) {
    fprintf(stderr, "Failed to stat '%s'\n", path);

    return false;
  }

  id->inode = stat_buf.st_dev;
  id->device = stat_buf.st_ino;

  return true;
}

static bool file_id_matches(const struct file_id* a, const struct file_id* b) {
  return (a->inode == b->inode) && (a->device == b->device);
}

/*
 - Return true if the path is a component of itself
 - Don't check the first ignored_path_length characters
   - This avoids excluding symlinks that link to a component of the base path
*/
static bool is_path_loop(const char* directory_path, unsigned int ignored_path_length) {
  //Get the file ID of the final component
  struct file_id final_id;
  get_file_id(directory_path, &final_id);

  const unsigned int directory_length = strlen(directory_path);
  char* string_buffer = malloc(directory_length + 1);
  memcpy(string_buffer, directory_path, directory_length + 1);

  /*
   - Compare the file ID of each path component to the final component's ID
   - Work from right to left to save string copies
   - If a match is found, the path contains a loop
  */
  for (unsigned int i = 0; i < directory_length; i++) {
    //Ignore an empty path
    if (directory_length - (i + 1) <= ignored_path_length) {
      break;
    }

    //If the remaining path is a component, check it
    if (directory_path[directory_length - (i + 1)] == '/') {
      string_buffer[directory_length - (i + 1)] = '\0';

      struct file_id segment_id;
      get_file_id(string_buffer, &segment_id);

      if (file_id_matches(&final_id, &segment_id)) {
        free(string_buffer);
        return true;
      }
    }
  }

  free(string_buffer);
  return false;
}

//Return the length of the non-final components
static unsigned int get_base_path_length(const char* path) {
  const unsigned int path_length = strlen(path);
  unsigned int base_path_length = path_length;

  //Strip terminating /
  for (unsigned int i = 0; i < path_length; i++) {
    if (path[(path_length - 1) - i] == '/') {
      base_path_length--;
    } else {
      break;
    }
  }

  //Strip the final component
  for (int i = base_path_length - 1; i >= 0; i--) {
    bool is_slash = (path[i] == '/');
    base_path_length--;

    if (is_slash) {
      break;
    }
  }

  return base_path_length;
}

/*
 - Recursively find all compilers in directory_path
 - Store them in compile_array, if it's not NULL
 - Returns the number of compilers found
*/
static unsigned int search_compiler_directory(char** compiler_array,
                                              const char* directory_path,
                                              unsigned int ignored_path_length) {
  const unsigned int path_length = strlen(directory_path);
  unsigned int detected_compiler_count = 0;

  //Open the directory
  DIR* dir_descriptor = opendir(directory_path);
  if (dir_descriptor == NULL) {
    fprintf(stderr, "Failed to open '%s'\n", directory_path);
    return 0;
  }

  //Recursively traverse the directory, looking for compilers
  struct dirent* dirent_ptr = readdir(dir_descriptor);
  while (dirent_ptr != NULL) {
    struct stat stat_buf;

    const unsigned int child_path_size = sizeof(char) * (path_length + strlen(dirent_ptr->d_name) + 2);
    char* child_path = malloc(child_path_size);

    //Get information on the directory's child
    sprintf(child_path, "%s/%s", directory_path, dirent_ptr->d_name);
    if (stat(child_path, &stat_buf) == -1) {
      fprintf(stderr, "Failed to stat file '%s'\n", child_path);

      free(child_path);
      continue;
    }

    const bool is_directory = S_ISDIR(stat_buf.st_mode);
    const bool is_file = S_ISREG(stat_buf.st_mode) || S_ISLNK(stat_buf.st_mode);

    //Recurse in directories, attempt to identify compilers from file paths
    if (is_directory) {
      //Skip itself and the previous directory
      if (strcmp(".", dirent_ptr->d_name) == 0 || strcmp("..", dirent_ptr->d_name) == 0) {
        dirent_ptr = readdir(dir_descriptor);
        free(child_path);
        continue;
      }

      //Ignore symlinks causing loops
      if (is_path_loop(child_path, ignored_path_length)) {
        dirent_ptr = readdir(dir_descriptor);
        free(child_path);
        continue;
      }

      //Find the next empty element to write to, then search the directory
      char** compiler_array_next = get_next_slot(compiler_array, detected_compiler_count);
      detected_compiler_count += search_compiler_directory(compiler_array_next, child_path,
                                                           ignored_path_length);
    } else if (is_file) {
      //Save the path if it's a known compiler
      enum compiler_type_enum compiler_type = identify_compiler(child_path);
      if (compiler_type != UNKNOWN_COMPILER) {
        //Find the next empty element to write to
        char** compiler_array_next = get_next_slot(compiler_array, detected_compiler_count);

        //Copy the string to the array
        if (compiler_array_next != NULL) {
          *compiler_array_next = malloc(child_path_size);
          memcpy(*compiler_array_next, child_path, child_path_size);
        }
        detected_compiler_count++;
      }
    }

    dirent_ptr = readdir(dir_descriptor);
    free(child_path);
  }

  closedir(dir_descriptor);
  return detected_compiler_count;
}

/*
 - Fill compiler_array with paths to compiler binaries, return the number of written entries
 - If compiler_array is NULL, just return the number of entries found
*/
static unsigned int fill_compiler_array(char** compiler_array, const char* search_path_var) {
  unsigned int detected_compiler_count = 0;

  //Search all directories in PATH, delimited by a colon or string end
  unsigned int path_start = 0;
  unsigned int path_end = 0;
  bool is_string_end = (search_path_var[path_end] == '\0');
  while (!is_string_end) {
    is_string_end = (search_path_var[path_end] == '\0');
    const bool is_component_end = is_string_end || search_path_var[path_end] == ':';

    //Search the path, or ignore it if it's escaped
    if (is_component_end) {
      const unsigned int path_length = path_end - path_start;
      if (path_length > 0 && search_path_var[path_end - 1] == '\\') {
        continue;
      } else {
        //Find the next empty element to write
        char** compiler_array_next = get_next_slot(compiler_array, detected_compiler_count);

        //Find the compilers in the path directory
        if (path_length > 1) {
          //Copy the path to a null-terminated string
          char* directory_path = malloc(sizeof(char) * (path_length + 1));
          memcpy(directory_path, search_path_var + path_start, path_length);
          directory_path[path_length] = '\0';

          //Search the path directory
          unsigned int ignored_path_length = get_base_path_length(directory_path);
          detected_compiler_count += search_compiler_directory(compiler_array_next,
                                                               directory_path, ignored_path_length);
          free(directory_path);
        }

        //Mark the start of the next substring
        path_start = path_end + 1;
      }
    }

    path_end++;
  }

  return detected_compiler_count;
}

/*
 - Return an array containing the paths for all compilers found in PATH
 - Write the number of entries to compiler_count
*/
static char** detect_compilers(unsigned int* compiler_count) {
  //Get the PATH from the environment
  const char* search_path_var = getenv("PATH");
  if (search_path_var == NULL) {
    fprintf(stderr, "PATH is unset, failed to find any compilers");

    *compiler_count = 0;
    return NULL;
  }

  //Find the number of compilers available, allocate space and return them
  *compiler_count = fill_compiler_array(NULL, search_path_var);
  char** compiler_array = malloc(sizeof(char*) * *compiler_count);
  fill_compiler_array(compiler_array, search_path_var);

  return compiler_array;
}

/*
 - Fill unique_compiler_array with the unique entries of compiler_array, by file ID
 - Return the number of unique entries
 - If unique_compiler_array is NULL, just return the number of unique entries
*/
static unsigned int fill_unique_compilers(char** compiler_array, unsigned int compiler_count,
                                          char** unique_compiler_array) {
  unsigned int unique_compiler_count = 0;
  for (unsigned int i = 0; i < compiler_count; i++) {
    //Get the file ID of the compiler to check for duplicates of
    struct file_id compiler_file_id;
    get_file_id(compiler_array[i], &compiler_file_id);

    /*
     - Look for duplicates in the previous entries
     - Future entries will be checked when it's their turn in the outer loop
    */
    bool matched = false;
    for (unsigned int j = 0; j < i; j++) {
      //Find the file ID of the potential match
      struct file_id nested_compiler_file_id;
      get_file_id(compiler_array[j], &nested_compiler_file_id);

      //Check if the files match for a duplicate
      if (file_id_matches(&compiler_file_id, &nested_compiler_file_id)) {
        matched = true;
        break;
      }
    }

    //If the compiler was unique so far, include it
    if (!matched) {
      if (unique_compiler_array != NULL) {
        //Allocate space for the compiler and copy it
        const unsigned int compiler_size = strlen(compiler_array[i]) + 1;
        unique_compiler_array[unique_compiler_count] = malloc(sizeof(char) * compiler_size);
        memcpy(unique_compiler_array[unique_compiler_count], compiler_array[i], compiler_size);
      }

      unique_compiler_count++;
    }
  }

  return unique_compiler_count;
}

/*
 - Return a new array containing the unique entries of compiler_array, by file ID
 - Write the number of unique entries to unique_compiler_count
*/
char** select_unique_compilers(char** compiler_array, unsigned int compiler_count,
                               unsigned int* unique_compiler_count) {
  //Allocate space for the unique entries
  *unique_compiler_count = fill_unique_compilers(compiler_array, compiler_count, NULL);
  char** unique_compiler_array = malloc(sizeof(char*) * *unique_compiler_count);

  //Fill the array and return it
  fill_unique_compilers(compiler_array, compiler_count, unique_compiler_array);
  return unique_compiler_array;
}

/*
 - Return an array containing the paths for all unique compilers found in PATH
 - Write the number of entries to compiler_count
*/
char** detect_unique_compilers(unsigned int* compiler_count) {
  unsigned int all_compilers_count = 0;
  char** compiler_array = detect_compilers(&all_compilers_count);

  //Create a new array with the unique entries
  char** unique_compiler_array = select_unique_compilers(compiler_array, all_compilers_count,
                                                         compiler_count);

  //Free the original array
  free_compiler_array(compiler_array, all_compilers_count);

  return unique_compiler_array;
}

void free_compiler_array(char** compiler_paths, unsigned int compiler_count) {
  for (unsigned int i = 0; i < compiler_count; i++) {
    free(compiler_paths[i]);
  }

  free(compiler_paths);
}
