add_executable(ion-completefile
	ion-completefile.c
)

target_link_libraries(ion-completefile
	tu
)

install(TARGETS ion-completefile
	DESTINATION ${EXTRABINDIR}
)
