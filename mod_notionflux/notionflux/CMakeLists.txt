add_executable(notionflux
	notionflux.c
)

target_link_libraries(notionflux
	X11
	socket
)

install(TARGETS notionflux DESTINATION ${BINDIR})
