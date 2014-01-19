add_module(mod_xrandr
	SOURCES
		mod_xrandr.c
	EXPORTS
)

install(FILES
		cfg_xrandr.lua
	DESTINATION ${ETCDIR}
)
