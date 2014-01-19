set(c_sources
	binding.c conf-bindings.c cursor.c event.c exec.c focus.c
	strings.c key.c modules.c mwmhints.c pointer.c property.c
	screen.c sizehint.c window.c ioncore.c
	xic.c selection.c clientwin.c colormap.c region.c eventh.c
	attach.c resize.c grab.c manage.c regbind.c
	rootwin.c tags.c names.c saveload.c frame.c
	frame-pointer.c conf.c reginfo.c extlconv.c fullscreen.c mplex.c
	bindmaps.c gr.c infowin.c activity.c netwm.c frame-draw.c
	kbresize.c rectangle.c xwindow.c presize.c extlrx.c
	pholder.c mplexpholder.c llist.c basicpholder.c sizepolicy.c
	stacking.c group.c grouppholder.c group-cw.c navi.c
	group-ws.c float-placement.c framedpholder.c
	return.c detach.c screen-notify.c frame-tabs-recalc.c profiling.c
	log.c
)

set(lua_sources
	ioncore_ext.lua ioncore_luaext.lua ioncore_bindings.lua
	ioncore_winprops.lua ioncore_misc.lua ioncore_efbb.lua
	ioncore_wd.lua ioncore_menudb.lua ioncore_tabnum.lua
	ioncore_quasiact.lua
)

set(mainloopsources
	${CMAKE_SOURCE_DIR}/libmainloop/select.c
	${CMAKE_SOURCE_DIR}/libmainloop/defer.c
	${CMAKE_SOURCE_DIR}/libmainloop/signal.c
	${CMAKE_SOURCE_DIR}/libmainloop/hooks.c
	${CMAKE_SOURCE_DIR}/libmainloop/exec.c
)

mkexports(ioncore
	SOURCES ${c_sources}
	OUTPUT_PREFIX exports
	EXTRAS -reexport mainloop ${mainloopsources}
)

add_library(ioncore
	${c_sources}
	exports.c
)
