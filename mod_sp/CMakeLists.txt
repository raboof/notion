add_module(mod_sp
	SOURCES
		main.c
	EXPORTS
)

install(FILES
		cfg_sp.lua
	DESTINATION ${ETCDIR}
)
