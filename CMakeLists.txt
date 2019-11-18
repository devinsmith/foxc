# Minimum of CentOS 6.6 :(
cmake_minimum_required(VERSION 2.8.12)
project(foxc)

INCLUDE (CheckIncludeFiles)

IF(UNIX)
	set (X11_Xft_FOUND 0)
	find_package(X11)
	find_package(Freetype)

	if (X11_Xrandr_FOUND)
		set (HAVE_XRANDR_H 1)
	endif(X11_Xrandr_FOUND)

	if (X11_Xft_FOUND)
		set (HAVE_XFT_H 1)
		include_directories(${X11_Xft_INCLUDE_PATH})
	endif(X11_Xft_FOUND)

	if (FREETYPE_FOUND)
		include_directories(${FREETYPE_INCLUDE_DIR_ft2build})
		include_directories(${FREETYPE_INCLUDE_DIR_freetype2})
	endif(FREETYPE_FOUND)

	include_directories(${X11_INCLUDE_DIR})

ENDIF(UNIX)

# Bring in headers
include_directories(include)
include_directories("${PROJECT_BINARY_DIR}")

configure_file("${PROJECT_SOURCE_DIR}/src/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h")

file(GLOB SOURCES "src/*.c")

add_library(foxc STATIC ${SOURCES})
