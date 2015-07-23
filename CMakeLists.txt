cmake_minimum_required(VERSION 2.6)
set(PROJ auth)
set(AUTHLIB_VERSION "0.1")
project(${PROJ})
set(AUTHLIB "CGI_auth")
set(ATEST auth_test)
set(HTTPS_DIR "/Data/Misc/SSL") # корень https-сервера
set(SERVER_NAME "localhost") # полное имя сервера
set(FILEDIR "/Data/Misc/html/tmp") # директория для хранения загруженных файлов
set(DBNAME "${HTTPS_DIR}/cgi-bin/user-pass") # путь к базе данных
set(LOCALEDIR "/usr/share/locale")
if(DEFINED MAX_QMEM)
	add_definitions(-DMAX_MEMORY_FOR_QUERY=\"${MAX_QMEM}\")
endif()
if(DEFINED DEBUG)
	add_definitions(-DEBUG)
endif()
if(DEFINED MAX_QSIZE)
	add_definitions(-DMAX_QSIZE=\"${MAX_QSIZE}\")
endif()
if(DEFINED MAX_FSIZE)
	add_definitions(-DMAX_FSIZE=\"${MAX_FSIZE}\")
endif()
set(ASRC "src/auth.c")
set(ATESTSRC "src/auth_test.c")
set(CMAKE_COLOR_MAKEFILE ON)

#aux_source_directory(./src SOURCES)
aux_source_directory(./lib LIBSRCS)
set(LCPATH "locale/ru")
set(CFLAGS -O3 -Wall -Werror -W -std=c99)
add_definitions(${CFLAGS} -D_XOPEN_SOURCE=666 -DDBNAME=\"${DBNAME}\"
	-DFILEDIR=\"${FILEDIR}\" -DSERVER_NAME=\"${SERVER_NAME}\"
	-DGETTEXT_PACKAGE=\"${PROJ}\" -DLOCALEDIR=\"${LOCALEDIR}\"
	-DPACKAGE_VERSION=\"${AUTHLIB_VERSION}\")
set(PO_FILE "${LCPATH}/messages.po")
set(MO_FILE "${LCPATH}/LC_MESSAGES/${PROJ}.mo")
set(RU_FILE "${LCPATH}/ru.po")
set(LIBHEADER "src/include/CGI_auth.h")
find_package(PkgConfig REQUIRED)
pkg_check_modules(${PROJ} REQUIRED
		sqlite3>=0.23
		)
include_directories("src/include" ${${PROJ}_INCLUDE_DIRS})
add_library(${AUTHLIB} SHARED ${LIBSRCS} ${PO_FILE} ${MO_FILE})
add_executable(${PROJ} ${ASRC})
add_executable(${ATEST} ${ATESTSRC})
target_link_libraries(${AUTHLIB} -lcrypt -lm)
target_link_libraries(${PROJ} ${${PROJ}_LIBRARIES} ${AUTHLIB})
target_link_libraries(${ATEST} ${${PROJ}_LIBRARIES} ${AUTHLIB})
link_directories(${${PROJ}_LIBRARY_DIRS} "/usr/local/lib")

set(PCFILE "${AUTHLIB}.pc")
configure_file("./lib/${AUTHLIB}.pc.in" ${PCFILE} @ONLY)

# Installation of the program
#set(CMAKE_INSTALL_PREFIX ${HTTPS_DIR} CACHE STRING "Install path" FORCE)
install(TARGETS ${PROJ} ${ATEST}
		DESTINATION "${HTTPS_DIR}/cgi-bin"
		)#PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES ${MO_FILE}
		DESTINATION "${LOCALEDIR}/ru/LC_MESSAGES"
		)#PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
install(FILES ${LIBHEADER}
		DESTINATION "include"
		)
set(CMAKE_INSTALL_SO_NO_EXE "0")
install(TARGETS ${AUTHLIB} LIBRARY
		DESTINATION "lib"
		)
install(FILES ${PCFILE}
		DESTINATION "lib/pkgconfig"
		)
# Script to be executed at installation time (kind of post-intallation script) to
# change the right accesses on the installed files
#install(SCRIPT install_html.cmake)

if(DEFINED DEBUG)
	find_package(Gettext REQUIRED)
	find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
	if(NOT GETTEXT_XGETTEXT_EXECUTABLE )
		  message(FATAL_ERROR "xgettext not found")
	endif(NOT GETTEXT_XGETTEXT_EXECUTABLE )
add_custom_command(
	OUTPUT ${PO_FILE}
	COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} --from-code=koi8-r ${LIBSRCS} -c -k_ -kN_ -o ${PO_FILE}
	COMMAND sed 's/charset=UTF-8/charset=koi8-r/' ${PO_FILE} | enconv > tmp__ && mv -f tmp__ ${PO_FILE}
	COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} -Uis ${RU_FILE} ${PO_FILE}
	DEPENDS ${LIBSRCS})
add_custom_command(
	OUTPUT ${MO_FILE}
	COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} ${RU_FILE} -o ${MO_FILE}
	DEPENDS ${RU_FILE})
endif()

