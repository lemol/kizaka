#include <dlfcn.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef DEV_MODE
#define DEV_MODE
#endif

#include "dev.h"

// UTILS

static long get_last_modified(const char *name) {
  struct stat s;
  stat(name, &s);

#ifdef OS_MAC
  return s.st_mtimespec.tv_sec;
#endif

#ifdef OS_LINUX
  return s.st_mtim.tv_sec;
#endif
}

// SOURCE FILES

typedef struct {
  const char *name;
  long version;
  long previous_version;
} SourceFile;

typedef struct {
  SourceFile *files;
  size_t length;
} SourceFiles;

SourceFile *init_source_files(const char *paths[], size_t length) {
  SourceFile *files = malloc(length * sizeof(SourceFile));

  for (int i = 0; i < length; i++) {
    files[i].name = paths[i];
    files[i].version = 0;
    files[i].previous_version = 0;
  }

  return files;
}

bool should_rebuild(SourceFiles *source, int *changed_index,
                    long *new_last_modified) {
  SourceFile *files = source->files;

  for (int i = 0; i < source->length; i++) {
    *new_last_modified = get_last_modified(files[i].name);

    if (*new_last_modified == files[i].version) {
      continue;
    }

    *changed_index = i;

    return true;
  }

  return false;
}

void rebuild() {
  printf("Rebuilding with 'make build'\n");
  system("make dev-build");
}

// HOT RELOAD

#define MAX_HOT_RELOADABLE_FUNCTIONS 10

typedef struct {
  const char *name;
  void *function;
} Function;

typedef struct {
  const char *name;
  void *handle;
  long version;
  long previous_version;
  size_t function_count;
  Function functions[MAX_HOT_RELOADABLE_FUNCTIONS];
} Dll;

bool should_hot_reload(Dll *dll, long *new_last_modified) {
  *new_last_modified = get_last_modified(dll->name);

  if (*new_last_modified == dll->version) {
    return false;
  }

  return true;
}

char *duplicate_dll_file(Dll *dll, long version) {
  char fpath[FILENAME_MAX];
  sprintf(fpath, "%s.%ld", dll->name, version);

  char cmd[FILENAME_MAX * 2 + 64];
  sprintf(cmd, "cp %s %s", dll->name, fpath);

  printf("Copying %s to %s\n", dll->name, fpath);
  system(cmd);

  return strdup(fpath);
}

void rm_dll_file(const char *name, long version) {
  char fpath[FILENAME_MAX];
  sprintf(fpath, "%s.%ld", name, version);

  char cmd[FILENAME_MAX + 32];
  sprintf(cmd, "rm %s", fpath);

  printf("Removing previous version %s\n", fpath);
  system(cmd);
}

void hot_reload(Dll *dll) {
  char *new_dll_name = duplicate_dll_file(dll, dll->version);

  if (dll->handle) {
#ifndef OS_LINUX
    dlclose(dll->handle);
#else
// WTF this is not working on Linux????
#endif

    rm_dll_file(dll->name, dll->previous_version);
  }

  dll->handle = dlopen(new_dll_name, RTLD_NOW);
  if (!dll->handle) {
    fprintf(stderr, "dlopen: %s\n", dlerror());
    return;
  }

  for (int i = 0; i < dll->function_count; i++) {
    Function f = dll->functions[i];
    printf("Reloading function %s\n", f.name);
    void *ptr = dlsym(dll->handle, f.name);

    if (!ptr) {
      fprintf(stderr, "dlsym: %s\n", dlerror());
      return;
    }

    *((void **)f.function) = ptr;
  }

  printf("Hot reload successful\n");
}

// DEV

enum NextAction { IDLE, REBUILD, HOT_RELOAD };

typedef struct {
  SourceFiles *source;
  Dll *dll;
  pthread_t watch_thread;
  enum NextAction action;
} Dev;

static Dev *dev_instance = {0};

void dev_update(Dev *dev) {
  int changed_index;
  long new_last_modified;

  if (should_rebuild(dev->source, &changed_index, &new_last_modified)) {
    SourceFile *changed_file = &dev->source->files[changed_index];

    if (changed_file->version != 0) {
      printf("Source %s changed\n", changed_file->name);
      dev->action = REBUILD;
    }

    changed_file->previous_version = changed_file->version;
    changed_file->version = new_last_modified;
    return;
  }

  if (should_hot_reload(dev->dll, &new_last_modified)) {
    if (dev->dll->version != 0) {
      dev->action = HOT_RELOAD;
    }

    dev->dll->previous_version = dev->dll->version;
    dev->dll->version = new_last_modified;

    return;
  }
}

void dev_watch_update(Dev *dev) {
  while (true) {
    dev_update(dev);

    if (dev->action == REBUILD) {
      dev->action = IDLE;
      rebuild();
    }

    sleep(1);
  }
}

void dev_check_watch(Dev *dev) {
  if (dev->action != HOT_RELOAD) {
    return;
  }

  dev->action = IDLE;

  printf("Hot reloading\n");
  hot_reload(dev->dll);
}

void dev_create_watch_thread(Dev *dev, pthread_t *watch_thread) {
  if (pthread_create(watch_thread, NULL, (void *)dev_watch_update, dev) != 0) {
    fprintf(stderr, "Error creating watch thread\n");
    exit(1);
  }
}

void dev_instance_start(const char *source_paths[], char *dll_name,
                        size_t source_length) {
  SourceFile *files = init_source_files(source_paths, source_length);

  Dll *dll = (Dll *)malloc(sizeof(Dll));
  dll->name = dll_name;
  dll->function_count = 0;
  dll->version = 0;
  dll->previous_version = 0;
  dev_instance = (Dev *)malloc(sizeof(Dev));
  dev_instance->source = (SourceFiles *)malloc(sizeof(SourceFiles));
  dev_instance->source->files = files;
  dev_instance->source->length = source_length;
  dev_instance->dll = dll;
  dev_instance->action = IDLE;

  pthread_t watch_thread;
  dev_create_watch_thread(dev_instance, &watch_thread);
  dev_instance->watch_thread = watch_thread;

  printf("Dev instance started\n");
}

void dev_instance_close() {
  printf("Closing dev instance\n");

  if (dev_instance->dll->handle) {
    dlclose(dev_instance->dll->handle);
  }

  if (dev_instance->dll->version != 0) {
    rm_dll_file(dev_instance->dll->name, dev_instance->dll->version);
  }

  if (dev_instance->watch_thread) {
    pthread_cancel(dev_instance->watch_thread);
  }

  free(dev_instance->dll);
  free(dev_instance->source->files);
  free(dev_instance->source);
  free(dev_instance);

  printf("Dev instance closed\n");
}

void dev_hot_reload_here() { dev_check_watch(dev_instance); }

void dev_hot_reload_define(char *name, void *fptr) {
  printf("Registering function %s\n", name);

  Dll *dll = dev_instance->dll;
  dll->functions[dll->function_count++] = (Function){name, fptr};
}
