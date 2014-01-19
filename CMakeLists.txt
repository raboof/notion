cmake_minimum_required(VERSION 2.8.5)

include_directories(${CMAKE_SOURCE_DIR})

set(WARNINGS "-W -Wimplicit -Wreturn-type -Wswitch -Wcomment -Wtrigraphs")
set(WARNINGS "${WARNINGS} -Wformat -Wchar-subscripts -Wparentheses -pedantic")
set(WARNINGS "${WARNINGS} -Wuninitialized")

set(CMAKE_C_FLAGS "${WARNINGS} -Os -std=c99 -g")

add_definitions(-D_POSIX_C_SOURCE=200112L)

include(cmake/config.cmake)
include(cmake/exports.cmake)
include(cmake/module.cmake)

add_subdirectory(libtu)
add_subdirectory(libmainloop)
add_subdirectory(libextl)

add_subdirectory(ioncore)
add_subdirectory(notion)
add_subdirectory(etc)
add_subdirectory(utils)
#add_subdirectory(man)
#add_subdirectory(po)

add_subdirectory(de)
add_subdirectory(mod_dock)
add_subdirectory(mod_menu)
add_subdirectory(mod_notionflux)
add_subdirectory(mod_query)
add_subdirectory(mod_sm)
add_subdirectory(mod_sp)
add_subdirectory(mod_statusbar)
add_subdirectory(mod_tiling)
add_subdirectory(mod_xinerama)
add_subdirectory(mod_xkbevents)
add_subdirectory(mod_xrandr)
