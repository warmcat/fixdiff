
function(patch_check CMD SRC SRC1 PATCH EXPSHA EXPSHA1 EXPSHA_WIN EXPSHA1_WIN)
	file(COPY_FILE ${SRC}-orig ${SRC})

	if (SRC1)
		file(COPY_FILE ${SRC1}-orig ${SRC1})
	endif()

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
		if ( "${RESULT_SHA256}" STREQUAL "${EXPSHA_WIN}" )
		else()
			message(FATAL_ERROR "${SRC} SHA256 differs: ${RESULT_SHA256}")
		endif()
	endif()

	if (EXPSHA1)
		file(SHA256 ${SRC1} RESULT_SHA256)

		if ( "${RESULT_SHA256}" STREQUAL "${EXPSHA1}" )
		else()
			if ( "${RESULT_SHA256}" STREQUAL "${EXPSHA1_WIN}" )
			else()
				message(FATAL_ERROR "${SRC1} SHA256 differs: ${RESULT_SHA256}")
			endif()
		endif()
	endif()

endfunction(patch_check)

patch_check("${CMD}" "${SRC}" "${SRC1}" "${PATCH}" "${EXPSHA}" "${EXPSHA1}" "${EXPSHA_WIN}" "${EXPSHA1_WIN}")

