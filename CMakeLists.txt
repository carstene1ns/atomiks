cmake_minimum_required(VERSION 3.20...4.3 FATAL_ERROR)

project(atomiks VERSION 1.0.5 LANGUAGES C
	DESCRIPTION "A remake of the classic Atomix game"
	HOMEPAGE_URL "https://github.com/carstene1ns/atomiks")

set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type is set to ${CMAKE_BUILD_TYPE}.")

# core lib
add_library(atomcore OBJECT
	src/atomcore.c src/atomcore.h
	src/drv_gra.c src/drv_gra.h
	src/gz.c src/gz.h) # includes src/tinfl.c

# executable
add_executable(atomiks
	src/atomiks.c
	src/cfg.c src/cfg.h
	src/data.h
	src/drv_inp.c src/drv_inp.h
	src/drv_snd.c src/drv_snd.h
	src/drv_tim.c src/drv_tim.h
	src/levels.h
	$<TARGET_OBJECTS:atomcore>)
target_compile_definitions(atomiks PRIVATE PVER="v${PROJECT_VERSION}")
target_compile_options(atomiks PRIVATE -Wall -Wextra)

if(WIN32)
	set_target_properties(atomiks PROPERTIES WIN32_EXECUTABLE TRUE)
	target_sources(atomiks PRIVATE res/atomiks.rc)
endif()

# editor
add_executable(editor EXCLUDE_FROM_ALL src/editor.c $<TARGET_OBJECTS:atomcore>)
target_compile_options(editor PRIVATE -Wall -Wextra)

# external libs
find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_mixer REQUIRED CONFIG)
find_package(SDL2_image CONFIG)
target_link_libraries(atomcore PUBLIC SDL2::SDL2)
target_link_libraries(atomiks PRIVATE SDL2::SDL2 SDL2_mixer::SDL2_mixer)
target_link_libraries(editor PRIVATE SDL2::SDL2)

# tools
add_executable(file2c EXCLUDE_FROM_ALL tools/file2c.c)
if(SDL2_image_FOUND)
	add_executable(png2bmp EXCLUDE_FROM_ALL tools/png2bmp.c)
	target_link_libraries(png2bmp PRIVATE SDL2::SDL2 SDL2_image::SDL2_image)
endif()
add_executable(zopfli EXCLUDE_FROM_ALL
	tools/zopfli-1.0/blocksplitter.c tools/zopfli-1.0/blocksplitter.h
	tools/zopfli-1.0/cache.c tools/zopfli-1.0/cache.h
	tools/zopfli-1.0/deflate.c tools/zopfli-1.0/deflate.h
	tools/zopfli-1.0/gzip_container.c tools/zopfli-1.0/gzip_container.h
	tools/zopfli-1.0/hash.c tools/zopfli-1.0/hash.h
	tools/zopfli-1.0/katajainen.c tools/zopfli-1.0/katajainen.h
	tools/zopfli-1.0/lz77.c tools/zopfli-1.0/lz77.h
	tools/zopfli-1.0/squeeze.c tools/zopfli-1.0/squeeze.h
	tools/zopfli-1.0/tree.c tools/zopfli-1.0/tree.h
	tools/zopfli-1.0/util.c tools/zopfli-1.0/util.h
	tools/zopfli-1.0/zlib_container.c tools/zopfli-1.0/zlib_container.h
	tools/zopfli-1.0/zopfli.h
	tools/zopfli-1.0/zopfli_bin.c
	tools/zopfli-1.0/zopfli_lib.c
)
target_link_libraries(zopfli PRIVATE m)

# installation
if(WIN32)
	# put everything in a folder
	install(TARGETS atomiks RUNTIME DESTINATION dist)
elseif(UNIX)
	include(GNUInstallDirs)
	install(TARGETS atomiks RUNTIME DESTINATION /bin)
	install(FILES res/atomiks.desktop DESTINATION /share/applications)
	install(FILES res/atomiks.png DESTINATION /share/icons/hicolor/64x64/apps)
endif()

# source code formatting
if(NOT ASTYLE_FOUND)
	find_program(ASTYLE_EXECUTABLE astyle)
	if(ASTYLE_EXECUTABLE)
		message(STATUS "Found source code formatter: astyle")
		set(ASTYLE_FOUND 1 CACHE INTERNAL "Astyle has been found")
	endif()
endif()
if(ASTYLE_EXECUTABLE)
	separate_arguments(ASTYLE_OPTIONS UNIX_COMMAND
		"--options=none --project=none --suffix=none --style=attach
		--indent=spaces=2 --indent-preproc-block --indent-switches
		--min-conditional-indent=0 --squeeze-lines=1 --max-code-length=100
		--pad-oper --pad-comma --pad-header --unpad-paren --align-pointer=name
		--ignore-exclude-errors --exclude=data.h --exclude=levels.h --exclude=tinfl.c")
	add_custom_target(source-format
		COMMAND ${ASTYLE_EXECUTABLE} ${ASTYLE_OPTIONS}
			"${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/*.c"
		COMMENT "Applying source code format..." VERBATIM)
else()
	add_custom_target(source-format
		COMMAND ${CMAKE_COMMAND} -E false
		COMMENT "No source code formatter enabled!" VERBATIM)
endif()
