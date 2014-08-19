cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(RBDoom3BFG)

################################################################################

option(USE_MFC_TOOLS
		"Compile the built-in MFC based tools" OFF)
		
option(MONOLITH
		"Embed game logic into main executable" ON)
		
option(SDL2
		"Use SDL2 instead of SDL1.2" OFF)
		
option(OPENAL
		"Use OpenAL soft instead of XAudio2" OFF)

option(FFMPEG
		"Use FMPEG to render Bink videos" ON)

option(IDTOOLS
		"Compile the built-in game tools" ON)

option(STEAM_API
		"Use SteamWorks API instead of generic" OFF)

#options########################################################################

option(BUNDLED_JPEG
		"Use bundled libjpeg" OFF)

option(BUNDLED_PNG
		"Use bundled libpng" OFF)

option(BUNDLED_ZLIB
		"Use bundled zlib" ON)

option(BUNDLED_MINIZIP
		"Use bundled minizip" ON)

option(BUNDLED_TIMIDITY
		"Use bundled timidity" ON)

option(BUNDLED_IRRXML
		"Use bundled irrxml" OFF)

option(BUNDLED_GLEW
		"Use bundled glew" ON)

option(BUNDLED_FREETYPE
		"Use bundled freetype" OFF)

option(BUNDLED_OGGVORBIS
		"Use bundled oggvorbis" OFF)

################################################################################

# Implicated build configuration

if(UNIX)
  set(OPENAL openal)
endif()

if(STEAM_API)
  add_definitions(-DUSE_STEAM_API)
endif()

################################################################################
		
if(MSVC)
	#message(STATUS CMAKE_ROOT: ${CMAKE_ROOT})
	
	#if(CMAKE_CL_64)
	#	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin/win64)
	#else()
	#	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin/win32)
	#endif()
	
	#message(STATUS EXECUTABLE_OUTPUT_PATH: ${EXECUTABLE_OUTPUT_PATH})
	#message(STATUS PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR})
	#message(STATUS CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR})
else()
	message(STATUS CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE})
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	add_definitions(-pipe)
	#add_definitions(-Wall)
	add_definitions(-Werror=format-security)
	add_definitions(-Werror=format)
	add_definitions(-mmmx -msse -msse2)
	if(WIN32)
		# require msvcr70.dll or newer for _aligned_malloc etc
		# I think it is from Visual C++ .NET 2002, so it should be available on any remotely modern system.
		add_definitions(-D__MSVCRT_VERSION__=0x0700)
		
		find_package(DirectX REQUIRED)
		include_directories(libs/mingw-hacks)
		include_directories(${DirectX_INCLUDE_DIR})
		add_definitions(-DUSE_XINPUT)
	endif()	
	
	#if(NOT ANDROID)
		add_definitions(-DUSE_EXCEPTIONS)
	#endif()
	
	# the warnings are used for every profile anyway, so put them in a variable
	set(my_warn_flags "-Wno-pragmas -Wno-unused-variable -Wno-unused-but-set-variable -Wno-switch -Wno-unused-value -Winvalid-pch -Wno-multichar")
	
	if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
		# append clang-specific settings for warnings (the second one make sure clang doesn't complain
		# about unknown -W flags, like -Wno-unused-but-set-variable)
		set(my_warn_flags "${my_warn_flags} -Wno-local-type-template-args -Wno-unknown-warning-option -Wno-inline-new-delete")
	endif()
	
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -O0 -ggdb -fno-strict-aliasing ${my_warn_flags}")
	#set(CMAKE_C_FLAGS_DEBUGALL "${CMAKE_C_FLAGS_DEBUGALL} -g -ggdb -D_DEBUG -fno-strict-aliasing ${my_warn_flags}")
	#set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE} -g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer -fno-strict-aliasing ${my_warn_flags}")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math -fno-unsafe-math-optimizations -fomit-frame-pointer -fno-strict-aliasing ${my_warn_flags}")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g -O3 -ffast-math -fno-unsafe-math-optimizations -fomit-frame-pointer -fno-strict-aliasing ${my_warn_flags}")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -ffast-math -fno-unsafe-math-optimizations -fomit-frame-pointer -fno-strict-aliasing ${my_warn_flags}")
	
	set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL})
	#set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})
	set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
	set(CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})

elseif(MSVC)
	
	# Multiprocessor support for compiling
	# /MP
	
	# /MT  use multithreaded, static runtime library
	#      (=> binary won't need MSVCP100D.dll or similar)
	# /MTd the same with debug symbols
	# /MD and /MDd - the same with dynamic runtime lib
	#     (=> MSVCP100D.dll or similar is needed to run binary)

	# Inline Function Expansion: Any Suitable (/Ob2)
	# "/Ob2",
			
	# enable Intrinsic Functions
	# "/Oi",
			
	# Omit Frame Pointers
	# "/Oy",
	
	# if(MSVC_VERSION EQUAL 1700)
		# #message(STATUS "$ENV{LIBPATH}")
		
		# find_path(Windows_winmd_DIR NAMES Windows.winmd
			# HINTS
			# C:/Program Files (x86)/Windows Kits/8.0/References/CommonConfiguration/Neutral
			# PATH_SUFFIXES "Neutral")
			
		# message(STATUS "${Windows_winmd_DIR}")
		
		# #set(WINRT_OPTIONS "/ZW /D USE_WINRT /AI \"C:\Program Files (x86)\Windows Kits\8.0\References\CommonConfiguration\Neutral\"")
		# #set(WINRT_OPTIONS "/ZW /D USE_WINRT /AI \"$ENV{LIBPATH}\" ")
		# set(WINRT_OPTIONS "/ZW /D USE_WINRT")
	
		# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP ${WINRT_OPTIONS}")
		# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy ${WINRT_OPTIONS}")
		# set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi ${WINRT_OPTIONS}")
		# set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy ${WINRT_OPTIONS}")
		
		# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP ${WINRT_OPTIONS}")
		# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy ${WINRT_OPTIONS}")
		# set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP ${WINRT_OPTIONS}")
		# set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy ${WINRT_OPTIONS}")
	# else()
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP /MTd")
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy /MT")
		set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi /MTd")
		set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy /MT")
		
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP /MTd")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy /MT")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /MTd")
		set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy /MT")
		
		# RB: without /SAFESEH:NO we can't link against ffmpeg libs and VS2013 or we have to build our own
		# libs for newer compilers
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SAFESEH:NO")
	# endif()
	

	add_definitions(-DWIN32 
					-D_WINDOWS
					-D_CRT_SECURE_NO_DEPRECATE
					-D_CRT_NONSTDC_NO_DEPRECATE
					-D_CRT_SECURE_NO_WARNINGS
					-D_MBCS
					#-DUSE_OPENAL
					-DUSE_EXCEPTIONS)
					
	if(NOT CMAKE_CL_64)
		add_definitions(-D_USE_32BIT_TIME_T)
	endif()
					
	if(USE_MFC_TOOLS)
		add_definitions(-D_AFXDLL)
		
		# 0: Use Standard Windows Libraries
		# 1: Use MFC in a Static Library
		# 2: Use MFC in a Shared DLL 
		set(CMAKE_MFC_FLAG 2)
	endif()
endif()

#if(STANDALONE)
#	add_definitions(-DSTANDALONE)
#endif()



#idlib##########################################################################

