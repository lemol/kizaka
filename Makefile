BUILD_DIR ?= ./build
CFLAGS = -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-value -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-unused-const-variable -Wno-unused-label -Wno-unused-macros -Wno-unused-result -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-value -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-unused-const-variable -Wno-unused-label -Wno-unused-macros -Wno-unused-result -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-value -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-unused-const-variable -Wno-unused-label -Wno-unused-macros -Wno-unused-result
INCLUDES = -I../raylib/src
RAYLIB_DLL_DIR = ../raylib/src
DLL_PATHS = ${RAYLIB_DLL_DIR}

ifeq ($(OS),Windows_NT)
	RAYLIB_DLL = ${RAYLIB_DLL_DIR}/raylib.dll
	LIBS = ${RAYLIB_DLL} -lm -lopengl32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -
	DLL_EXTENSION = dll
endif

ifeq ($(shell uname),Darwin)
	RAYLIB_DLL = ${RAYLIB_DLL_DIR}/libraylib.dylib
	LIBS = ${RAYLIB_DLL} -lm -framework Cocoa -framework IOKit
	export DYLB_LIBRARY_PATH := ${RAYLIB_DLL_DIR}
	DLL_EXTENSION = dylib
endif

ifeq ($(shell uname),Linux)
	RAYLIB_DLL = ${RAYLIB_DLL_DIR}/libraylib.so
	LIBS = ${RAYLIB_DLL} -lm -lpthread -ldl -lrt -lX11
	export LD_LIBRARY_PATH := ${RAYLIB_DLL_DIR}
	DLL_EXTENSION = so
endif

dev: main.c dev.c prepare-dev
	gcc -ggdb -fsanitize=address -O1 -fno-omit-frame-pointer \
		-o ${BUILD_DIR}/main-dev ${INCLUDES} -D_DEV_MODE dev.c main.c ${LIBS} && \
		${BUILD_DIR}/main-dev

dev-build: main.c prepare-dev
	gcc -ggdb -shared -o ${BUILD_DIR}/main.${DLL_EXTENSION} ${INCLUDES} main.c dev.c ${LIBS}

build: main.c prepare-dev
	gcc -o ${BUILD_DIR}/main ${INCLUDES} main.c ${LIBS}

prepare-dev:
	mkdir -p ${BUILD_DIR}
