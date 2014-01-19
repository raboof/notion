add_executable(notion
	notion.c
)

target_link_libraries(notion
	${LUA_LIBRARIES}
	X11
	Xext
	SM
	ICE
	dl
	m

	ioncore
	mainloop
	tu
	extl
)

install(TARGETS notion DESTINATION ${BINDIR})
