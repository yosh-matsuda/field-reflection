cmake_minimum_required(VERSION 3.16)

#
# COMPILER/LINKER FLAGS
#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ENV{CMAKE_EXPORT_COMPILE_COMMANDS} ON)

if(UNIX)
  # Warnings
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wcast-align -Wcast-qual -Wdelete-non-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wpointer-arith -Winit-self -Wredundant-decls -Wsign-promo -Wswitch-default -Wswitch-enum -Wvariadic-macros -Wwrite-strings -Wno-unknown-pragmas"
  )

  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow=local -Wno-pragmas")
  endif()
else() # WIN32
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# O3 optimization
if(NOT MSVC)
  string(REPLACE "-O2" "-O3" CMAKE_CXX_FLAGS_RELWITHDEBINFO
                 ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
  string(REPLACE "-O2" "-O3" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif()

# GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# executables
add_executable("${PROJECT_NAME}_test" test.cpp)
target_link_libraries("${PROJECT_NAME}_test"
                      PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)

# add google test
include(GoogleTest)
gtest_discover_tests("${PROJECT_NAME}_test" DISCOVERY_MODE PRE_TEST)

# compile commands
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(
    gen_compile_commands ALL
    COMMENT "Generating compile_commands.json"
    DEPENDS ${PROJECT_NAME}_test)

  # Remove old file
  add_custom_command(
    TARGET gen_compile_commands
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove
            ${CMAKE_SOURCE_DIR}/compile_commands.json)

  # Generate a compilation database with header files
  if(NOT COMPDB)
    find_program(COMPDB compdb)
  endif()

  if(COMPDB)
    add_custom_command(
      TARGET gen_compile_commands
      POST_BUILD
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMAND ${COMPDB} list > ${CMAKE_SOURCE_DIR}/compile_commands.json
      COMMENT "Running compdb")
  else()
    message(AUTHOR_WARNING "'compdb' not found")

    # Copy file instead of compdb
    add_custom_command(
      TARGET gen_compile_commands
      POST_BUILD
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/compile_commands.json
              ${CMAKE_SOURCE_DIR}/compile_commands.json
      COMMENT "Creating symbolic link")
  endif()
endif()
