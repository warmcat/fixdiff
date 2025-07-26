cmake_minimum_required(VERSION 3.21)
project(fixdiff C)
include(CTest)

set(SRCS fixdiff.c)

set(COMPILE_WARNING_AS_ERROR 1)
add_executable(${PROJECT_NAME} ${SRCS})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# separate sha256 for windows, because both stdout shell redirect
# and piping stdout onward are in _TEXT

add_test(NAME lws1
	 COMMAND ${CMAKE_COMMAND}
	 	-DCMD=$<TARGET_FILE:${PROJECT_NAME}>
		-DSRC=client-parser-ws.c
		-DPATCH=gemini.patch
		-DEXPSHA=fec27b802dc46c2e26f5ccc9316683a780c0785dc46c95f7a9fe73314bb81f5d
		-DEXPSHA_WIN=2e6b9b12ae0128c9edfc109744b9c67848712b0521c322a45104895aa4cbc3b1
		-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtest.cmake
	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/1)

add_test(NAME lws2
	 COMMAND ${CMAKE_COMMAND}
	 	-DCMD=$<TARGET_FILE:${PROJECT_NAME}>
		-DSRC=deaddrop.js
		-DSRC1=protocol_lws_deaddrop.c
		-DPATCH=gemini.patch
		-DEXPSHA=0f988ae2f642a825188448e4bee9db8943050e80e075dbcd6fc4ed9ada87f6fb
		-DEXPSHA_WIN=2e6b9b12ae0128c9edfc109744b9c67848712b0521c322a45104895aa4cbc3b1
		-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtest.cmake
	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/2)

