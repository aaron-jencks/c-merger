// Created by Aaron Jencks May 17th, 2023

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#include <stddef.h>

typedef struct {
  void** arr;
  size_t capacity;
  size_t count;
} arraylist_t;

arraylist_t create_arraylist(size_t capacity);

void destroy_arraylist(arraylist_t arr);

size_t arraylist_append(arraylist_t* arr, void* item);

void* arraylist_pop(arraylist_t* arr);

#endif
#include <stdlib.h>


#ifndef ERROR_H
#define ERROR_H

void handle_error(int status, char* fmt, ...);

void handle_memory_error(void* ptr);

#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>



void handle_error(int status, char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  if(status) exit(status);
}

void handle_memory_error(void* ptr) {
  if(!ptr) handle_error(1, "out of memory\n");
}



arraylist_t create_arraylist(size_t capacity) {
  arraylist_t result;
  result.count = 0;
  result.capacity = capacity;
  result.arr = (void**)malloc(sizeof(void*) * capacity);
  handle_memory_error(result.arr);
  return result;
}

void destroy_arraylist(arraylist_t arr) {
  free(arr.arr);
}

size_t arraylist_append(arraylist_t* arr, void* item) {
  if(arr->count == arr->capacity) {
    arr->capacity <<= 1;
    arr->arr = (void**)realloc(arr->arr, sizeof(void*) * arr->capacity);
    handle_memory_error(arr->arr);
  }
  arr->arr[arr->count] = item;
  return arr->count++;
}

void* arraylist_pop(arraylist_t* arr) {
  if(!arr->count) return NULL;
  return arr->arr[--arr->count];
}




typedef struct {
  char* fname;
  char* contents;
  size_t clength;
  char* dir;
} file_t;

typedef struct {
  file_t source;
  size_t start;
  size_t stop;
} include_t;

// need a way to determine if we've seen a c file before
// or a header file
// if we have, then ignore this one because it's been inserted beforehand

arraylist_t visited_files;

bool file_is_header(file_t file) {
  size_t nlen = strlen(file.fname);
  return file.fname[nlen-2] == '.' && file.fname[nlen-1] == 'h';
}

char* read_file_contents(char* fname) {
  fprintf(stderr, "Reading file contents of %s\n", fname);

  FILE* fp = fopen(fname, "r");
  int fsize;
  char* text;

  fseek(fp, 0, SEEK_END);
  fsize = ftell(fp);
  rewind(fp);

  text = (char*)malloc(fsize+1);
  handle_memory_error(text);
  fread(text, 1, fsize, fp);
  text[fsize] = 0;

  fclose(fp);

  return text;
}

file_t create_source_file(file_t header) {
  file_t result;
  char* fnc = strdup(header.fname);

  handle_memory_error(fnc);
  result.fname = fnc;
  size_t flen = strlen(fnc);
  result.fname[flen-1] = 'c';

  // get the absolute dirname
  result.dir = strdup(header.dir);
  handle_memory_error(result.dir);

  // get the file contents
  size_t nflen = strlen(result.fname) + strlen(result.dir) + 2;
  fnc = (char*)malloc(sizeof(char) * nflen);
  handle_memory_error(fnc);
  fnc[nflen-1] = 0;
  sprintf(fnc, "%s/%s", result.dir, result.fname);
  if(!access(fnc, F_OK)) {
    result.contents = read_file_contents(fnc);
    result.clength = strlen(result.contents);
  } else {
    printf("Failed to find source file for %s, skipping\n", result.fname);
    result.contents = 0;
    result.clength = 0;
  }

  return result;
}

file_t create_file(char* fname) {
  file_t result;
  char* fnc = strdup(fname);

  // get the basename
  handle_memory_error(fnc);
  result.fname = strdup(basename(fnc));
  handle_memory_error(result.fname);
  free(fnc);

  // get the absolute dirname
  fnc = strdup(fname);
  handle_memory_error(fnc);
  result.dir = strdup(dirname(fnc));
  handle_memory_error(result.dir);
  free(fnc);
  fnc = result.dir;
  result.dir = realpath(fnc, NULL);
  handle_memory_error(result.dir);
  free(fnc);

  // get the file contents
  result.contents = read_file_contents(fname);
  result.clength = strlen(result.contents);

  return result;
}

include_t create_include(file_t source, size_t start, size_t stop) {
  return (include_t){source, start, stop};
}

