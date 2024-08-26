BUILD_DIR ?= ./build
CFLAGS = -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-value -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-unused-const-variable -Wno-unused-label -Wno-unused-macros -Wno-unused-result -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-value -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-unused-const-variable -Wno-unused-label -Wno-unused-macros -Wno-unused-result -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-value -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-unused-const-variable -Wno-unused-label -Wno-unused-macros -Wno-unused-result
INCLUDES = -I../raylib/src -I../glfw/include
RAYLIB_DLL_DIR = ../raylib/src
RAYLIB_DLL = ${RAYLIB_DLL_DIR}/libraylib.dylib
LIBS = ${RAYLIB_DLL} -L../glfw/build/src -lm -lglfw -framework Cocoa -framework IOKit
DYLD_LIBRARY_PATH = ${RAYLIB_DLL_DIR}

dev: main.c dev.c prepare-dev
	gcc -o ${BUILD_DIR}/main-dev ${INCLUDES} -DDEV_MODE dev.c main.c ${LIBS} && \
		DYLD_LIBRARY_PATH=${RAYLIB_DLL_DIR} ./${BUILD_DIR}/main-dev

dev-build: main.c prepare-dev
	gcc -shared -o ${BUILD_DIR}/main.dylib ${INCLUDES} main.c dev.c ${LIBS}

build: main.c prepare-dev
	gcc -o ${BUILD_DIR}/main ${INCLUDES} main.c ${LIBS}

prepare-dev:
	mkdir -p ${BUILD_DIR}
