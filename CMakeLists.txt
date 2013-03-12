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
	status
	syspopup
	notification
	appsvc)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag} -Wall -g")
ENDFOREACH(flag)
#SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -finstrument-functions")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIC")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g ")
SET(CMAKE_C_FLAGS_RELEASE "-O2")
#This is flag for Gprof
#SET(CMAKE_C_FLAGS_RELEASE "-pg")

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" STREQUAL "arm")
	ADD_DEFINITIONS("-DTARGET")
	MESSAGE("add -DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DSLP_DEBUG")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

#ADD_CUSTOM_TARGET(custom_popup.edj
#COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/data
#${CMAKE_CURRENT_SOURCE_DIR}/data/custom_popup.edc
#${CMAKE_CURRENT_SOURCE_DIR}/data/custom_popup.edj DEPENDS
#${CMAKE_CURRENT_SOURCE_DIR}/data/custom_popup.edc
#)

#ADD_DEPENDENCIES(${PROJECT_NAME} custom_popup.edj)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION /usr/apps/org.tizen.net-popup/bin)

# install desktop file & icon
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_BINARY_DIR}/resources/org.tizen.net-popup.xml DESTINATION /usr/share/packages/)
#INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.tizen.net-popup.png DESTINATION /usr/share/icons/default/small)
#INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/custom_popup.edj DESTINATION /usr/apps/org.tizen.net-popup/res/edje)

ADD_SUBDIRECTORY(po)
