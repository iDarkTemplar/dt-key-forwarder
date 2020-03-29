cmake_minimum_required( VERSION 3.16.0 )

project(dt-key-forwarder
	VERSION 0.3.0
	LANGUAGES C)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set (OS_LINUX true)
	add_definitions(-DOS_Linux)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
	set (OS_FREEBSD true)
	add_definitions(-DOS_FreeBSD)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")

# installation directory configuration
set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)
set(SBIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/sbin CACHE PATH "Installation prefix for user executables" FORCE)
set(LIB_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX} CACHE PATH  "Installation prefix for object code libraries" FORCE)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "Installation prefix for C header files" FORCE)
set(DATA_PREFIX "${CMAKE_INSTALL_PREFIX}/share" CACHE PATH "Prefix prepended to data install directories" FORCE)

# don't USE -O3 with GCC, it causes less precise calculations
if (CMAKE_COMPILER_IS_GNUCC)
	set (CMAKE_C_FLAGS_RELEASE "-O2 -pipe -march=native -Wall -Wextra")
	set (CMAKE_C_FLAGS_DEBUG "-g -O0 -pipe -march=native -Wall -Wextra")
endif (CMAKE_COMPILER_IS_GNUCC)

# clang compiler options
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
	set (CLANG_WARNINGS "-Weverything -Wno-missing-prototypes -Wno-padded -Wno-sign-conversion -Wno-conversion -Wno-disabled-macro-expansion -Wno-cast-align" )
	set (CMAKE_C_FLAGS_RELEASE "-O3 -pipe ${CLANG_WARNINGS}")
	set (CMAKE_C_FLAGS_DEBUG "-g -O0 -pipe ${CLANG_WARNINGS}")
endif ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")

find_package ( DtCommand 2.0.0 REQUIRED )
find_package ( X11 COMPONENTS Xi Xkb Xtst REQUIRED )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${DtCommand_INCLUDE_DIRS} )
include_directories( ${X11_Xi_INCLUDE_PATH} ${X11_xkblib_INCLUDE_PATH} ${X11_Xtst_INCLUDE_PATH} )

add_definitions(-D_FILE_OFFSET_BITS=64)

set ( CLIENT_SOURCES client/main.c )
set ( CLIENT_HEADERS common/commands.h )
set ( CLIENT_LIBS ${X11_X11_LIB} ${X11_Xi_LIB} )

set ( SERVER_SOURCES server/main.c )
set ( SERVER_HEADERS common/commands.h )
set ( SERVER_LIBS ${DtCommand_LIBRARIES} ${X11_X11_LIB} ${X11_Xtst_LIB})

add_executable( dt-key-forwarder-client ${CLIENT_SOURCES} ${CLIENT_HEADERS})
target_link_libraries( dt-key-forwarder-client ${CLIENT_LIBS} )

add_executable( dt-key-forwarder-server ${SERVER_SOURCES} ${SERVER_HEADERS})
target_link_libraries( dt-key-forwarder-server ${SERVER_LIBS} )

# installation config
install(TARGETS dt-key-forwarder-client RUNTIME DESTINATION ${BIN_INSTALL_DIR})
install(TARGETS dt-key-forwarder-server RUNTIME DESTINATION ${BIN_INSTALL_DIR})
