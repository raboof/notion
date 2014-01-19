add_subdirectory(ion-completefile)
add_subdirectory(ion-statusd)

install(FILES
		ion-runinxterm
		ion-completeman
		notion-lock
	DESTINATION ${SHAREDIR}
)
