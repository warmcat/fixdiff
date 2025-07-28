cmake_minimum_required(VERSION 3.21)
project(fixdiff C)
include(CTest)

set(SRCS fixdiff.c)

set(COMPILE_WARNING_AS_ERROR 1)
add_executable(${PROJECT_NAME} ${SRCS})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# separate sha256 for windows, because both stdout shell redirect
# and piping stdout onward are in _TEXT

# single patched file, directly taken from Gemini 2.5 copy button

add_test(NAME lws1
	 COMMAND ${CMAKE_COMMAND}
	 	-DCMD=$<TARGET_FILE:${PROJECT_NAME}>
		-DSRC=client-parser-ws.c
		-DPATCH=gemini.patch
		-DEXPSHA=fec27b802dc46c2e26f5ccc9316683a780c0785dc46c95f7a9fe73314bb81f5d
		-DEXPSHA_WIN=2e6b9b12ae0128c9edfc109744b9c67848712b0521c322a45104895aa4cbc3b1
		-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtest.cmake
	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/1)

 # two concatenated single file patches

add_test(NAME lws2
	 COMMAND ${CMAKE_COMMAND}
	 	-DCMD=$<TARGET_FILE:${PROJECT_NAME}>
		-DSRC=deaddrop.js
		-DSRC1=protocol_lws_deaddrop.c
		-DPATCH=gemini.patch
		-DEXPSHA=39365eaf3a5ba562ff40273d8d6c9a0760c917322ed29c66c7fafc6a9f4d5cd1
		-DEXPSHA1=c742cdad75b3f4f1d742b4cf135079ba319f9b85ce8615799e2ed4baa4e12b97
		-DEXPSHA_WIN=8c5eda52afdf8976090ab75969753ea260c2a9c0e52bd7eb898e9137b0952a64
		-DEXPSHA1_WIN=dadd4162eee0c8acdbdeb43cb9e97c448c766dbab2bec9d866fcd5a76243b593
		-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtest.cmake
	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/2)

 # two concatenated patches using git style diff and index lines (otherwise same as above)

 add_test(NAME lws3
	 COMMAND ${CMAKE_COMMAND}
	 	-DCMD=$<TARGET_FILE:${PROJECT_NAME}>
		-DSRC=deaddrop.js
		-DSRC1=protocol_lws_deaddrop.c
		-DPATCH=gemini.patch
		-DEXPSHA=39365eaf3a5ba562ff40273d8d6c9a0760c917322ed29c66c7fafc6a9f4d5cd1
		-DEXPSHA1=c742cdad75b3f4f1d742b4cf135079ba319f9b85ce8615799e2ed4baa4e12b97
		-DEXPSHA_WIN=8c5eda52afdf8976090ab75969753ea260c2a9c0e52bd7eb898e9137b0952a64
		-DEXPSHA1_WIN=dadd4162eee0c8acdbdeb43cb9e97c448c766dbab2bec9d866fcd5a76243b593
		-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtest.cmake
	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/3)

 # two concatenated patches using git style diff and index lines with extra newline at end
 # (otherwise same as above)

 add_test(NAME lws4
	 COMMAND ${CMAKE_COMMAND}
	 	-DCMD=$<TARGET_FILE:${PROJECT_NAME}>
		-DSRC=deaddrop.js
		-DSRC1=protocol_lws_deaddrop.c
		-DPATCH=gemini.patch
		-DEXPSHA=39365eaf3a5ba562ff40273d8d6c9a0760c917322ed29c66c7fafc6a9f4d5cd1
		-DEXPSHA1=c742cdad75b3f4f1d742b4cf135079ba319f9b85ce8615799e2ed4baa4e12b97
		-DEXPSHA_WIN=8c5eda52afdf8976090ab75969753ea260c2a9c0e52bd7eb898e9137b0952a64
		-DEXPSHA1_WIN=dadd4162eee0c8acdbdeb43cb9e97c448c766dbab2bec9d866fcd5a76243b593
		-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtest.cmake
	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/4)

 # Same as test1 but with 3 x examples of whitespace-only lines added that should be
 # reduced down to empty lines

add_test(NAME lws5
	 COMMAND ${CMAKE_COMMAND}
	 	-DCMD=$<TARGET_FILE:${PROJECT_NAME}>
		-DSRC=client-parser-ws.c
		-DPATCH=gemini.patch
		-DEXPSHA=fec27b802dc46c2e26f5ccc9316683a780c0785dc46c95f7a9fe73314bb81f5d
		-DEXPSHA_WIN=2e6b9b12ae0128c9edfc109744b9c67848712b0521c322a45104895aa4cbc3b1
		-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtest.cmake
	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/5)


