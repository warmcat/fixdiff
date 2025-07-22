cmake_minimum_required(VERSION 3.10)
project(fixdiff C)

include(CTest)

set(SRCS fixdiff.c)

set(COMPILE_WARNING_AS_ERROR 1)

add_executable(${PROJECT_NAME} ${SRCS})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

add_test(NAME lws1 COMMAND bash test.sh WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/1) 
