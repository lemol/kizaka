#ifndef _DEV_H
#define _DEV_H

#include <stdlib.h>

void dev_instance_start(const char *source_paths[], char *dll_name,
                        size_t source_length);

void dev_hot_reload_here();

void dev_hot_reload_define(char *name, void *fptr);

void dev_instance_close();

#define DEV_START(source_paths, dll_name, source_length)                       \
  dev_instance_start(source_paths, dll_name, source_length)

#define DEV_HOT_RELOAD_HERE() dev_hot_reload_here()

#define DEV_HOT_RELOAD_DEFINE(r, f, ...)                                       \
  r (*_dev_hot_reloadable_##f)(__VA_ARGS__) = f;                               \
  do {                                                                         \
    dev_hot_reload_define(#f, &_dev_hot_reloadable_##f);                       \
  } while (0)

#define DEV_WITH_HOT_RELOAD(f) _dev_hot_reloadable_##f

#define DEV_CLOSE() dev_instance_close()

#define SOURCE_PATHS(...)                                                      \
  (const char *[]) { __VA_ARGS__ }

#endif // !_DEV_H
