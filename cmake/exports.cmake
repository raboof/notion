macro(mkexports module)
	set(options)
	set(one_val_args OUTPUT_PREFIX)
	set(multi_val_args SOURCES EXTRAS)

	cmake_parse_arguments(MKE "${options}" "${one_val_args}" "${multi_val_args}" ${ARGN})

	set(cfile "${MKE_OUTPUT_PREFIX}.c")
	set(hfile "${MKE_OUTPUT_PREFIX}.h")

	add_custom_command(
		OUTPUT ${cfile} ${hfile}
		COMMAND ${CMAKE_SOURCE_DIR}/libextl/libextl-mkexports
			-module ${module} -o ${cfile} -h ${hfile} ${MKE_SOURCES}
			${MKE_EXTRAS}
		DEPENDS ${MKE_SOURCES}
	)
endmacro()