bool file_is_visited(file_t file) {
  for(size_t fi = 0; fi < visited_files.count; fi++) {
    file_t tf = *(file_t*)visited_files.arr[fi];
    if(!(strcmp(file.fname, tf.fname) || strcmp(file.dir, tf.dir))) {
      return true;
    }
  }
  return false;
}

void visit_file(file_t file) {
  file_t* fp = (file_t*)malloc(sizeof(file_t));
  handle_memory_error(fp);
  *fp = file;
  arraylist_append(&visited_files, fp);
}

arraylist_t find_file_includes(file_t file) {
  arraylist_t result = create_arraylist(10);

  char* template = "#include\"";
  size_t ti = 0;
  bool nl = true;
  size_t line = 1;

  for(size_t ci = 0; ci < file.clength; ci++) {
    char c = file.contents[ci];

    if(c == ' ' || c == '\t') continue;

    if((nl || ti) && c == template[ti]) {
      if(nl) nl = false;

      if(++ti == 9) {
        // we found an include
        size_t fstart = ++ci;
        while(file.contents[ci++] != '"');
        size_t fend = ci-1;
        size_t nlen = fend-fstart;

        char* fname = (char*)malloc(sizeof(char) * (nlen+1));
        handle_memory_error(fname);

        fname = memcpy(fname, file.contents+fstart, nlen);
        fname[nlen] = 0;

        printf("Found an include for %s on line %d\n",
          fname, line++);

        include_t* fp = (include_t*)malloc(sizeof(include_t));
        handle_memory_error(fp);
        *fp = create_include(create_file(fname),
          fstart-10, fend+1);
        arraylist_append(&result, fp);
        free(fname);

        while(file.contents[ci++] != '\n');
        ci -= 2;
        nl = true;
      }

      continue;
    } else if (ti) {
      // so that we don't continue to increment it
      ti = 0;
    }

    if(c == '\n') {
      line++;
      nl = true;
      ti = 0;
      continue;
    }

    nl = false;
  }

  return result;
}

void move_to_dir(char* dir) {
  if(chdir(dir)) {
    handle_error(errno,
      "changing to directory %s failed with code %d\n",
      dir, errno);
  }
}

typedef struct {
  file_t file;
  size_t start;
  size_t stop;
} span_t;

span_t* create_span_ptr_range(file_t file,
  size_t start, size_t stop) {

  span_t* result =
    (span_t*)malloc(sizeof(span_t));
  handle_memory_error(result);

  result->file = file;
  result->start = start;
  result->stop = stop;

  return result;
}

void create_merged_spans(arraylist_t* output, file_t current) {
  move_to_dir(current.dir);
  arraylist_t includes = find_file_includes(current);
  visit_file(current);

  size_t parent_start = 0;
  for(size_t ni = 0; ni < includes.count; ni++) {
    include_t* inc = (include_t*)includes.arr[ni];

    arraylist_append(output, create_span_ptr_range(
      current, parent_start, inc->start));

    if(file_is_visited(inc->source)) {
      parent_start = inc->stop;
      free(inc);
      continue;
    }

    create_merged_spans(output, inc->source);

    if(file_is_header(inc->source)) {
      file_t source = create_source_file(inc->source);
      if(source.clength) create_merged_spans(output, source);
    }

    parent_start = inc->stop;

    free(inc);
  }

  arraylist_append(output, create_span_ptr_range(
    current, parent_start, current.clength));

  destroy_arraylist(includes);
}

void display_span(span_t s) {
  printf("Span for section of %s (%d,%d)\n", s.file.fname, s.start, s.stop);
}

int main(int argc, char* argv[]) {
  if(argc != 3) {
    // incorrect usage given
    printf("merger\nMerges multiple C files into one\nUsage: merger main out\nfile\tA C file with a main() to merge #include's into\nout\tThe output filename to put all of the merged files into\n");
    return 0;
  }
  visited_files = create_arraylist(10);

  arraylist_t output = create_arraylist(100);
  file_t start = create_file(argv[1]);

  FILE* ofp = fopen(argv[2], "w+");

  create_merged_spans(&output, start);

  while(visited_files.count) {
    free(arraylist_pop(&visited_files));
  }
  destroy_arraylist(visited_files);

  printf("Writing output to %s\n", argv[2]);

  for(size_t oi = 0; oi < output.count; oi++) {
    span_t* span = (span_t*)output.arr[oi];
    display_span(*span);
    for(size_t offset = span->start; offset < span->stop; offset++) {
      fputc(span->file.contents[offset], ofp);
    }
    free(span);
  }

  destroy_arraylist(output);

  fclose(ofp);

  printf("Finished generating merged file\n");

  return 0;
}
