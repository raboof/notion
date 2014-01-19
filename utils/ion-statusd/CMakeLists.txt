set(c_sources
	ion-statusd.c
	exec.c
	extlrx.c
)

set(lua_sources
	statusd_date.lua
	statusd_mail.lua
	statusd_load.lua
)

set(mainloopsources
	${CMAKE_SOURCE_DIR}/libmainloop/select.c
	${CMAKE_SOURCE_DIR}/libmainloop/defer.c
	${CMAKE_SOURCE_DIR}/libmainloop/signal.c
	${CMAKE_SOURCE_DIR}/libmainloop/hooks.c
	${CMAKE_SOURCE_DIR}/libmainloop/exec.c
)

mkexports(statusd
	SOURCES ${c_sources}
	OUTPUT_PREFIX exports
	EXTRAS -reexport mainloop ${mainloopsources}
)

add_executable(ion-statusd
	${c_sources}
	exports.c
)

target_link_libraries(ion-statusd
	${LUA_LIBRARIES}
	dl
	m

	mainloop
	extl
	tu
)

install(TARGETS ion-statusd
	DESTINATION ${EXTRABINDIR}
)
