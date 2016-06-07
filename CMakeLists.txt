CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(net-popup C)

SET(SRCS ${CMAKE_SOURCE_DIR}/src/net-popup.c)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(LOCALEDIR "${PREFIX}/share/locale")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	appcore-efl
	bundle
	dlog
	ecore
	edje
	elementary
	evas
	syspopup
	notification
	capi-appfw-application
	glib-2.0
	gio-2.0
	gio-unix-2.0
	appsvc
	vconf
	efl-extension)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -Wall -Werror -fvisibility=hidden -fPIE")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -fPIE")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -fPIE")

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" STREQUAL "arm")
	ADD_DEFINITIONS("-DTARGET")
	MESSAGE("add -DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} -pie)

ADD_CUSTOM_TARGET(netpopup-custom.edj
	COMMAND edje_cc ${CMAKE_CURRENT_SOURCE_DIR}/edcs/netpopup-custom.edc
		${CMAKE_CURRENT_SOURCE_DIR}/edcs/netpopup-custom.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/edcs/netpopup-custom.edc
)

ADD_DEPENDENCIES(${PROJECT_NAME} netpopup-custom.edj)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)

# install desktop file & icon
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/edcs/netpopup-custom.edj DESTINATION /usr/ug/res/edje/net-popup/)
INSTALL(FILES ${CMAKE_BINARY_DIR}/resources/net.netpopup.xml DESTINATION ${PREFIX}/share/packages/)

ADD_SUBDIRECTORY(po)
