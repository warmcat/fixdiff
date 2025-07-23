cmake_minimum_required(VERSION 3.10)
project(fixdiff C)
include(CTest)

set(SRCS fixdiff.c)

set(COMPILE_WARNING_AS_ERROR 1)
add_executable(${PROJECT_NAME} ${SRCS})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

add_test(NAME lws1
	 COMMAND ${CMAKE_COMMAND}
	 	-DCMD=$<TARGET_FILE:${PROJECT_NAME}>
		-DSRC=client-parser-ws.c
		-DPATCH=gemini.patch
		-DEXPSHA=fec27b802dc46c2e26f5ccc9316683a780c0785dc46c95f7a9fe73314bb81f5d
		-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtest.cmake
	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/1)

