cmake_minimum_required(VERSION 3.10)
project(Main VERSION 1.0)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

# Find libs
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)
pkg_check_modules(CJSON REQUIRED libcjson)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Define the executable
add_executable(main main.c)

# Apply flags
target_include_directories(main
  PRIVATE ${FUSE3_INCLUDE_DIRS}
  PRIVATE ${CJSON_INCLUDE_DIRS}
  PRIVATE ${GLIB_INCLUDE_DIRS}
)

target_compile_options(main
  PRIVATE ${FUSE3_CFLAGS_OTHER}
  PRIVATE ${CJSON_CFLAGS_OTHER}
  PRIVATE ${GLIB_CFLAGS_OTHER}
)

# Link libs
target_link_libraries(main
  PRIVATE ${FUSE3_LIBRARIES}
  PRIVATE ${CJSON_LIBRARIES}
  PRIVATE ${GLIB_LIBRARIES}
)