file(GLOB ID__INCLUDES idlib/*.h)
file(GLOB ID__SOURCES idlib/*.cpp)

file(GLOB ID_BV_INCLUDES idlib/bv/*.h)
file(GLOB ID_BV_SOURCES idlib/bv/*.cpp)

file(GLOB ID_CONTAINERS_INCLUDES idlib/containers/*.h)
file(GLOB ID_CONTAINERS_SOURCES idlib/containers/*.cpp)

file(GLOB ID_GEOMETRY_INCLUDES idlib/geometry/*.h)
file(GLOB ID_GEOMETRY_SOURCES idlib/geometry/*.cpp)

file(GLOB ID_HASHING_INCLUDES idlib/hashing/*.h)
file(GLOB ID_HASHING_SOURCES idlib/hashing/*.cpp)

file(GLOB ID_MATH_INCLUDES idlib/math/*.h)
file(GLOB ID_MATH_SOURCES idlib/math/*.cpp)

file(GLOB ID_SYS_INCLUDES idlib/sys/*.h)
file(GLOB ID_SYS_SOURCES idlib/sys/*.cpp)

if(MSVC)
	file(GLOB ID_SYS_WIN32_INCLUDES idlib/sys/win32/*.h)
	file(GLOB ID_SYS_WIN32_SOURCES idlib/sys/win32/*.cpp)
else()
	file(GLOB ID_SYS_POSIX_INCLUDES idlib/sys/posix/*.h)
	file(GLOB ID_SYS_POSIX_SOURCES idlib/sys/posix/*.cpp)
endif()

set(ID_INCLUDES_ALL
			${ID__INCLUDES}
			${ID_BV_INCLUDES}
			${ID_CONTAINERS_INCLUDES}
			${ID_GEOMETRY_INCLUDES}
			${ID_HASHING_INCLUDES}
			${ID_MATH_INCLUDES}
			${ID_SYS_INCLUDES}
			)
			
set(ID_SOURCES_ALL
			${ID__SOURCES}
			${ID_BV_SOURCES}
			${ID_CONTAINERS_SOURCES}
			${ID_GEOMETRY_SOURCES}
			${ID_HASHING_SOURCES}
			${ID_MATH_SOURCES}
			${ID_SYS_SOURCES}
			)

if(MSVC)
	list(APPEND ID_INCLUDES_ALL ${ID_SYS_WIN32_INCLUDES})
	list(APPEND ID_SOURCES_ALL ${ID_SYS_WIN32_SOURCES})
else()
	list(APPEND ID_INCLUDES_ALL ${ID_SYS_POSIX_INCLUDES})
	list(APPEND ID_SOURCES_ALL ${ID_SYS_POSIX_SOURCES})
endif()

source_group("idlib" FILES ${ID__INCLUDES})
source_group("idlib" FILES ${ID__SOURCES})
source_group("idlib_bv" FILES ${ID_BV_INCLUDES})
source_group("idlib_bv" FILES ${ID_BV_SOURCES})
source_group("idlib_containers" FILES ${ID_CONTAINERS_INCLUDES})
source_group("idlib_containers" FILES ${ID_CONTAINERS_SOURCES})
source_group("idlib_geometry" FILES ${ID_GEOMETRY_INCLUDES})
source_group("idlib_geometry" FILES ${ID_GEOMETRY_SOURCES})
source_group("idlib_hashing" FILES ${ID_HASHING_INCLUDES})
source_group("idlib_hashing" FILES ${ID_HASHING_SOURCES})
source_group("idlib_math" FILES ${ID_MATH_INCLUDES})
source_group("idlib_math" FILES ${ID_MATH_SOURCES})
source_group("idlib_sys" FILES ${ID_SYS_INCLUDES})
source_group("idlib_sys" FILES ${ID_SYS_SOURCES})

if(MSVC)
	source_group("idlib_sys\\win32" FILES ${ID_SYS_WIN32_INCLUDES})
	source_group("idlib_sys\\win32" FILES ${ID_SYS_WIN32_SOURCES})
else()
	source_group("idlib_sys\\posix" FILES ${ID_SYS_POSIX_INCLUDES})
	source_group("idlib_sys\\posix" FILES ${ID_SYS_POSIX_SOURCES})
endif()


add_library(idlib ${ID_SOURCES_ALL} ${ID_INCLUDES_ALL})

#libjpeg########################################################################

if(BUNDLED_JPEG)
  set(JPEG_INCLUDES
    libs/jpeg-6b/jchuff.h
    libs/jpeg-6b/jconfig.h
    libs/jpeg-6b/jdct.h
    libs/jpeg-6b/jdhuff.h
    libs/jpeg-6b/jerror.h
    libs/jpeg-6b/jinclude.h
    libs/jpeg-6b/jmemsys.h
    libs/jpeg-6b/jmorecfg.h
    libs/jpeg-6b/jpegint.h
    libs/jpeg-6b/jpeglib.h
    libs/jpeg-6b/jversion.h)



  set(JPEG_SOURCES
    libs/jpeg-6b/jcapimin.c
    libs/jpeg-6b/jcapistd.c
    libs/jpeg-6b/jctrans.c
    libs/jpeg-6b/jcparam.c
    libs/jpeg-6b/jdatadst.c
    libs/jpeg-6b/jcinit.c
    libs/jpeg-6b/jcmaster.c
    libs/jpeg-6b/jcmarker.c
    libs/jpeg-6b/jcmainct.c
    libs/jpeg-6b/jcprepct.c
    libs/jpeg-6b/jccoefct.c
    libs/jpeg-6b/jccolor.c
    libs/jpeg-6b/jcsample.c
    libs/jpeg-6b/jchuff.c
    libs/jpeg-6b/jcphuff.c
    libs/jpeg-6b/jcdctmgr.c
    libs/jpeg-6b/jfdctfst.c
    libs/jpeg-6b/jfdctflt.c
    libs/jpeg-6b/jfdctint.c
    libs/jpeg-6b/jdapimin.c
    libs/jpeg-6b/jdapistd.c
    libs/jpeg-6b/jdtrans.c
    libs/jpeg-6b/jdatasrc.c
    libs/jpeg-6b/jdmaster.c
    libs/jpeg-6b/jdinput.c
    libs/jpeg-6b/jdmarker.c
    libs/jpeg-6b/jdhuff.c
    libs/jpeg-6b/jdphuff.c
    libs/jpeg-6b/jdmainct.c
    libs/jpeg-6b/jdcoefct.c
    libs/jpeg-6b/jdpostct.c
    libs/jpeg-6b/jddctmgr.c
    libs/jpeg-6b/jidctfst.c
    libs/jpeg-6b/jidctflt.c
    libs/jpeg-6b/jidctint.c
    libs/jpeg-6b/jidctred.c
    libs/jpeg-6b/jdsample.c
    libs/jpeg-6b/jdcolor.c
    libs/jpeg-6b/jquant1.c
    libs/jpeg-6b/jquant2.c
    libs/jpeg-6b/jdmerge.c
    libs/jpeg-6b/jcomapi.c
    libs/jpeg-6b/jutils.c
    libs/jpeg-6b/jerror.c
    libs/jpeg-6b/jmemmgr.c
    libs/jpeg-6b/jmemnobs.c)
  include_directories(libs/jpeg-6b)
  add_library(jpeg ${JPEG_SOURCES} ${JPEG_INCLUDES})
  set(JPEG_LIBRARY jpeg)
else()
  set(JPEG_LIBRARY jpeg)
endif()

#libpng#########################################################################

if(BUNDLED_PNG)	
  file(GLOB PNG_INCLUDES libs/png/*.h)
  file(GLOB PNG_SOURCES libs/png/*.c)
  include_directories(libs/png)
  add_library(png ${PNG_SOURCES} ${PNG_INCLUDES})
  set(PNG_LIBRARY png)
else()
  set(PNG_LIBRARY png)
endif()


#libz###########################################################################

file(GLOB ZLIB_INCLUDES libs/zlib/*.h)
file(GLOB ZLIB_SOURCES libs/zlib/*.c)

if(BUNDLED_ZLIB)
  add_library(zlib ${ZLIB_SOURCES} ${ZLIB_INCLUDES})
  set(ZLIB_LIBRARY zlib)
endif()

#minizip########################################################################

file(GLOB MINIZIP_INCLUDES libs/zlib/minizip/*.h)
file(GLOB MINIZIP_SOURCES libs/zlib/minizip/*.c libs/zlib/minizip/*.cpp)

if(BUNDLED_MINIZIP)
  add_library(minizip ${MINIZIP_SOURCES} ${MINIZIP_INCLUDES})
  set(MINIZIP_LIBRARY minizip)
endif()

#irrxml#########################################################################

file(GLOB IRRXML_INCLUDES libs/irrxml/src/*.h)
file(GLOB IRRXML_SOURCES libs/irrxml/src/*.cpp)

if(BUNDLED_IRRXML)
  add_library(irrxml ${IRRXML_SOURCES} ${IRRXML_INCLUDES})
  set(IRRXML_LIBRARY irrxml)
endif()

#glew###########################################################################

set(GLEW_INCLUDES
	libs/glew/include/GL/glew.h)
	
if(WIN32)
	set(GLEW_INCLUDES ${GLEW_INCLUDES} libs/glew/include/GL/wglew.h)
else()
	set(GLEW_INCLUDES ${GLEW_INCLUDES} libs/glew/include/GL/glxew.h)
endif()
	
set(GLEW_SOURCES
	libs/glew/src/glew.c)

if(BUNDLED_GLEW)
  add_library(glew ${GLEW_SOURCES} ${GLEW_INCLUDES})
  set(GLEW_LIBRARY glew)
endif()

#freetype#######################################################################	

set(FREETYPE_SOURCES
	libs/freetype/src/autofit/autofit.c
	libs/freetype/src/bdf/bdf.c
	libs/freetype/src/cff/cff.c
	libs/freetype/src/base/ftbase.c
	libs/freetype/src/base/ftbitmap.c
	libs/freetype/src/cache/ftcache.c
	libs/freetype/src/base/ftdebug.c
	libs/freetype/src/base/ftgasp.c
	libs/freetype/src/base/ftglyph.c
	libs/freetype/src/gzip/ftgzip.c
	libs/freetype/src/base/ftinit.c
	libs/freetype/src/lzw/ftlzw.c
	libs/freetype/src/base/ftstroke.c
	libs/freetype/src/base/ftsystem.c
	libs/freetype/src/smooth/smooth.c
	libs/freetype/src/base/ftbbox.c
	libs/freetype/src/base/ftmm.c
	libs/freetype/src/base/ftpfr.c
	libs/freetype/src/base/ftsynth.c
	libs/freetype/src/base/fttype1.c
	libs/freetype/src/base/ftwinfnt.c
	libs/freetype/src/pcf/pcf.c
	libs/freetype/src/pfr/pfr.c
	libs/freetype/src/psaux/psaux.c
	libs/freetype/src/pshinter/pshinter.c
	libs/freetype/src/psnames/psmodule.c
	libs/freetype/src/raster/raster.c
	libs/freetype/src/sfnt/sfnt.c
	libs/freetype/src/truetype/truetype.c
	libs/freetype/src/type1/type1.c
	libs/freetype/src/cid/type1cid.c
	libs/freetype/src/type42/type42.c
	libs/freetype/src/winfonts/winfnt.c)

if(BUNDLED_FREETYPE)
  add_library(freetype ${FREETYPE_SOURCES})
  set(FREETYPE_LIBRARY freetype)
endif()

#oggvorbis######################################################################

set(OGGVORBIS_INCLUDES
	libs/oggvorbis/ogg/ogg.h
	libs/oggvorbis/ogg/os_types.h)
	
set(OGGVORBIS_SOURCES
	libs/oggvorbis/oggsrc/bitwise.c
	libs/oggvorbis/oggsrc/framing.c
	
	libs/oggvorbis/vorbissrc/mdct.c
	libs/oggvorbis/vorbissrc/smallft.c
	libs/oggvorbis/vorbissrc/block.c
	libs/oggvorbis/vorbissrc/envelope.c
	libs/oggvorbis/vorbissrc/windowvb.c
	libs/oggvorbis/vorbissrc/lsp.c
	libs/oggvorbis/vorbissrc/lpc.c
	libs/oggvorbis/vorbissrc/analysis.c
	libs/oggvorbis/vorbissrc/synthesis.c
	libs/oggvorbis/vorbissrc/psy.c
	libs/oggvorbis/vorbissrc/info.c
	libs/oggvorbis/vorbissrc/floor1.c
	libs/oggvorbis/vorbissrc/floor0.c
	libs/oggvorbis/vorbissrc/res0.c
	libs/oggvorbis/vorbissrc/mapping0.c
	libs/oggvorbis/vorbissrc/registry.c
	libs/oggvorbis/vorbissrc/codebook.c
	libs/oggvorbis/vorbissrc/sharedbook.c
	libs/oggvorbis/vorbissrc/lookup.c
	libs/oggvorbis/vorbissrc/bitrate.c
	libs/oggvorbis/vorbissrc/vorbisfile.c)

if(BUNDLED_OGGVORBIS)
  add_library(oggvorbis ${OGGVORBIS_SOURCES} ${OGGVORBIS_INCLUDES})
  set(OGGVORBIS_LIBRARY oggvorbis)
endif()

#timidity#######################################################################

file(GLOB TIMIDITY_INCLUDES libs/timidity/*.h)
file(GLOB TIMIDITY_SOURCES libs/timidity/*.cpp)

if(BUNDLED_TIMIDITY)
  add_library(timidity ${TIMIDITY_SOURCES} ${TIMIDITY_INCLUDES})
  set(TIMIDITY_LIBRARY timidity)
endif()

#idtools########################################################################

# not a library, but still optional

file(GLOB COMPILER_INCLUDES tools/compilers/*.h)

file(GLOB COMPILER_AAS_INCLUDES tools/compilers/aas/*.h)
file(GLOB COMPILER_AAS_SOURCES tools/compilers/aas/*.cpp)

file(GLOB COMPILER_DMAP_INCLUDES tools/compilers/dmap/*.h)
file(GLOB COMPILER_DMAP_SOURCES tools/compilers/dmap/*.cpp)

file(GLOB COMPILER_RENDERBUMP_INCLUDES tools/compilers/renderbump/*.h)
file(GLOB COMPILER_RENDERBUMP_SOURCES tools/compilers/renderbump/*.cpp)

file(GLOB COMPILER_ROQVQ_INCLUDES tools/compilers/roqvq/*.h)
file(GLOB COMPILER_ROQVQ_SOURCES tools/compilers/roqvq/*.cpp)

set(TOOLS_INCLUDES tools/edit_public.h)

file(GLOB TOOLS_COMAFX_INCLUDES tools/comafx/*.h)
file(GLOB TOOLS_COMAFX_SOURCES tools/comafx/*.cpp)

set(TOOLS_COMMON_INCLUDES
	tools/common/ColorButton.h
	tools/common/DialogHelpers.h
	tools/common/MaskEdit.h
	tools/common/OpenFileDialog.h
	tools/common/PropertyGrid.h
	tools/common/RegistryOptions.h
	#tools/common/RenderBumpFlatDialog.h
	tools/common/RollupPanel.h
	tools/common/SpinButton.h)

set(TOOLS_COMMON_SOURCES
	tools/common/AlphaPopup.cpp
	tools/common/ColorButton.cpp
	tools/common/MaskEdit.cpp
	tools/common/OpenFileDialog.cpp
	tools/common/PropertyGrid.cpp
	tools/common/RegistryOptions.cpp
	#tools/common/RenderBumpFlatDialog.cpp
	tools/common/RollupPanel.cpp
	tools/common/SpinButton.cpp)

file(GLOB TOOLS_COMMON_PROPTREE_INCLUDES tools/common/PropTree/*.h)
file(GLOB TOOLS_COMMON_PROPTREE_SOURCES tools/common/PropTree/*.cpp)

file(GLOB EDITOR_AF_INCLUDES tools/af/*.h)
file(GLOB EDITOR_AF_SOURCES tools/af/*.cpp)

set(EDITOR_DECL_INCLUDES
	tools/decl/DialogDeclBrowser.h
	tools/decl/DialogDeclEditor.h
	tools/decl/DialogDeclNew.h)
	
set(EDITOR_DECL_SOURCES
	tools/decl/DialogDeclBrowser.cpp
	tools/decl/DialogDeclEditor.cpp
	tools/decl/DialogDeclNew.cpp)
	
file(GLOB EDITOR_DEBUGGER_INCLUDES tools/debugger/*.h)
file(GLOB EDITOR_DEBUGGER_SOURCES tools/debugger/*.cpp)

file(GLOB EDITOR_GUIED_INCLUDES tools/guied/*.h)
set(EDITOR_GUIED_SOURCES
	tools/guied/GEApp.cpp
	tools/guied/GECheckInDlg.cpp
	tools/guied/GEDeleteModifier.cpp
	tools/guied/GEHideModifier.cpp
	tools/guied/GEInsertModifier.cpp
	tools/guied/GEItemPropsDlg.cpp
	tools/guied/GEItemScriptsDlg.cpp
	tools/guied/GEKeyValueModifier.cpp
	tools/guied/GEModifier.cpp
	tools/guied/GEModifierGroup.cpp
	tools/guied/GEModifierStack.cpp
	tools/guied/GEMoveModifier.cpp
	tools/guied/GENavigator.cpp
	tools/guied/GEOptions.cpp
	tools/guied/GEOptionsDlg.cpp
	tools/guied/GEProperties.cpp
	tools/guied/GEPropertyPage.cpp
	tools/guied/GESelectionMgr.cpp
	tools/guied/GESizeModifier.cpp
	tools/guied/GEStateModifier.cpp
	tools/guied/GEStatusBar.cpp
	tools/guied/GETransformer.cpp
	tools/guied/GEViewer.cpp
	tools/guied/GEWindowWrapper.cpp
	#tools/guied/GEWindowWrapper_stub.cpp
	tools/guied/GEWorkspace.cpp
	tools/guied/GEWorkspaceFile.cpp
	tools/guied/GEZOrderModifier.cpp
	tools/guied/guied.cpp)

file(GLOB EDITOR_MATERIALEDITOR_INCLUDES tools/materialeditor/*.h)
file(GLOB EDITOR_MATERIALEDITOR_SOURCES tools/materialeditor/*.cpp)

file(GLOB EDITOR_PARTICLE_INCLUDES tools/particle/*.h)
file(GLOB EDITOR_PARTICLE_SOURCES tools/particle/*.cpp)

file(GLOB EDITOR_PDA_INCLUDES tools/pda/*.h)
file(GLOB EDITOR_PDA_SOURCES tools/pda/*.cpp)

file(GLOB EDITOR_RADIANT_INCLUDES tools/radiant/*.h)
file(GLOB EDITOR_RADIANT_SOURCES tools/radiant/*.cpp)

file(GLOB EDITOR_SCRIPT_INCLUDES tools/script/*.h)
file(GLOB EDITOR_SCRIPT_SOURCES tools/script/*.cpp)

file(GLOB EDITOR_SOUND_INCLUDES tools/sound/*.h)
file(GLOB EDITOR_SOUND_SOURCES tools/sound/*.cpp)


source_group("tools\\compilers" FILES ${COMPILER_INCLUDES})

source_group("tools\\compilers\\aas" FILES ${COMPILER_AAS_INCLUDES})
source_group("tools\\compilers\\aas" FILES ${COMPILER_AAS_SOURCES})

source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_INCLUDES})
source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_SOURCES})

source_group("tools\\compilers\\renderbump" FILES ${COMPILER_RENDERBUMP_INCLUDES})
source_group("tools\\compilers\\renderbump" FILES ${COMPILER_RENDERBUMP_SOURCES})

source_group("tools\\compilers\\roqvq" FILES ${COMPILER_ROQVQ_INCLUDES})
source_group("tools\\compilers\\roqvq" FILES ${COMPILER_ROQVQ_SOURCES})

source_group("tools" FILES ${TOOLS_INCLUDES})

source_group("tools\\comafx" FILES ${TOOLS_COMAFX_INCLUDES})
source_group("tools\\comafx" FILES ${TOOLS_COMAFX_SOURCES})

source_group("tools\\common" FILES ${TOOLS_COMMON_INCLUDES})
source_group("tools\\common" FILES ${TOOLS_COMMON_SOURCES})

source_group("tools\\common\\proptree" FILES ${TOOLS_COMMON_PROPTREE_INCLUDES})
source_group("tools\\common\\proptree" FILES ${TOOLS_COMMON_PROPTREE_SOURCES})

source_group("tools\\af" FILES ${EDITOR_AF_INCLUDES})
source_group("tools\\af" FILES ${EDITOR_AF_SOURCES})

source_group("tools\\debugger" FILES ${EDITOR_DEBUGGER_INCLUDES})
source_group("tools\\debugger" FILES ${EDITOR_DEBUGGER_SOURCES})

source_group("tools\\decl" FILES ${EDITOR_DECL_INCLUDES})
source_group("tools\\decl" FILES ${EDITOR_DECL_SOURCES})

source_group("tools\\guied" FILES ${EDITOR_GUIED_INCLUDES})
source_group("tools\\guied" FILES ${EDITOR_GUIED_SOURCES})

source_group("tools\\materialeditor" FILES ${EDITOR_MATERIALEDITOR_INCLUDES})
source_group("tools\\materialeditor" FILES ${EDITOR_MATERIALEDITOR_SOURCES})

source_group("tools\\particle" FILES ${EDITOR_PARTICLE_INCLUDES})
source_group("tools\\particle" FILES ${EDITOR_PARTICLE_SOURCES})

source_group("tools\\pda" FILES ${EDITOR_PDA_INCLUDES})
source_group("tools\\pda" FILES ${EDITOR_PDA_SOURCES})

source_group("tools\\radiant" FILES ${EDITOR_RADIANT_INCLUDES})
source_group("tools\\radiant" FILES ${EDITOR_RADIANT_SOURCES})

source_group("tools\\script" FILES ${EDITOR_SCRIPT_INCLUDES})
source_group("tools\\script" FILES ${EDITOR_SCRIPT_SOURCES})

source_group("tools\\sound" FILES ${EDITOR_SOUND_INCLUDES})
source_group("tools\\sound" FILES ${EDITOR_SOUND_SOURCES})

set(IDTOOLS_INCLUDES
			#${TOOLS_INCLUDES}
			#${TOOLS_COMAFX_INCLUDES}
			#${TOOLS_COMMON_INCLUDES}
			#${TOOLS_COMMON_PROPTREE_INCLUDES}
			#${EDITOR_AF_INCLUDES}
			#${EDITOR_DEBUGGER_INCLUDES}
			#${EDITOR_DECL_INCLUDES}
			#${EDITOR_GUIED_INCLUDES}
			#${EDITOR_MATERIALEDITOR_INCLUDES}
			#${EDITOR_PARTICLE_INCLUDES}
			#${EDITOR_PDA_INCLUDES}
			#${EDITOR_RADIANT_INCLUDES}
			#${EDITOR_SCRIPT_INCLUDES}
			#${EDITOR_SOUND_INCLUDES}
			${COMPILER_INCLUDES}
			${COMPILER_AAS_INCLUDES} ${COMPILER_AAS_SOURCES}
			${COMPILER_DMAP_INCLUDES} ${COMPILER_DMAP_SOURCES}
			#${COMPILER_RENDERBUMP_INCLUDES} ${COMPILER_RENDERBUMP_SOURCES}
			#${COMPILER_ROQVQ_INCLUDES} ${COMPILER_ROQVQ_SOURCES}
			)
			
set(IDTOOLS_SOURCES
			#${TOOLS_COMAFX_SOURCES}
			#${TOOLS_COMMON_SOURCES}
			#${TOOLS_COMMON_PROPTREE_SOURCES}
			#${EDITOR_AF_SOURCES}
			#${EDITOR_DEBUGGER_SOURCES}
			#${EDITOR_DECL_SOURCES}
			#${EDITOR_GUIED_SOURCES}
			#${EDITOR_MATERIALEDITOR_SOURCES}
			#${EDITOR_PARTICLE_SOURCES}
			#${EDITOR_PDA_SOURCES}
			#${EDITOR_RADIANT_SOURCES}
			#${EDITOR_SCRIPT_SOURCES}
			#${EDITOR_SOUND_SOURCES}
			${COMPILER_AAS_SOURCES}
			${COMPILER_DMAP_SOURCES}
			#${COMPILER_RENDERBUMP_SOURCES}
			#${COMPILER_ROQVQ_SOURCES}
			)

################################################################################

file(GLOB AAS_INCLUDES aas/*.h)
file(GLOB AAS_SOURCES aas/*.cpp)

file(GLOB_RECURSE CM_INCLUDES cm/*.h)
file(GLOB_RECURSE CM_SOURCES cm/*.cpp)

file(GLOB FRAMEWORK_INCLUDES framework/*.h)
file(GLOB FRAMEWORK_SOURCES framework/*.cpp)

file(GLOB FRAMEWORK_ASYNC_INCLUDES framework/async/*.h)
file(GLOB FRAMEWORK_ASYNC_SOURCES framework/async/*.cpp)

file(GLOB RENDERER_INCLUDES renderer/*.h)
file(GLOB RENDERER_SOURCES renderer/*.cpp)
	
file(GLOB RENDERER_COLOR_INCLUDES renderer/Color/*.h)
file(GLOB RENDERER_COLOR_SOURCES renderer/Color/*.cpp)

file(GLOB RENDERER_DXT_INCLUDES renderer/DXT/*.h)
file(GLOB RENDERER_DXT_SOURCES renderer/DXT/*.cpp)

file(GLOB RENDERER_JOBS_INCLUDES renderer/jobs/*.h)
file(GLOB RENDERER_JOBS_SOURCES renderer/jobs/*.cpp)

file(GLOB RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES renderer/jobs/dynamicshadowvolume/*.h)
file(GLOB RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES renderer/jobs/dynamicshadowvolume/*.cpp)

file(GLOB RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES renderer/jobs/prelightshadowvolume/*.h)
file(GLOB RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES renderer/jobs/prelightshadowvolume/*.cpp)

file(GLOB RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES renderer/jobs/staticshadowvolume/*.h)
file(GLOB RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES renderer/jobs/staticshadowvolume/*.cpp)

file(GLOB RENDERER_OPENGL_INCLUDES renderer/OpenGL/*.h)
file(GLOB RENDERER_OPENGL_SOURCES renderer/OpenGL/*.cpp)
	
set(SOUND_INCLUDES
	sound/snd_defines.h
	sound/snd_local.h
	sound/sound.h
	sound/SoundVoice.h
	sound/WaveFile.h)
	
set(SOUND_SOURCES
	#sound/snd_cache.cpp
	#sound/snd_decoder.cpp
	#sound/snd_efxfile.cpp
	sound/snd_emitter.cpp
	sound/snd_shader.cpp
	sound/snd_system.cpp
	sound/snd_world.cpp
	sound/SoundVoice.cpp
	sound/WaveFile.cpp
	)
	
set(XAUDIO2_INCLUDES
	sound/XAudio2/XA2_SoundHardware.h
	sound/XAudio2/XA2_SoundSample.h
	sound/XAudio2/XA2_SoundVoice.h)
	
set(XAUDIO2_SOURCES
	sound/XAudio2/XA2_SoundHardware.cpp
	sound/XAudio2/XA2_SoundSample.cpp
	sound/XAudio2/XA2_SoundVoice.cpp)
	
set(OPENAL_INCLUDES
	sound/OpenAL/AL_SoundHardware.h
	sound/OpenAL/AL_SoundSample.h
	sound/OpenAL/AL_SoundVoice.h)
	
set(OPENAL_SOURCES
	sound/OpenAL/AL_SoundHardware.cpp
	sound/OpenAL/AL_SoundSample.cpp
	sound/OpenAL/AL_SoundVoice.cpp)

set(STUBAUDIO_INCLUDES
	sound/stub/SoundStub.h)

set(STUBAUDIO_SOURCES
	sound/stub/SoundSample.cpp)
	
file(GLOB SYS_INCLUDES sys/*.h)
file(GLOB SYS_SOURCES sys/*.cpp)

file(GLOB UI_INCLUDES ui/*.h)
file(GLOB UI_SOURCES ui/*.cpp)

file(GLOB SWF_INCLUDES swf/*.h)
file(GLOB SWF_SOURCES swf/*.cpp)


set(GAMED3XP_INCLUDES
	d3xp/Achievements.h
	d3xp/Actor.h
	d3xp/AF.h
	d3xp/AFEntity.h
	d3xp/AimAssist.h
	d3xp/BrittleFracture.h
	d3xp/Camera.h
	#d3xp/EndLevel.h
	d3xp/Entity.h
	d3xp/Fx.h
	d3xp/Game.h
	d3xp/GameEdit.h
	d3xp/Game_local.h
	d3xp/Grabber.h
	d3xp/IK.h
	d3xp/Item.h
	d3xp/Leaderboards.h
	d3xp/Light.h
	d3xp/Misc.h
	d3xp/Moveable.h
	d3xp/Mover.h
	d3xp/MultiplayerGame.h
	d3xp/Player.h
	d3xp/PlayerIcon.h
	d3xp/PlayerView.h
	d3xp/PredictedValue.h
	d3xp/Projectile.h
	d3xp/Pvs.h
	d3xp/SecurityCamera.h
	d3xp/SmokeParticles.h
	d3xp/Sound.h
	d3xp/Target.h
	d3xp/Trigger.h
	d3xp/Weapon.h
	d3xp/WorldSpawn.h)
	
set(GAMED3XP_SOURCES
	d3xp/Achievements.cpp
	d3xp/Actor.cpp
	d3xp/AF.cpp
	d3xp/AFEntity.cpp
	d3xp/AimAssist.cpp
	d3xp/BrittleFracture.cpp
	d3xp/Camera.cpp
	#d3xp/EndLevel.cpp
	d3xp/Entity.cpp
	d3xp/Fx.cpp
	d3xp/GameEdit.cpp
	d3xp/Game_local.cpp
	d3xp/Game_network.cpp
	d3xp/Grabber.cpp
	d3xp/IK.cpp
	d3xp/Item.cpp
	d3xp/Leaderboards.cpp
	d3xp/Light.cpp
	d3xp/Misc.cpp
	d3xp/Moveable.cpp
	d3xp/Mover.cpp
	d3xp/MultiplayerGame.cpp
	d3xp/Player.cpp
	d3xp/PlayerIcon.cpp
	d3xp/PlayerView.cpp
	d3xp/precompiled.cpp
	d3xp/Projectile.cpp
	d3xp/Pvs.cpp
	d3xp/SecurityCamera.cpp
	d3xp/SmokeParticles.cpp
	d3xp/Sound.cpp
	d3xp/Target.cpp
	d3xp/Trigger.cpp
	d3xp/Weapon.cpp
	d3xp/WorldSpawn.cpp)

file(GLOB GAMED3XP_AI_INCLUDES d3xp/ai/*.h)
file(GLOB GAMED3XP_AI_SOURCES d3xp/ai/*.cpp)

file(GLOB GAMED3XP_ANIM_INCLUDES d3xp/anim/*.h)
file(GLOB GAMED3XP_ANIM_SOURCES d3xp/anim/*.cpp)

file(GLOB GAMED3XP_GAMESYS_INCLUDES d3xp/gamesys/*.h)
set(GAMED3XP_GAMESYS_SOURCES
	#d3xp/gamesys/Callbacks.cpp
	d3xp/gamesys/Class.cpp
	#d3xp/gamesys/DebugGraph.cpp
	d3xp/gamesys/Event.cpp
	d3xp/gamesys/SaveGame.cpp
	d3xp/gamesys/SysCmds.cpp
	d3xp/gamesys/SysCvar.cpp
	#d3xp/gamesys/TypeInfo.cpp
	)

file(GLOB GAMED3XP_MENUS_INCLUDES d3xp/menus/*.h)
file(GLOB GAMED3XP_MENUS_SOURCES d3xp/menus/*.cpp)
	
file(GLOB GAMED3XP_PHYSICS_INCLUDES d3xp/physics/*.h)
file(GLOB GAMED3XP_PHYSICS_SOURCES d3xp/physics/*.cpp)

file(GLOB GAMED3XP_SCRIPT_INCLUDES d3xp/script/*.h)
file(GLOB GAMED3XP_SCRIPT_SOURCES d3xp/script/*.cpp)

file(GLOB DOOMCLASSIC_INCLUDES ../doomclassic/doom/*.h)
file(GLOB DOOMCLASSIC_SOURCES ../doomclassic/doom/*.cpp)

set(DOOMCLASSIC_STUBAUDIO_SOURCES ../doomclassic/doom/i_sound_stub.cpp)
set(DOOMCLASSIC_OPENAL_SOURCES ../doomclassic/doom/i_sound_openal.cpp)
set(DOOMCLASSIC_XAUDIO2_SOURCES ../doomclassic/doom/i_sound_win32.cpp)

list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/doom/i_sound_stub.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/doom/i_sound_openal.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/doom/i_sound_win32.cpp)

file(GLOB_RECURSE WIN32_INCLUDES sys/win32/*.h)
#file(GLOB_RECURSE WIN32_SOURCES sys/win32/*.cpp)

set(WIN32_SOURCES
	sys/win32/win_glimp.cpp
	sys/win32/win_input.cpp
	sys/win32/win_main.cpp
	sys/win32/win_shared.cpp
	sys/win32/win_snd.cpp
	sys/win32/win_syscon.cpp
	sys/win32/win_taskkeyhook.cpp
	sys/win32/win_wndproc.cpp)
	
if(MSVC)
	list(APPEND WIN32_SOURCES sys/win32/win_cpu.cpp)
endif()

set(WIN32_RESOURCES
	# sys/win32/rc/res/BEVEL.BMP
	# sys/win32/rc/res/BITMAP2.BMP
	# sys/win32/rc/res/BMP00001.BMP
	# sys/win32/rc/res/BMP0002.BMP
	# sys/win32/rc/res/DEFTEX.WAL
	# sys/win32/rc/res/ENDCAP.BMP
	# sys/win32/rc/res/GetString.htm
	# sys/win32/rc/res/IBEVEL.BMP
	# sys/win32/rc/res/IENDCAP.BMP
	# sys/win32/rc/res/MEFileToolbar.bmp
	# sys/win32/rc/res/MEtoolbar.bmp
	# sys/win32/rc/res/MaterialEditor.ico
	# sys/win32/rc/res/PropTree.rc2
	# sys/win32/rc/res/Q.BMP
	# sys/win32/rc/res/RADIANT3.GIF
	# sys/win32/rc/res/Radiant.ico
	# sys/win32/rc/res/RadiantDoc.ico
	# sys/win32/rc/res/TOOLBAR1.BMP
	# sys/win32/rc/res/TOOLBAR2.BMP
	# sys/win32/rc/res/Toolbar.bmp
	# sys/win32/rc/res/VIEWDEFA.BMP
	# sys/win32/rc/res/VIEWOPPO.BMP
	# sys/win32/rc/res/bmp00002.bmp
	# sys/win32/rc/res/bmp00003.bmp
	# sys/win32/rc/res/bmp00004.bmp
	# sys/win32/rc/res/bmp00005.bmp
	# sys/win32/rc/res/cchsb.bmp
	# sys/win32/rc/res/ccrgb.bmp
	# sys/win32/rc/res/dbg_back.bmp
	# sys/win32/rc/res/dbg_breakpoint.ico
	# sys/win32/rc/res/dbg_current.ico
	# sys/win32/rc/res/dbg_currentline.ico
	# sys/win32/rc/res/dbg_empty.ico
	# sys/win32/rc/res/dbg_open.bmp
	# sys/win32/rc/res/dbg_toolbar.bmp
	sys/win32/rc/res/doom.ico
	# sys/win32/rc/res/fpoint.cur
	# sys/win32/rc/res/fxed_link.ico
	# sys/win32/rc/res/fxed_toolbar.bmp
	# sys/win32/rc/res/fxeditor.ico
	# sys/win32/rc/res/guied.ico
	# sys/win32/rc/res/guied_collapse.ico
	# sys/win32/rc/res/guied_expand.ico
	# sys/win32/rc/res/guied_hand.cur
	# sys/win32/rc/res/guied_nav_visible.ico
	# sys/win32/rc/res/guied_nav_visibledisabled.ico
	# sys/win32/rc/res/guied_scripts.ico
	# sys/win32/rc/res/guied_scripts_white.ico
	# sys/win32/rc/res/guied_viewer_toolbar.bmp
	# sys/win32/rc/res/icon2.ico
	# sys/win32/rc/res/logo_sm3dfx.bmp
	# sys/win32/rc/res/matedtree.bmp
	# sys/win32/rc/res/me_disabled_icon.ico
	# sys/win32/rc/res/me_enabled.ico
	# sys/win32/rc/res/me_off_icon.ico
	# sys/win32/rc/res/me_on_icon.ico
	# sys/win32/rc/res/qe3.ico
	# sys/win32/rc/res/shaderbar.bmp
	# sys/win32/rc/res/shaderdoc.ico
	# sys/win32/rc/res/shaderframe.ico
	# sys/win32/rc/res/spliter.cur
	)


#if(USE_MFC_TOOLS)
	list(APPEND WIN32_RESOURCES sys/win32/rc/doom.rc)
#else()
#	list(APPEND WIN32_RESOURCES sys/win32/rc/doom_nomfc.rc)
#endif()


file(GLOB POSIX_INCLUDES sys/posix/*.h)
file(GLOB POSIX_SOURCES sys/posix/*.cpp)

file(GLOB COMMON_INCLUDES sys/common/*.h)
file(GLOB COMMON_SOURCES sys/common/*.cpp)

file(GLOB SDL_INCLUDES sys/sdl/*.h)
file(GLOB SDL_SOURCES sys/sdl/*.cpp)


source_group("aas" FILES ${AAS_INCLUDES})
source_group("aas" FILES ${AAS_SOURCES})

source_group("cm" FILES ${CM_INCLUDES})
source_group("cm" FILES ${CM_SOURCES})

source_group("framework" FILES ${FRAMEWORK_INCLUDES})
source_group("framework" FILES ${FRAMEWORK_SOURCES})

source_group("framework\\async" FILES ${FRAMEWORK_ASYNC_INCLUDES})
source_group("framework\\async" FILES ${FRAMEWORK_ASYNC_SOURCES})

source_group("renderer" FILES ${RENDERER_INCLUDES})
source_group("renderer" FILES ${RENDERER_SOURCES})

source_group("renderer\\Color" FILES ${RENDERER_COLOR_INCLUDES})
source_group("renderer\\Color" FILES ${RENDERER_COLOR_SOURCES})

source_group("renderer\\DXT" FILES ${RENDERER_DXT_INCLUDES})
source_group("renderer\\DXT" FILES ${RENDERER_DXT_SOURCES})

source_group("renderer\\jobs" FILES ${RENDERER_JOBS_INCLUDES})
source_group("renderer\\jobs" FILES ${RENDERER_JOBS_SOURCES})

source_group("renderer\\jobs\\dynamicshadowvolume" FILES ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\dynamicshadowvolume" FILES ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES})

source_group("renderer\\jobs\\prelightshadowvolume" FILES ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\prelightshadowvolume" FILES ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES})

source_group("renderer\\jobs\\staticshadowvolume" FILES ${RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\staticshadowvolume" FILES ${RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES})

source_group("renderer\\OpenGL" FILES ${RENDERER_OPENGL_INCLUDES})
source_group("renderer\\OpenGL" FILES ${RENDERER_OPENGL_SOURCES})

source_group("sound" FILES ${SOUND_INCLUDES})
source_group("sound" FILES ${SOUND_SOURCES})

source_group("sound\\XAudio2" FILES ${XAUDIO2_INCLUDES})
source_group("sound\\XAudio2" FILES ${XAUDIO2_SOURCES})

source_group("sound\\OpenAL" FILES ${OPENAL_INCLUDES})
source_group("sound\\OpenAL" FILES ${OPENAL_SOURCES})

source_group("sound\\stub" FILES ${STUBAUDIO_INCLUDES})
source_group("sound\\stub" FILES ${STUBAUDIO_SOURCES})

source_group("ui" FILES ${UI_INCLUDES})
source_group("ui" FILES ${UI_SOURCES})

source_group("swf" FILES ${SWF_INCLUDES})
source_group("swf" FILES ${SWF_SOURCES})

source_group("sys" FILES ${SYS_INCLUDES})
source_group("sys" FILES ${SYS_SOURCES})

source_group("doomclassic" FILES ${DOOMCLASSIC_INCLUDES})
source_group("doomclassic" FILES ${DOOMCLASSIC_SOURCES})

source_group("doomclassic" FILES ${DOOMCLASSIC_STUBAUDIO_SOURCES})
source_group("doomclassic" FILES ${DOOMCLASSIC_OPENAL_SOURCES})
source_group("doomclassic" FILES ${DOOMCLASSIC_XAUDIO2_SOURCES})

source_group("game-d3xp" FILES ${GAMED3XP_INCLUDES})
source_group("game-d3xp" FILES ${GAMED3XP_SOURCES})

source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_INCLUDES})
source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_SOURCES})

source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_INCLUDES})
source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_SOURCES})

source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_INCLUDES})
source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_SOURCES})

source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_INCLUDES})
source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_SOURCES})

source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_INCLUDES})
source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_SOURCES})

source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_INCLUDES})
source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_SOURCES})

source_group("sys\\win32" FILES ${WIN32_INCLUDES})
source_group("sys\\win32" FILES ${WIN32_SOURCES})
source_group("sys\\win32\\Resources" FILES ${WIN32_RESOURCES})

source_group("sys\\posix" FILES ${POSIX_INCLUDES})
source_group("sys\\posix" FILES ${POSIX_SOURCES})

source_group("sys\\common" FILES ${COMMON_INCLUDES})
source_group("sys\\common" FILES ${COMMON_SOURCES})

source_group("sys\\sdl" FILES ${SDL_INCLUDES})
source_group("sys\\sdl" FILES ${SDL_SOURCES})

set(RBDOOM3_INCLUDES
			${AAS_INCLUDES}
			${CM_INCLUDES}
			${FRAMEWORK_INCLUDES}
			${FRAMEWORK_ASYNC_INCLUDES}
			${RENDERER_INCLUDES}
			${RENDERER_COLOR_INCLUDES}
			${RENDERER_DXT_INCLUDES}
			${RENDERER_JOBS_INCLUDES}
			${RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES}
			${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES}
			${RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES}
			${RENDERER_OPENGL_INCLUDES}
			${SOUND_INCLUDES}
			${UI_INCLUDES}
			${SWF_INCLUDES}
			${COMMON_INCLUDES}
			${DOOMCLASSIC_INCLUDES}
			
			)
			
set(RBDOOM3_SOURCES
			${AAS_SOURCES}
			${CM_SOURCES}
			${FRAMEWORK_SOURCES}
			${FRAMEWORK_ASYNC_SOURCES} 
			${RENDERER_SOURCES}
			${RENDERER_COLOR_SOURCES}
			${RENDERER_DXT_SOURCES}
			${RENDERER_JOBS_SOURCES}
			${RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES}
			${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES}
			${RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES}
			${RENDERER_OPENGL_SOURCES}
			${SOUND_SOURCES}
			${UI_SOURCES}
			${SWF_SOURCES}
			${COMMON_SOURCES}
			${DOOMCLASSIC_SOURCES}
			)

add_definitions(-DUSE_DOOMCLASSIC)

add_definitions(-D__DOOM__
				-DGLEW_STATIC
				#-DBUILD_FREETYPE
				#-DFT2_BUILD_LIBRARY
				)
				
if(MONOLITH)
	list(APPEND RBDOOM3_INCLUDES
		${GAMED3XP_INCLUDES}
		${GAMED3XP_AI_INCLUDES} 
		${GAMED3XP_ANIM_INCLUDES} 
		${GAMED3XP_GAMESYS_INCLUDES} 
		${GAMED3XP_MENUS_INCLUDES}
		${GAMED3XP_PHYSICS_INCLUDES}
		${GAMED3XP_SCRIPT_INCLUDES})
		
	list(APPEND RBDOOM3_SOURCES
		${GAMED3XP_SOURCES}
		${GAMED3XP_AI_SOURCES}
		${GAMED3XP_ANIM_SOURCES}
		${GAMED3XP_GAMESYS_SOURCES}
		${GAMED3XP_MENUS_SOURCES}
		${GAMED3XP_PHYSICS_SOURCES}
		${GAMED3XP_SCRIPT_SOURCES})
else()
	add_definitions(-D__DOOM_DLL__)
endif()

# KORTEMIK: fix these to contain only /
include_directories(
			.
			idlib
			libs/glew/include
			libs/zlib)

if(IDTOOLS)
  add_definitions(-DUSE_IDTOOLS)
 
	list(APPEND RBDOOM3_INCLUDES
	  ${IDTOOLS_INCLUDES}
	)
		
	list(APPEND RBDOOM3_SOURCES
	  ${IDTOOLS_SOURCES} 
	)
endif()
			
if(MSVC)

	# if(MSVC_VERSION EQUAL 1700)
		# set(DirectX_LIBRARIES
			# dinput8
			# dsound
			# dxguid
			# #dxerr
			# Xinput
			# xaudio2.lib
			# )
		
		# if(NOT OPENAL)
			# list(APPEND DirectX_LIBRARIES xaudio2.lib)
		# endif()
		
	# else()
		find_package(DirectX REQUIRED)
		include_directories(${DirectX_INCLUDE_DIR})
	#endif()
	
	if(OPENAL)
		add_definitions(-DUSE_OPENAL)
	
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/include)
		
		if(CMAKE_CL_64)
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win64)
		else()
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win32)
		endif()
		
		list(APPEND RBDOOM3_INCLUDES ${OPENAL_INCLUDES})
		list(APPEND RBDOOM3_SOURCES
			${OPENAL_SOURCES}
			${DOOMCLASSIC_OPENAL_SOURCES})
		
		set(OpenAL_LIBRARIES
			OpenAL32)
			
		if(CMAKE_CL_64)
			install(FILES libs/openal-soft/lib/win64/OpenAL64.dll DESTINATION .)
		else()
			install(FILES libs/openal-soft/lib/win32/OpenAL32.dll DESTINATION .)
			install(FILES libs/openal-soft/lib/win32/OpenAL32.pdb DESTINATION .)
		endif()
	else()
		list(APPEND RBDOOM3_INCLUDES ${XAUDIO2_INCLUDES})
		list(APPEND RBDOOM3_SOURCES
			${XAUDIO2_SOURCES}
			${DOOMCLASSIC_XAUDIO2_SOURCES})
	endif()
	
	if(FFMPEG)
		add_definitions(-DUSE_FFMPEG)
	
		if(CMAKE_CL_64)
			include_directories(libs/ffmpeg-win64/include)
			include_directories(libs/ffmpeg-win64/include/libswscale)
			include_directories(libs/ffmpeg-win64/include/libavformat)
			include_directories(libs/ffmpeg-win64/include/libavdevice)
			include_directories(libs/ffmpeg-win64/include/libavcodec)
			
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg-win64/lib)
		else()
			include_directories(libs/ffmpeg-win32/include)
			include_directories(libs/ffmpeg-win32/include/libswscale)
			include_directories(libs/ffmpeg-win32/include/libavformat)
			include_directories(libs/ffmpeg-win32/include/libavdevice)
			include_directories(libs/ffmpeg-win32/include/libavcodec)
			
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg-win32/lib)
		endif()
		
		set(FFmpeg_LIBRARIES
				avcodec
				avformat
				avutil
				swscale)
		
	endif()
	
	if(USE_MFC_TOOLS)
		
		include_directories(libs/atlmfc/include)
		if(CMAKE_CL_64)
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/atlmfc/lib/amd64)
		else()
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/atlmfc/lib)
		endif()
		
		add_definitions(-DUSE_MFC_TOOLS)
		
		list(APPEND RBDOOM3_SOURCES
			${TOOLS_INCLUDES}
			${TOOLS_COMAFX_INCLUDES} ${TOOLS_COMAFX_SOURCES}
			${TOOLS_COMMON_INCLUDES} ${TOOLS_COMMON_SOURCES}
			${TOOLS_COMMON_PROPTREE_INCLUDES} ${TOOLS_COMMON_PROPTREE_SOURCES}
			
			${EDITOR_AF_INCLUDES} ${EDITOR_AF_SOURCES}
			${EDITOR_DEBUGGER_INCLUDES} ${EDITOR_DEBUGGER_SOURCES}
			${EDITOR_DECL_INCLUDES} ${EDITOR_DECL_SOURCES}
			${EDITOR_GUIED_INCLUDES} ${EDITOR_GUIED_SOURCES}
			${EDITOR_MATERIALEDITOR_INCLUDES} ${EDITOR_MATERIALEDITOR_SOURCES}
			${EDITOR_PARTICLE_INCLUDES} ${EDITOR_PARTICLE_SOURCES}
			${EDITOR_PDA_INCLUDES} ${EDITOR_PDA_SOURCES}
			${EDITOR_RADIANT_INCLUDES} ${EDITOR_RADIANT_SOURCES}
			${EDITOR_SCRIPT_INCLUDES} ${EDITOR_SCRIPT_SOURCES}
			${EDITOR_SOUND_INCLUDES} ${EDITOR_SOUND_SOURCES})
	endif()
	
	list(APPEND RBDOOM3_INCLUDES
		${SYS_INCLUDES} 
		${WIN32_INCLUDES})
		
	list(APPEND RBDOOM3_SOURCES
		${SYS_SOURCES}
		${WIN32_SOURCES})

	list(REMOVE_DUPLICATES RBDOOM3_SOURCES)
	
	list(APPEND RBDOOM3_SOURCES ${WIN32_RESOURCES})
	
	add_executable(RBDoom3BFG WIN32 ${RBDOOM3_INCLUDES} ${RBDOOM3_SOURCES})
		
	target_link_libraries(RBDoom3BFG
	  idlib
	  ${JPEG_LIBRARY}
	  ${PNG_LIBRARY}
	  ${ZLIB_LIBRARY}
	  ${MINIZIP_LIBRARY}
	  ${TIMIDITY_LIBRARY}
	  ${IRRXML_LIBRARY}
	  ${GLEW_LIBRARY}
	  ${FREETYPE_LIBRARY}
	  ${OGGVORBIS_LIBRARY}
	  ${DirectX_LIBRARIES}
	  #libcurl 
	  #openal32 
	  opengl32
	  glu32
	  dbghelp
	  #eaxguid
	  iphlpapi
	  winmm
	  wsock32.lib
	  ${OpenAL_LIBRARIES}
	  ${FFmpeg_LIBRARIES}
	  )
		
	#CMAKE_BINARY_DIR
	if(CMAKE_CL_64)
		install(TARGETS RBDoom3BFG
			RUNTIME DESTINATION .)
	else()
		install(TARGETS RBDoom3BFG
			RUNTIME DESTINATION .)
	endif()
	
else()

	list(APPEND RBDOOM3_SOURCES
		${SYS_INCLUDES} ${SYS_SOURCES})
	
	find_package(OpenGL REQUIRED)
	include_directories(${OPENGL_INCLUDE_DIRS})

	if(UNIX)
		if(FFMPEG)
			find_package(FFMPEG REQUIRED)
			add_definitions(-DUSE_FFMPEG)
			
			include_directories(${FFMPEG_INCLUDE_DIR})
			link_directories(${FFMPEG_LIBRARIES_DIRS})
		endif()

		if(SDL2)
			find_package(SDL2 REQUIRED)
			include_directories(${SDL2_INCLUDE_DIR})
			set(SDLx_LIBRARY ${SDL2_LIBRARY})
		else()
			find_package(SDL REQUIRED)
			include_directories(${SDL_INCLUDE_DIR})
			set(SDLx_LIBRARY ${SDL_LIBRARY})
		endif()

		if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
			list(REMOVE_ITEM POSIX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_linux.cpp)
		else()
			list(REMOVE_ITEM POSIX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_osx.cpp)
		endif()

		list(APPEND RBDOOM3_SOURCES
			${POSIX_INCLUDES} ${POSIX_SOURCES}
			${SDL_INCLUDES} ${SDL_SOURCES})
			
		if(OPENAL)
			find_package(OpenAL REQUIRED)
			add_definitions(-DUSE_OPENAL)

			#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/include)
			
			#if(CMAKE_CL_64)
			#	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win64)
			#else()
			#	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win32)
			#endif()
			
			list(APPEND RBDOOM3_INCLUDES ${OPENAL_INCLUDES})
			list(APPEND RBDOOM3_SOURCES
				${OPENAL_SOURCES}
				${DOOMCLASSIC_OPENAL_SOURCES})
		else()
			list(APPEND RBDOOM3_INCLUDES ${STUBAUDIO_INCLUDES})
			list(APPEND RBDOOM3_SOURCES
				${STUBAUDIO_SOURCES}
				${DOOMCLASSIC_STUBAUDIO_SOURCES})
		endif()
			
	endif()

	list(REMOVE_DUPLICATES RBDOOM3_SOURCES)
	
	GET_DIRECTORY_PROPERTY(_directory_flags DEFINITIONS)
	LIST(APPEND _compiler_FLAGS ${_directory_flags})
	SEPARATE_ARGUMENTS(_compiler_FLAGS)

	add_executable(RBDoom3BFG WIN32 ${RBDOOM3_SOURCES})

	if(NOT WIN32)
		if(NOT "${CMAKE_SYSTEM}" MATCHES "Darwin")
			set(RT_LIBRARY rt)
		endif()

		if(NOT "${CMAKE_SYSTEM}" MATCHES "FreeBSD")
			set(DL_LIBRARY dl)
		endif()

		target_link_libraries(RBDoom3BFG
		  idlib
		  ${JPEG_LIBRARY}
		  ${PNG_LIBRARY}
		  ${ZLIB_LIBRARY}
		  ${MINIZIP_LIBRARY}
		  ${TIMIDITY_LIBRARY}
		  ${IRRXML_LIBRARY}
		  ${GLEW_LIBRARY}
		  ${FREETYPE_LIBRARY}
		  ${OGGVORBIS_LIBRARY}
		  ${OPENGL_LIBRARIES}
		  pthread
		  ${DL_LIBRARY}
		  ${RT_LIBRARY}
		  ${SDLx_LIBRARY}
		  ${OPENAL_LIBRARY}
		  ${FFMPEG_LIBRARIES}
		  )
	endif()
endif()
