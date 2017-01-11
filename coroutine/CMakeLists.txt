# cmake最低版本号
cmake_minimum_required (VERSION 2.8)
# 项目名字
project (coroutine)


# 设置C++编译器
set(CMAKE_CXX_COMPILER "g++")
# 输出编译选项
set(CMAKE_VERBOSE_MAKEFILE on)
#设置编译版本类型
set(CMAKE_BUILD_TYPE "Debug")
#设置C++ 通用 版本编译选项
set(CMAKE_CXX_FLAGS "-g -rdynamic -Wall -Wno-reorder -Wno-literal-suffix -finput-charset=gbk -fPIC --std=c++11")
#设置C++ Debug 版本编译选项
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -D _DEBUG")
#设置C++ Release 版本编译选项
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

aux_source_directory(. LIST_SRCS)

enable_language(C ASM)

add_executable(main context_linux.S ${LIST_SRCS})
target_link_libraries(main pthread)