#ifndef _DEV_H
#define _DEV_H

#include <stdlib.h>

#if defined(__APPLE__) || defined(__MACH__)
#define OS_MACOS
#endif

#if defined(__linux__)
#define OS_LINUX
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define OS_WINDOWS
#endif

#ifdef OS_MACOS
#define DLL_NAME(name) #name ".dylib"
#endif

#ifdef OS_LINUX
#define DLL_NAME(name) name ".so"
#endif

#ifdef OS_WINDOWS
#define DLL_NAME(name) #name ".dll"
#endif

#define NOOP (void)0
#define SOURCE_PATHS(...)                                                      \
  (const char *[]) { __VA_ARGS__ }

#ifndef _DEV_MODE
#define DEV_START(source_paths, dll_name, source_length) NOOP

#define DEV_HOT_RELOAD_HERE() NOOP

#define DEV_HOT_RELOAD_DEFINE(r, f, ...) NOOP

#define DEV_WITH_HOT_RELOAD(f) f

#define DEV_CLOSE() NOOP
#else
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
#endif

#endif // !_DEV_H
