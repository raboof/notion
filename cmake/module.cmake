macro(add_module name)
	set(options EXPORTS)
	set(one_val_args)
	set(multi_val_args SOURCES)

	cmake_parse_arguments(AM "${options}" "${one_val_args}" "${multi_val_args}" ${ARGN})

	set(sources ${AM_SOURCES})

	if(${AM_EXPORTS})
		mkexports(${name} SOURCES ${sources} OUTPUT_PREFIX exports)

		set(sources ${sources} exports.c)
	endif()

	add_library(${name} MODULE ${sources})
	set_target_properties(${name} PROPERTIES PREFIX "")
	install(TARGETS ${name} DESTINATION ${MODULEDIR})
endmacro()
