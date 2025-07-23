
function(patch_check CMD SRC PATCH EXPSHA)
	file(COPY_FILE ${SRC}-orig ${SRC})

	execute_process(COMMAND cat ${PATCH}
			COMMAND ${CMD}
			COMMAND patch -p1
			RESULT_VARIABLE CMD_RESULT)
	if (CMD_RESULT)
		message(FATAL_ERROR "Error running ${CMD}")
	endif()

	file(SHA256 ${SRC} RESULT_SHA256)

	if ( "${RESULT_SHA256}" STREQUAL "${EXPSHA}" )
	else()
		message(FATAL_ERROR "Test SHA256 differs")
	endif()
endfunction(patch_check)

patch_check("${CMD}" "${SRC}" "${PATCH}" "${EXPSHA}")

