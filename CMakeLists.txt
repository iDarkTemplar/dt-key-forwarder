project(dt-key-forwarder)

cmake_minimum_required( VERSION 2.8.8 )

set(DKF_VERSION_MAJOR 0)
set(DKF_VERSION_MINOR 1)
set(DKF_VERSION_PATCH 0)
set(DKF_VERSION "${DKF_VERSION_MAJOR}.${DKF_VERSION_MINOR}.${DKF_VERSION_PATCH}")

set(CPACK_PACKAGE_VERSION_MAJOR ${DKF_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DKF_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DKF_VERSION_PATCH})
set(CPACK_GENERATOR "TBZ2")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES
	".git;build;CMakeLists.txt.user;CMakeLists.txt.user.*;TODO")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${DKF_VERSION}")
# MUST be after CPACK_* variables.
include(CPack)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set (OS_LINUX true)
	add_definitions(-DOS_Linux)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
	set (OS_FREEBSD true)
	add_definitions(-DOS_FreeBSD)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")

# installation directory configuration
set(INST_PREFIX "/usr" CACHE PATH "Prefix prepended to install directories")
set(CMAKE_INSTALL_PREFIX "${INST_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

message(STATUS "To change installation directory use -DINST_PREFIX=path, for example, -DINST_PREFIX=/usr")

# don't USE -O3 with GCC, it causes less precise calculations
if (CMAKE_COMPILER_IS_GNUCC)
	set (CMAKE_C_FLAGS_RELEASE "-O2 -pipe -march=native -Wall -Wextra")
	set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
endif (CMAKE_COMPILER_IS_GNUCC)

# clang compiler options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set (CLANG_WARNINGS "-Weverything -Wno-missing-prototypes -Wno-padded -Wno-sign-conversion -Wno-conversion -Wno-disabled-macro-expansion -Wno-cast-align" )
	set (CMAKE_C_FLAGS_RELEASE "-O3 -pipe ${CLANG_WARNINGS}")
	set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

find_package ( DtCommand REQUIRED )
find_package ( X11 COMPONENTS X11_Xinput X11_Xkblib X11_XTest REQUIRED )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${DT_CMD_INCLUDE_DIRS} )
include_directories( ${X11_Xinput_INCLUDE_PATH} ${X11_Xkblib_INCLUDE_PATH} ${X11_XTest_INCLUDE_PATH} )

add_definitions(-D_FILE_OFFSET_BITS=64)

set ( CLIENT_SOURCES client/main.c )
set ( CLIENT_HEADERS common/commands.h )
set ( CLIENT_LIBS ${X11_X11_LIB} ${X11_Xinput_LIB} )

set ( SERVER_SOURCES server/main.c )
set ( SERVER_HEADERS common/commands.h )
set ( SERVER_LIBS ${DT_CMD_LIBRARIES} ${X11_X11_LIB} ${X11_XTest_LIB})

add_executable( dt-key-forwarder-client ${CLIENT_SOURCES} ${CLIENT_HEADERS})
target_link_libraries( dt-key-forwarder-client ${CLIENT_LIBS} )

add_executable( dt-key-forwarder-server ${SERVER_SOURCES} ${SERVER_HEADERS})
target_link_libraries( dt-key-forwarder-server ${SERVER_LIBS} )

# installation config
install(TARGETS dt-key-forwarder-client RUNTIME DESTINATION ./bin)
install(TARGETS dt-key-forwarder-server RUNTIME DESTINATION ./bin)
