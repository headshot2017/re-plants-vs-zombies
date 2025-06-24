cmake_minimum_required(VERSION 3.5)

project(re-plants-vs-zombies LANGUAGES C CXX ASM)

file(GLOB SOURCES
	*.cpp
	SexyAppFramework/*.cpp
	SexyAppFramework/graphics/*.cpp
	SexyAppFramework/imagelib/*.cpp
	SexyAppFramework/misc/*.cpp
	SexyAppFramework/paklib/*.cpp
	SexyAppFramework/sound/*.cpp
	SexyAppFramework/widget/*.cpp
	#SexyAppFramework/ogg/*.c
	SexyAppFramework/fcaseopen/*.c
	Sexy.TodLib/*.cpp
	Lawn/*.cpp
	Lawn/Widget/*.cpp
	Lawn/System/*.cpp
	SexyAppFramework/LawnProject.rc
)

# these files are included and must not be built in the project
list(REMOVE_ITEM SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/Sexy.TodLib/TodDrawTriangle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Sexy.TodLib/TodDrawTriangleInc.cpp
)

set(NO_GLEW OFF)
set(LOW_MEMORY OFF)
set(DOWNSCALE_COUNT 1)
if(NINTENDO_SWITCH)
	set(NO_GLEW ON)
	set(VORBIS_LIBS vorbisfile vorbis)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch
	)
elseif(NINTENDO_3DS)
	set(NO_GLEW ON)
	set(LOW_MEMORY ON)
	set(DOWNSCALE_COUNT 2)
	ctr_add_shader_library(colored ${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/colored.v.pica)
	ctr_add_shader_library(textured ${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/textured.v.pica)
	dkp_add_embedded_binary_library(shaders colored textured)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds
	)
else()
	set(VORBIS_LIBS vorbisfile vorbis)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc
	)
endif()

option(CONSOLE "Show console on Windows" ON)

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL2 REQUIRED)
if (NOT NO_GLEW)
	find_package(GLEW REQUIRED)
endif()

set(SDL_MIXER_X_STATIC ON CACHE BOOL "Build static library of SDL Mixer X" FORCE)
set(SDL_MIXER_X_SHARED OFF CACHE BOOL "Build shared library of SDL Mixer X" FORCE)
set(USE_MIDI OFF CACHE BOOL "SDL Mixer X midi" FORCE)
set(USE_MODPLUG OFF CACHE BOOL "SDL Mixer X modplug" FORCE)
set(USE_XMP OFF CACHE BOOL "SDL Mixer X xmp" FORCE)
set(USE_OPUS OFF CACHE BOOL "SDL Mixer X opus" FORCE)
set(USE_DRFLAC OFF CACHE BOOL "SDL Mixer X flac" FORCE)
set(USE_PXTONE OFF CACHE BOOL "SDL Mixer X pxtone" FORCE)
add_subdirectory(SexyAppFramework/sound/SDL-Mixer-X)

add_executable(re-plants-vs-zombies ${SOURCES})
target_include_directories(re-plants-vs-zombies PRIVATE
	${PROJECT_SOURCE_DIR}/SexyAppFramework
	${PROJECT_SOURCE_DIR}/SexyAppFramework/sound/SDL-Mixer-X/include
	${SDL2_INCLUDE_DIRS}
	${PLAT_INCLUDES}
)
target_compile_definitions(re-plants-vs-zombies PRIVATE _DEBUG)

if (LOW_MEMORY)
	target_compile_definitions(re-plants-vs-zombies PRIVATE LOW_MEMORY)
endif()
target_compile_definitions(re-plants-vs-zombies PRIVATE IMG_DOWNSCALE=${DOWNSCALE_COUNT})

target_link_libraries(re-plants-vs-zombies jpeg png z SDL2_mixer_ext_Static openmpt mpg123 ${VORBIS_LIBS} ogg ${SDL2_LIBRARIES} ${GLEW_LIBRARIES})

if (WIN32)
	target_link_options(re-plants-vs-zombies PRIVATE -static -static-libgcc -static-libstdc++)
	target_link_libraries(re-plants-vs-zombies opengl32 ws2_32 ddraw dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 shlwapi)
	target_compile_definitions(re-plants-vs-zombies PRIVATE WINDOWS)

	if(NOT CONSOLE)
		target_link_options(re-plants-vs-zombies PRIVATE -mwindows)
	endif()
elseif (NINTENDO_SWITCH)
	target_link_libraries(re-plants-vs-zombies glad EGL glapi drm_nouveau)

	nx_generate_nacp(re-plants-vs-zombies.nacp
		NAME "Plants vs. Zombies"
		AUTHOR "PopCap Games, Unofficial Switch port by Headshotnoby"
		VERSION "0.1"
	)

	nx_create_nro(re-plants-vs-zombies
		NACP re-plants-vs-zombies.nacp
		ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon-switch.jpg
	)
elseif (NINTENDO_3DS)
	target_link_libraries(re-plants-vs-zombies citro3d shaders)

	ctr_generate_smdh(re-plants-vs-zombies.smdh
		NAME "Plants vs. Zombies"
		DESCRIPTION "GOTY Edition"
		AUTHOR "PopCap Games, Unofficial 3DS port by Headshotnoby"
		VERSION "0.1"
		ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon-3ds.png"
	)

	ctr_create_3dsx(re-plants-vs-zombies
		SMDH re-plants-vs-zombies.smdh
	)
else()
	target_link_libraries(re-plants-vs-zombies GL)
endif()
