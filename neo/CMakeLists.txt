cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED APP_NAME)
	# RB: This sets the name of the .exe and is the only line needed for a different mod executable.
	# If you want to make a different game then you also need to change neo/framework/Licensee.h
    set(APP_NAME RBDoom3BFG)
endif()
project(${APP_NAME})

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.6")
	# set MSVC startup project
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${APP_NAME})
endif()
	
option(STANDALONE
		"Skip DOOM-3-BFG base/ folder and use content/ instead" OFF)

option(DOOM_CLASSIC
		"Build with Doom 1 & 2" ON)

option(FORCE_COLOR_OUTPUT
		"Always produce ANSI-colored output (GNU/Clang only)." OFF)

option(COMPILE_COMMANDS
		"Generate compile_commands.json" OFF)

option(USE_PRECOMPILED_HEADERS
		"Use precompiled headers during build" ON)
		
option(OPENAL
		"Use OpenAL soft instead of XAudio2" OFF)

option(FFMPEG
		"Use FMPEG to render Bink videos" OFF)

option(BINKDEC
		"Use included libbinkdec to render Bink videos" ON)

option(USE_MoltenVK
		"Link directly to MoltenVK library instead of Vulkan loader on macOS" OFF)

option(ONATIVE
		"Optimize for the host CPU" OFF)
		
option(WINDOWS10
		"Build for Windows 10+" ON)
		
option(USE_SYSTEM_ZLIB
         "Use the system zlib instead of the bundled one" OFF)

option(USE_SYSTEM_RAPIDJSON
         "Use the system rapidjson instead of the bundled one" OFF)

option(USE_DX12
		"Use DirectX 12" ON)

option(USE_VULKAN
		"Use Vulkan" ON)

option(USE_VMA
		"Use AMD's Vulkan Memory Allocator instead of the NVRHI builtin one" ON)

option(OPTICK
		"Enable profiling with Optick" OFF)

option(RETAIL
		"Strip certain developer features and cheats from shipping builds" OFF)

option(FLATPAK
		"Strip certain developer features and cheats from flatpak builds" OFF)
		
option(REPRODUCIBLE_BUILD
		"Replace __DATE__ and __TIME__ by hardcoded values for reproducible builds" OFF)

#set(NVRHI_INSTALL OFF)

set(CPU_TYPE "" CACHE STRING "When set, passes this string as CPU-ID which will be embedded into the binary.")

# SRS - Turn on compiler optimizations for x86 and also when cross-compiling from Apple arm64 to x86_64
if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)" OR CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
	set(CPU_OPTIMIZATION "-mmmx -msse -msse2" CACHE STRING "Which CPU-specific optimizations should be used besides the compiler's default?")
endif()

# SRS - Turn on MMX/SSE intrinsics for x86 but not when cross-compiling from Apple x86_64 to arm64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)|(e2k)|(E2K)" AND NOT CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
	option(USE_INTRINSICS_SSE "Compile using x86 MMX/SSE intrinsics (e.g SSE SIMD instructions)" ON)
else()
	option(USE_INTRINSICS_SSE "Compile using x86 MMX/SSE intrinsics (e.g SSE SIMD instructions)" OFF)
endif()

# SRS - Turn on NEON intrinsics for arm but not when cross-compiling from Apple arm64 to x86_64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(arm64)|(ARM64)|(aarch64)|(AArch64)" AND NOT CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
	option(USE_INTRINSICS_NEON "Compile using arm NEON intrinsics (e.g NEON SIMD instructions)" ON)
else()
	option(USE_INTRINSICS_NEON "Compile using arm NEON intrinsics (e.g NEON SIMD instructions)" OFF)
endif()

if(FFMPEG AND BINKDEC)
	message(FATAL_ERROR "Only one of FFMPEG and BINKDEC (or neither) can be enabled at a time")
endif()

if(UNIX)
	set(OPENAL ON)
	set(USE_DX12 OFF)
	set(USE_VULKAN ON)
	
    # SRS - Disable precompiled headers when cross-compiling on OSX, e.g. when creating universal binaries
    #     - Also disable precompiled headers when using the Ninja Multi-Config generator on linux or macOS
    if((CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64" AND CMAKE_OSX_ARCHITECTURES MATCHES "arm64" ) OR
       (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64"  AND CMAKE_OSX_ARCHITECTURES MATCHES "x86_64") OR
       (CMAKE_GENERATOR MATCHES "Ninja Multi-Config"))
        set(USE_PRECOMPILED_HEADERS OFF)
    endif()
    
	# SRS - For single-config dev builds, define NO_MULTI_CONFIG to search for basepath up 1 dir level vs 2
    if((CMAKE_GENERATOR MATCHES "Makefiles") OR
       (CMAKE_GENERATOR MATCHES "Ninja" AND NOT CMAKE_GENERATOR MATCHES "Multi-Config"))
		add_definitions(-DNO_MULTI_CONFIG)
    endif()
endif()

if(COMPILE_COMMANDS)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# Enforced colored output
if (CMAKE_COMPILER_IS_GNUCC)
	add_compile_options(-fdiagnostics-color=always)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
	add_compile_options(-fcolor-diagnostics)
endif ()

if(NOT MSVC)
	message(STATUS CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE})
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")

	add_compile_options(-pipe)
	#add_compile_options(-Wall)
	add_compile_options(-Werror=format-security)
	add_compile_options(-Werror=format)
	add_compile_options(-Wno-format-zero-length)
	add_compile_options(-Wno-nonnull)

	# Compiler check (needs -std=c++17 flag)
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
	if(COMPILER_SUPPORTS_CXX17)
		set(CMAKE_CXX_STANDARD 17)
		set(CMAKE_CXX_STANDARD_REQUIRED ON)
		set(CMAKE_CXX_EXTENSIONS OFF)
	else()
		message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
	endif()

	if(CPU_TYPE)
		add_definitions(-DCPUSTRING="${CPU_TYPE}")
	endif()
	if (CPU_OPTIMIZATION)
		SEPARATE_ARGUMENTS(CPU_OPTIMIZATION)
		add_compile_options(${CPU_OPTIMIZATION})
	endif()
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
	
	add_compile_options(-Wno-pragmas -Wno-unused-variable -Wno-switch -Wno-unused-value -Winvalid-pch -Wno-multichar)
	if(CMAKE_C_COMPILER_ID MATCHES "Clang")
		# add clang-specific settings for warnings (the second one make sure clang doesn't complain
		# about unknown -W flags, like -Wno-unused-but-set-variable)
		# SRS - Add -Wno-expansion-to-defined, Wno-nullability-completeness and -Wno-shorten-64-to-32 to list of warning settings
		add_compile_options(-Wno-local-type-template-args -Wno-unknown-warning-option -Wno-inline-new-delete -Wno-switch-enum -Wno-expansion-to-defined -Wno-nullability-completeness -Wno-shorten-64-to-32)
	endif()
	
	if(NOT CMAKE_CROSSCOMPILING AND ONATIVE)
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "((powerpc|ppc)64le)|(mips64)")
			add_compile_options(-mcpu=native)
		else()
			add_compile_options(-march=native)
		endif()
	endif()

	add_compile_options(-fno-strict-aliasing)

	# SRS - Make sure OSX can find system headers and add support for minimum OSX runtime version
    if(APPLE)
		# SRS - CMAKE_OSX_SYSROOT and CMAKE_OSX_DEPLOYMENT_TARGET are now defined by macOS build scripts prior to execution of CMake's project() statement.
		#       This approach supports CMake 4.0+ and earlier versions. Note -isysroot and -mmacosx-version-min still required for PCH command line builds.
		if(USE_PRECOMPILED_HEADERS AND NOT CMAKE_GENERATOR MATCHES "Xcode")
			if(CMAKE_OSX_SYSROOT)
				add_compile_options(-isysroot ${CMAKE_OSX_SYSROOT})
			endif()
			if(CMAKE_OSX_DEPLOYMENT_TARGET)
				add_compile_options(-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
			endif()
		endif()
		# SRS - Also add -fasm-blocks otherwise Xcode complains and -Qunused-arguments to silence MMX/SSE unused arg warnings when compiling for Apple arm64
		add_compile_options(-fasm-blocks -Qunused-arguments)
	endif()

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -D_DEBUG -O0")
    #set(CMAKE_C_FLAGS_DEBUGALL "${CMAKE_C_FLAGS_DEBUGALL} -g -ggdb -D_DEBUG")
    #set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE} -g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffp-contract=off -fomit-frame-pointer -fno-math-errno -fno-trapping-math")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g -O3 -ffp-contract=off -fomit-frame-pointer -fno-math-errno -fno-trapping-math")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -ffp-contract=off -fomit-frame-pointer -fno-math-errno -fno-trapping-math")
	
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
	
	
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP /MDd")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy /MD")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi /MDd")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy /MD")
		
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP /MDd")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy /MD")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /MDd")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy /MD")
		
	# RB: without /SAFESEH:NO we can't link against ffmpeg libs and VS2013 or we have to build our own
	# libs for newer compilers
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SAFESEH:NO")

	add_definitions(-DWIN32 
					-DNOMINMAX
					-D_WINDOWS
					-D_CRT_SECURE_NO_DEPRECATE
					-D_CRT_NONSTDC_NO_DEPRECATE
					-D_CRT_SECURE_NO_WARNINGS
					-D_MBCS
					-DUSE_EXCEPTIONS)

	if( WINDOWS10 ) # Windows RT
		add_definitions(-DUSE_WINRT)
	endif()

	if(NOT CMAKE_CL_64)
		add_definitions(-D_USE_32BIT_TIME_T)
	endif()

endif()

if (USE_INTRINSICS_SSE)
	add_definitions(-DUSE_INTRINSICS_SSE)
endif()

if (USE_INTRINSICS_NEON)
	add_definitions(-DUSE_INTRINSICS_NEON)
	add_definitions(-DSTBI_NEON)
endif()

if(STANDALONE)
	add_definitions(-DSTANDALONE)

	set(DOOM_CLASSIC OFF)
endif()

if(RETAIL)
	add_definitions(-DID_RETAIL)
endif()

if(REPRODUCIBLE_BUILD OR FLATPAK)
	# don't use __DATE__ and __TIME__ macros so builds are reproducible
	add_definitions(-DID_REPRODUCIBLE_BUILD)
endif()

# SRS - on Apple set find_package() to prefer dylibs over macOS frameworks and xcframeworks
#     - required for cmake >= 3.29 and MoltenVK, also prefers openal-soft over Apple OpenAL
if(APPLE)
	set(CMAKE_FIND_FRAMEWORK LAST)
endif()

if(USE_SYSTEM_ZLIB)
	find_package(ZLIB)
endif()

if(ZLIB_FOUND)
	include_directories(${ZLIB_INCLUDE_DIRS})
	set(ZLIB_LIBRARY ${ZLIB_LIBRARIES})
else()
	include_directories("libs/zlib")
	set(ZLIB_LIBRARY "" )
endif()

#SRS - move rapidjson before vulkan and other includes to prioritize bundled iterator fixes
if(USE_SYSTEM_RAPIDJSON)
	find_package(rapidjson)
endif()

if(RAPIDJSON_FOUND)
	include_directories("${RAPIDJSON_INCLUDE_DIRS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RAPIDJSON_CXX_FLAGS}")
else()
	include_directories("libs/rapidjson/include")
endif()

include_directories("libs/imgui")
include_directories("libs/mikktspace")

macro(SET_OPTION option value)
  set(${option} ${value} CACHE "" INTERNAL FORCE)
endmacro()

if(USE_VULKAN)
	# RB: moved this above the general Vulkan part so glslang does not include Vulkan SDK headers
	# which causes all kinds of weird segmentation faults because struct sizes don't match

	# Use FindVulkan module added with CMAKE 3.7
	if(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
		message( STATUS "Using module to find Vulkan and components" )
		list(APPEND Vulkan_COMPONENTS dxc)
	    if(APPLE AND USE_MoltenVK)
			list(APPEND Vulkan_COMPONENTS MoltenVK)
    	endif()
		find_package(Vulkan OPTIONAL_COMPONENTS ${Vulkan_COMPONENTS})
	endif()

	if(NOT Vulkan_FOUND)
		message(FATAL_ERROR "Could not find Vulkan library!")
		#find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		#if(Vulkan_LIBRARY)
		#	set(Vulkan_FOUND ON)
		#	message("Using bundled Vulkan library version")
		#endif()
	else()
		include_directories(${Vulkan_INCLUDE_DIRS})

		# SRS - ShaderMake uses DXC_CUSTOM_PATH as fallback if dxc for SPIRV not found by other means
		if(Vulkan_dxc_exe_FOUND AND NOT DXC_CUSTOM_PATH)
			get_filename_component(DXC_CUSTOM_PATH ${Vulkan_dxc_EXECUTABLE} DIRECTORY)
		endif()

        if(APPLE)
            # SRS - Enable Beta extensions for Vulkan portability subset features on macOS
            add_definitions(-DVK_ENABLE_BETA_EXTENSIONS)
            # SRS - Optionally link directly to MoltenVK headers/library for runtime config functions on OSX
            if(USE_MoltenVK)
            	if(Vulkan_MoltenVK_FOUND)
					add_definitions(-DUSE_MoltenVK)
                	include_directories(${Vulkan_MoltenVK_INCLUDE_DIR})
                	set(Vulkan_LIBRARY ${Vulkan_MoltenVK_LIBRARY} CACHE FILEPATH "Path to MoltenVK library." FORCE)
                else()
		            message(FATAL_ERROR "Must define VULKAN_SDK location if USE_MoltenVK option enabled!")
                endif()
			else()
				# If using Xcode and the Vulkan loader's environment variables are defined, make them available within Xcode scheme
				if(CMAKE_GENERATOR MATCHES "Xcode")
					if(DEFINED ENV{VK_ADD_LAYER_PATH})
						set(CMAKE_XCODE_SCHEME_ENVIRONMENT "${CMAKE_XCODE_SCHEME_ENVIRONMENT};VK_ADD_LAYER_PATH=$ENV{VK_ADD_LAYER_PATH}")
					endif()
					if(DEFINED ENV{VK_ICD_FILENAMES})
						set(CMAKE_XCODE_SCHEME_ENVIRONMENT "${CMAKE_XCODE_SCHEME_ENVIRONMENT};VK_ICD_FILENAMES=$ENV{VK_ICD_FILENAMES}")
					endif()
					if(DEFINED ENV{VK_DRIVER_FILES})
						set(CMAKE_XCODE_SCHEME_ENVIRONMENT "${CMAKE_XCODE_SCHEME_ENVIRONMENT};VK_DRIVER_FILES=$ENV{VK_DRIVER_FILES}")
					endif()
				endif()
            endif()
		endif()
		message(STATUS "Using Vulkan: " ${Vulkan_LIBRARY})
	endif()
	
	# Eric: For use with SDL2/Vulkan
	if(UNIX)
        # SRS - For UNIX-like target OSs use the generic SDL Vulkan platform
        add_definitions(-DVULKAN_USE_PLATFORM_SDL)
	elseif(WIN32)
		add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
	endif()

	# SRS - Add support for vma allocator vs. NVRHI builtin allocator
	if(USE_VMA)
		add_definitions(-DUSE_AMD_ALLOCATOR)
		include_directories("libs/vma/include")
		file(GLOB VMA_INCLUDES libs/vma/include/*.h)
		source_group("libs\\vma" FILES ${VMA_INCLUDES})
	endif()

endif()

set(NVRHI_WITH_DX11 "${USE_DX11}" CACHE BOOL "" FORCE)
set(NVRHI_WITH_DX12 "${USE_DX12}" CACHE BOOL "" FORCE)
set(NVRHI_WITH_VULKAN "${USE_VULKAN}" CACHE BOOL "" FORCE)

set(NVRHI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/nvrhi)
add_subdirectory(${NVRHI_DIR})
add_definitions(-DUSE_NVRHI)

set(SHADERMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/ShaderMake)
add_subdirectory(${SHADERMAKE_DIR})

if(USE_DX12)
	# SRS - ShaderMake now finds dxc for DXIL when required
	set(USE_DXIL_ON_DX12 TRUE CACHE BOOL "Use DXC to compile DXIL shaders on DX12 - otherwise FXC and DXBC")
endif()

include_directories(${NVRHI_DIR}/include)
include_directories(${SHADERMAKE_DIR}/include)

include_directories("libs/optick")

if(OPTICK)
	file(GLOB OPTICK_INCLUDES libs/optick/*.h)
	file(GLOB OPTICK_SOURCES libs/optick/*.cpp)

	source_group("libs\\optick" FILES ${OPTICK_INCLUDES})
	source_group("libs\\optick" FILES ${OPTICK_SOURCES})
endif()

add_subdirectory(idlib)

if(USE_INTRINSICS_SSE)
	add_subdirectory(libs/moc)
	set(MASKED_OCCLUSION_LIBRARY MaskedOcclusionCulling)
endif()

if(USE_INTRINSICS_SSE OR USE_INTRINSICS_NEON)
	add_subdirectory(libs/ispc_texcomp)
	set(ISPC_TEXCOMP_LIBRARY ispc_texcomp)

	#set(ISPC_TEXCOMP_LIBRARY Compressonator_MD)
	#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/compressonator/lib)
endif()

# RB: there haven't been requests for a separate map compiler executable by macOS users
# and it is ok to not support it until someone also supports TrenchBroomBFG on macOS
if(NOT APPLE AND NOT FLATPAK)
	add_subdirectory(tools/compilers)
endif()

file(GLOB NATVIS_SOURCES .natvis)

file(GLOB AAS_INCLUDES aas/*.h)
file(GLOB AAS_SOURCES aas/*.cpp)

file(GLOB CM_INCLUDES cm/*.h)
file(GLOB CM_SOURCES cm/*.cpp)

file(GLOB FRAMEWORK_INCLUDES framework/*.h)
file(GLOB FRAMEWORK_SOURCES framework/*.cpp)

file(GLOB FRAMEWORK_DECLS_INCLUDES framework/Decl*.h)
file(GLOB FRAMEWORK_DECLS_SOURCES framework/Decl*.cpp)

list(REMOVE_ITEM FRAMEWORK_INCLUDES ${FRAMEWORK_DECLS_INCLUDES})
list(REMOVE_ITEM FRAMEWORK_SOURCES ${FRAMEWORK_DECLS_SOURCES})

file(GLOB FRAMEWORK_IMGUI_INCLUDES imgui/*.h)
file(GLOB FRAMEWORK_IMGUI_SOURCES imgui/*.cpp)

file(GLOB RENDERER_INCLUDES renderer/*.h)
file(GLOB RENDERER_SOURCES renderer/*.cpp)
file(GLOB RENDERER_COLOR_INCLUDES renderer/Color/*.h)
file(GLOB RENDERER_COLOR_SOURCES renderer/Color/*.cpp)
file(GLOB RENDERER_DXT_INCLUDES renderer/DXT/*.h)
file(GLOB RENDERER_DXT_SOURCES renderer/DXT/*.cpp)


file(GLOB RENDERER_SMAA_INCLUDES renderer/SMAA/*.h)

file(GLOB RENDERER_NVRHI_INCLUDES renderer/NVRHI/*.h)
file(GLOB RENDERER_NVRHI_SOURCES renderer/NVRHI/*.cpp)
file(GLOB RENDERER_PASSES_INCLUDES renderer/Passes/*.h)
file(GLOB RENDERER_PASSES_SOURCES renderer/Passes/*.cpp)

file(GLOB IRRXML_INCLUDES libs/irrxml/src/*.h)
file(GLOB IRRXML_SOURCES libs/irrxml/src/*.cpp)

file(GLOB IMGUI_INCLUDES libs/imgui/*.h)
file(GLOB IMGUI_SOURCES libs/imgui/*.cpp)

file(GLOB STB_INCLUDES libs/stb/*.h)

file(GLOB MESA_INCLUDES libs/mesa/*.h)

file(GLOB TINYEXR_INCLUDES libs/tinyexr/*.h)

file(GLOB MIKKTSPACE_INCLUDES libs/mikktspace/*.h)
file(GLOB MIKKTSPACE_SOURCES libs/mikktspace/*.c)


if (NOT ZLIB_FOUND)
	file(GLOB ZLIB_INCLUDES libs/zlib/*.h)
	file(GLOB ZLIB_SOURCES libs/zlib/*.c)
else()
	set(ZLIB_INCLUDES "")
	set(ZLIB_SOURCES "")
endif()

if(BINKDEC)
	file(GLOB BINKDEC_INCLUDES libs/libbinkdec/include/*.h)
	file(GLOB BINKDEC_SOURCES libs/libbinkdec/src/*.c libs/libbinkdec/src/*.cpp)
	add_definitions(-DUSE_BINKDEC)
	include_directories("libs/libbinkdec/include")
endif()

file(GLOB MINIZIP_INCLUDES libs/zlib/minizip/*.h)
file(GLOB MINIZIP_SOURCES libs/zlib/minizip/*.c libs/zlib/minizip/*.cpp)

set(SOUND_INCLUDES
	sound/snd_local.h
	sound/sound.h
	sound/SoundVoice.h
	sound/WaveFile.h
	sound/CinematicAudio.h)
	
set(SOUND_SOURCES
	sound/snd_decoder.cpp
	sound/snd_emitter.cpp
	sound/snd_shader.cpp
	sound/snd_system.cpp
	sound/snd_world.cpp
	sound/SoundVoice.cpp
	sound/WaveFile.cpp)
	
set(XAUDIO2_INCLUDES
	sound/XAudio2/XA2_SoundHardware.h
	sound/XAudio2/XA2_SoundSample.h
	sound/XAudio2/XA2_SoundVoice.h
	sound/Xaudio2/XA2_CinematicAudio.h)
	
set(XAUDIO2_SOURCES
	sound/XAudio2/XA2_SoundHardware.cpp
	sound/XAudio2/XA2_SoundSample.cpp
	sound/XAudio2/XA2_SoundVoice.cpp
	sound/Xaudio2/XA2_CinematicAudio.cpp)
	
set(OPENAL_INCLUDES
	sound/OpenAL/AL_SoundHardware.h
	sound/OpenAL/AL_SoundSample.h
	sound/OpenAL/AL_SoundVoice.h
	sound/OpenAL/AL_CinematicAudio.h)
	
set(OPENAL_SOURCES
	sound/OpenAL/AL_SoundHardware.cpp
	sound/OpenAL/AL_SoundSample.cpp
	sound/OpenAL/AL_SoundVoice.cpp
	sound/OpenAL/AL_CinematicAudio.cpp)

set(STUBAUDIO_INCLUDES
	sound/stub/SoundStub.h)

set(STUBAUDIO_SOURCES
	sound/stub/SoundSample.cpp)
	
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

file(GLOB RAPIDJSON_INCLUDES libs/rapidjson/include/rapidjson/*.h)
	
file(GLOB SYS_INCLUDES sys/*.h)
file(GLOB SYS_SOURCES sys/*.cpp)

if(NOT USE_DX12)
	get_filename_component(devicemanager_dx12_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/DeviceManager_DX12.cpp ABSOLUTE)
	list(REMOVE_ITEM SYS_SOURCES "${devicemanager_dx12_cpp_full_path}")
endif()

if(NOT USE_VULKAN)
	get_filename_component(devicemanager_vk_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/DeviceManager_VK.cpp ABSOLUTE)
	list(REMOVE_ITEM SYS_SOURCES "${devicemanager_vk_cpp_full_path}")
endif()

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
	d3xp/Entity.h
	d3xp/EnvironmentProbe.h
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
	d3xp/Entity.cpp
	d3xp/EnvironmentProbe.cpp
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

file(GLOB COMPILER_INCLUDES tools/compilers/*.h)

file(GLOB COMPILER_AAS_INCLUDES tools/compilers/aas/*.h)
file(GLOB COMPILER_AAS_SOURCES tools/compilers/aas/*.cpp)

file(GLOB COMPILER_DMAP_INCLUDES tools/compilers/dmap/*.h)
file(GLOB COMPILER_DMAP_SOURCES tools/compilers/dmap/*.cpp)

set(TOOLS_INCLUDES tools/edit_public.h)

file(GLOB IMGUI_EDITOR_INCLUDES tools/imgui/*.h)
file(GLOB IMGUI_EDITOR_SOURCES tools/imgui/*.cpp)

file(GLOB IMGUI_EDITOR_LIGHT_INCLUDES tools/imgui/lighteditor/*.h)
file(GLOB IMGUI_EDITOR_LIGHT_SOURCES tools/imgui/lighteditor/*.cpp)

file(GLOB IMGUI_EDITOR_UTIL_INCLUDES tools/imgui/util/*.h)
file(GLOB IMGUI_EDITOR_UTIL_SOURCES tools/imgui/util/*.cpp)

file(GLOB DOOMCLASSIC_INCLUDES ../doomclassic/doom/*.h)
file(GLOB DOOMCLASSIC_SOURCES ../doomclassic/doom/*.cpp)

set(DOOMCLASSIC_STUBAUDIO_SOURCES ../doomclassic/doom/i_sound_stub.cpp)
set(DOOMCLASSIC_OPENAL_SOURCES ../doomclassic/doom/i_sound_openal.cpp)
set(DOOMCLASSIC_XAUDIO2_SOURCES ../doomclassic/doom/i_sound_win32.cpp)

list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/doom/i_sound_stub.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/doom/i_sound_openal.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/doom/i_sound_win32.cpp)

file(GLOB TIMIDITY_INCLUDES libs/timidity/*.h)
file(GLOB TIMIDITY_SOURCES libs/timidity/*.cpp)

file(GLOB_RECURSE WIN32_INCLUDES sys/win32/*.h)
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
	sys/win32/rc/res/doom.ico
	sys/win32/rc/doom.rc
	)

file(GLOB POSIX_INCLUDES sys/posix/*.h)
file(GLOB POSIX_SOURCES sys/posix/*.cpp)

file(GLOB COMMON_INCLUDES sys/common/*.h)
file(GLOB COMMON_SOURCES sys/common/*.cpp)

file(GLOB SDL_INCLUDES sys/sdl/*.h)
file(GLOB SDL_SOURCES sys/sdl/*.cpp)

source_group("engine\\aas" FILES ${AAS_INCLUDES})
source_group("engine\\aas" FILES ${AAS_SOURCES})

source_group("engine\\cm" FILES ${CM_INCLUDES})
source_group("engine\\cm" FILES ${CM_SOURCES})

source_group("engine\\framework" FILES ${FRAMEWORK_INCLUDES})
source_group("engine\\framework" FILES ${FRAMEWORK_SOURCES})

source_group("engine\\decls" FILES ${FRAMEWORK_DECLS_INCLUDES})
source_group("engine\\decls" FILES ${FRAMEWORK_DECLS_SOURCES})

source_group("engine\\imgui" FILES ${FRAMEWORK_IMGUI_INCLUDES})
source_group("engine\\imgui" FILES ${FRAMEWORK_IMGUI_SOURCES})

source_group("engine\\renderer" FILES ${RENDERER_INCLUDES})
source_group("engine\\renderer" FILES ${RENDERER_SOURCES})

source_group("engine\\renderer\\Color" FILES ${RENDERER_COLOR_INCLUDES})
source_group("engine\\renderer\\Color" FILES ${RENDERER_COLOR_SOURCES})

source_group("engine\\renderer\\DXT" FILES ${RENDERER_DXT_INCLUDES})
source_group("engine\\renderer\\DXT" FILES ${RENDERER_DXT_SOURCES})

source_group("engine\\renderer\\NVRHI" FILES ${RENDERER_NVRHI_INCLUDES})
source_group("engine\\renderer\\NVRHI" FILES ${RENDERER_NVRHI_SOURCES})

source_group("engine\\renderer\\Passes" FILES ${RENDERER_PASSES_INCLUDES})
source_group("engine\\renderer\\Passes" FILES ${RENDERER_PASSES_SOURCES})

source_group("engine\\renderer\\SMAA" FILES ${RENDERER_SMAA_INCLUDES})

source_group("engine\\sys\\win32" FILES ${WIN32_INCLUDES})
source_group("engine\\sys\\win32" FILES ${WIN32_SOURCES})
source_group("engine\\sys\\win32\\Resources" FILES ${WIN32_RESOURCES})

source_group("engine\\sys\\posix" FILES ${POSIX_INCLUDES})
source_group("engine\\sys\\posix" FILES ${POSIX_SOURCES})

source_group("engine\\sys\\common" FILES ${COMMON_INCLUDES})
source_group("engine\\sys\\common" FILES ${COMMON_SOURCES})

source_group("engine\\sys\\sdl" FILES ${SDL_INCLUDES})
source_group("engine\\sys\\sdl" FILES ${SDL_SOURCES})

source_group("engine\\sound" FILES ${SOUND_INCLUDES})
source_group("engine\\sound" FILES ${SOUND_SOURCES})

source_group("engine\\sound\\XAudio2" FILES ${XAUDIO2_INCLUDES})
source_group("engine\\sound\\XAudio2" FILES ${XAUDIO2_SOURCES})

source_group("engine\\sound\\OpenAL" FILES ${OPENAL_INCLUDES})
source_group("engine\\sound\\OpenAL" FILES ${OPENAL_SOURCES})

source_group("engine\\sound\\stub" FILES ${STUBAUDIO_INCLUDES})
source_group("engine\\sound\\stub" FILES ${STUBAUDIO_SOURCES})

source_group("engine\\ui" FILES ${UI_INCLUDES})
source_group("engine\\ui" FILES ${UI_SOURCES})

source_group("engine\\ui-flash" FILES ${SWF_INCLUDES})
source_group("engine\\ui-flash" FILES ${SWF_SOURCES})

source_group("engine\\sys" FILES ${SYS_INCLUDES})
source_group("engine\\sys" FILES ${SYS_SOURCES})

source_group("libs\\irrxml" FILES ${IRRXML_INCLUDES})
source_group("libs\\irrxml" FILES ${IRRXML_SOURCES})

source_group("libs\\imgui" FILES ${IMGUI_INCLUDES})
source_group("libs\\imgui" FILES ${IMGUI_SOURCES})

source_group("libs\\stb" FILES ${STB_INCLUDES})

source_group("libs\\mesa" FILES ${MESA_INCLUDES})

source_group("libs\\tinyexr" FILES ${TINYEXR_INCLUDES})

source_group("libs\\mikktspace" FILES ${MIKKTSPACE_INCLUDES})
source_group("libs\\mikktspace" FILES ${MIKKTSPACE_SOURCES})

source_group("libs\\libbinkdec" FILES ${BINKDEC_INCLUDES})
source_group("libs\\libbinkdec" FILES ${BINKDEC_SOURCES})

source_group("libs\\zlib" FILES ${ZLIB_INCLUDES})
source_group("libs\\zlib" FILES ${ZLIB_SOURCES})

source_group("libs\\zlib\\minizip" FILES ${MINIZIP_INCLUDES})
source_group("libs\\zlib\\minizip" FILES ${MINIZIP_SOURCES})

source_group("libs\\rapidjson" FILES ${RAPIDJSON_INCLUDES})

source_group("libs\\oggvorbis" FILES ${OGGVORBIS_INCLUDES})
source_group("libs\\oggvorbis" FILES ${OGGVORBIS_SOURCES})

source_group("libs\\timidity" FILES ${TIMIDITY_INCLUDES})
source_group("libs\\timidity" FILES ${TIMIDITY_SOURCES})

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

source_group("tools\\compilers" FILES ${COMPILER_INCLUDES})

source_group("tools\\compilers\\aasbuild" FILES ${COMPILER_AAS_INCLUDES})
source_group("tools\\compilers\\aasbuild" FILES ${COMPILER_AAS_SOURCES})

source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_INCLUDES})
source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_SOURCES})

source_group("tools" FILES ${TOOLS_INCLUDES})

source_group("tools\\imgui" FILES ${IMGUI_EDITOR_INCLUDES})
source_group("tools\\imgui" FILES ${IMGUI_EDITOR_SOURCES})

source_group("tools\\imgui\\lighteditor" FILES ${IMGUI_EDITOR_LIGHT_INCLUDES})
source_group("tools\\imgui\\lighteditor" FILES ${IMGUI_EDITOR_LIGHT_SOURCES})

source_group("tools\\imgui\\util" FILES ${IMGUI_EDITOR_UTIL_INCLUDES})
source_group("tools\\imgui\\util" FILES ${IMGUI_EDITOR_UTIL_SOURCES})

set(RBDOOM3_INCLUDES
			${AAS_INCLUDES}
			${CM_INCLUDES}
			${FRAMEWORK_INCLUDES}
			${FRAMEWORK_DECLS_INCLUDES}
			${RENDERER_INCLUDES}
			${RENDERER_COLOR_INCLUDES}
			${RENDERER_DXT_INCLUDES}
			${RENDERER_PASSES_INCLUDES}
			
			${RENDERER_SMAA_INCLUDES}
			${IRRXML_INCLUDES}
			${FRAMEWORK_IMGUI_INCLUDES}
			${IMGUI_INCLUDES}
			${STB_INCLUDES}
			${MESA_INCLUDES}
			${TINYEXR_INCLUDES}
			${MIKKTSPACE_INCLUDES}
			${ZLIB_INCLUDES}
			${MINIZIP_INCLUDES}
			${BINKDEC_INCLUDES}
			${RAPIDJSON_INCLUDES}
			${SOUND_INCLUDES}
			${OGGVORBIS_INCLUDES}
			${VMA_INCLUDES}
			${OPTICK_INCLUDES}
			${UI_INCLUDES}
			${SWF_INCLUDES}
			${COMMON_INCLUDES}
			
			${COMPILER_INCLUDES}
			${COMPILER_AAS_INCLUDES} ${COMPILER_AAS_SOURCES}
			${COMPILER_DMAP_INCLUDES} ${COMPILER_DMAP_SOURCES}
			
			${TOOLS_INCLUDES}
			${IMGUI_EDITOR_INCLUDES}
			${IMGUI_EDITOR_LIGHT_INCLUDES}
			${IMGUI_EDITOR_UTIL_INCLUDES}
			
			${GAMED3XP_INCLUDES}
			${GAMED3XP_AI_INCLUDES} 
			${GAMED3XP_ANIM_INCLUDES} 
			${GAMED3XP_GAMESYS_INCLUDES} 
			${GAMED3XP_MENUS_INCLUDES}
			${GAMED3XP_PHYSICS_INCLUDES}
			${GAMED3XP_SCRIPT_INCLUDES}
			)
			
set(RBDOOM3_SOURCES
			${NATVIS_SOURCES}
			${AAS_SOURCES}
			${CM_SOURCES}
			${FRAMEWORK_SOURCES}
			${FRAMEWORK_DECLS_SOURCES} 
			${RENDERER_SOURCES}
			${RENDERER_COLOR_SOURCES}
			${RENDERER_DXT_SOURCES}
			${RENDERER_PASSES_SOURCES}
			${IRRXML_SOURCES}
			${FRAMEWORK_IMGUI_SOURCES}
			${IMGUI_SOURCES}
			${MIKKTSPACE_SOURCES}
			${ZLIB_SOURCES}
			${MINIZIP_SOURCES}
			${BINKDEC_SOURCES}
			${SOUND_SOURCES}
			${OGGVORBIS_SOURCES}
			${OPTICK_SOURCES}
			${UI_SOURCES}
			${SWF_SOURCES}
			${COMMON_SOURCES}
			
			${COMPILER_AAS_SOURCES}
			${COMPILER_DMAP_SOURCES}
			
			${IMGUI_EDITOR_SOURCES}
			${IMGUI_EDITOR_LIGHT_SOURCES}
			${IMGUI_EDITOR_UTIL_SOURCES}
			
			${GAMED3XP_SOURCES}
			${GAMED3XP_AI_SOURCES}
			${GAMED3XP_ANIM_SOURCES}
			${GAMED3XP_GAMESYS_SOURCES}
			${GAMED3XP_MENUS_SOURCES}
			${GAMED3XP_PHYSICS_SOURCES}
			${GAMED3XP_SCRIPT_SOURCES}
			)

macro(AddSubdirectory dirName)
	file(GLOB DIR_INCLUDES ${dirName}/*.h)
	file(GLOB DIR_SOURCES ${dirName}/*.cpp)

	source_group(${dirName} FILES ${DIR_INCLUDES})
	source_group(${dirName} FILES ${DIR_SOURCES})

	list(APPEND RBDOOM3_INCLUDES ${DIR_INCLUDES})
	list(APPEND RBDOOM3_SOURCES ${DIR_SOURCES})
endmacro()

AddSubdirectory( tools/imgui/afeditor )

if(DOOM_CLASSIC)
	add_definitions(-DUSE_DOOMCLASSIC)

	list(APPEND RBDOOM3_INCLUDES ${DOOMCLASSIC_INCLUDES})
	list(APPEND RBDOOM3_INCLUDES ${TIMIDITY_INCLUDES})
	
	list(APPEND RBDOOM3_SOURCES ${DOOMCLASSIC_SOURCES})
	list(APPEND RBDOOM3_SOURCES ${TIMIDITY_SOURCES})
endif()

add_definitions(-D__DOOM__)
				

include_directories(
			.
			idlib
            )

if(MSVC)
	
	if(WINDOWS10)
		set(DirectX_LIBRARIES
			dinput8
			dsound
			dxguid
			#dxerr
			Xinput
			xaudio2.lib
			)
	else()
		find_package(DirectX REQUIRED)
		include_directories(${DirectX_INCLUDE_DIR})
	endif()
	
	if(USE_DX12)
		list(APPEND RBDOOM3_INCLUDES ${RENDERER_NVRHI_INCLUDES})
		list(APPEND RBDOOM3_SOURCES ${RENDERER_NVRHI_SOURCES})
	endif()
	
	if(OPENAL)
		add_definitions(-DUSE_OPENAL)
	
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/include)
		
		if(CMAKE_CL_64)
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win64)
		else()
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win32)
		endif()
		
		list(APPEND RBDOOM3_INCLUDES ${OPENAL_INCLUDES})
		list(APPEND RBDOOM3_SOURCES	${OPENAL_SOURCES})
		if(DOOM_CLASSIC)
			list(APPEND RBDOOM3_SOURCES	${DOOMCLASSIC_OPENAL_SOURCES})
		endif()		
		
		set(OpenAL_LIBRARIES
			OpenAL32)
			
		if(CMAKE_CL_64)
			install(FILES libs/openal-soft/lib/bin/win64/OpenAL32.dll DESTINATION .)
		else()
			install(FILES libs/openal-soft/lib/bin/win32/OpenAL32.dll DESTINATION .)
			install(FILES libs/openal-soft/lib/bin/win32/OpenAL32.pdb DESTINATION .)
		endif()
	else()
		list(APPEND RBDOOM3_INCLUDES ${XAUDIO2_INCLUDES})
		list(APPEND RBDOOM3_SOURCES	${XAUDIO2_SOURCES})

		if(DOOM_CLASSIC)
			list(APPEND RBDOOM3_SOURCES	${DOOMCLASSIC_XAUDIO2_SOURCES})
		endif()
	endif()
	
	if(FFMPEG)
		add_definitions(-DUSE_FFMPEG)
	
		if(CMAKE_CL_64)
			include_directories(libs/ffmpeg-win64/include)
			include_directories(libs/ffmpeg-win64/include/libswscale)
			include_directories(libs/ffmpeg-win64/include/libavformat)
			include_directories(libs/ffmpeg-win64/include/libavdevice)
			include_directories(libs/ffmpeg-win64/include/libavcodec)
			include_directories(libs/ffmpeg-win64/include/libswresample)
			
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg-win64/lib)
		else()
			include_directories(libs/ffmpeg-win32/include)
			include_directories(libs/ffmpeg-win32/include/libswscale)
			include_directories(libs/ffmpeg-win32/include/libavformat)
			include_directories(libs/ffmpeg-win32/include/libavdevice)
			include_directories(libs/ffmpeg-win32/include/libavcodec)
			include_directories(libs/ffmpeg-win32/include/libswresample)
			
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg-win32/lib)
		endif()
		
		set(FFmpeg_LIBRARIES
				avcodec
				avformat
				avutil
				swscale
				swresample)
		
	endif()
	
	list(APPEND RBDOOM3_INCLUDES
		${SYS_INCLUDES} 
		${WIN32_INCLUDES})
		
	list(APPEND RBDOOM3_SOURCES
		${SYS_SOURCES}
		${WIN32_SOURCES})

	list(REMOVE_DUPLICATES RBDOOM3_SOURCES)
	
	if(USE_PRECOMPILED_HEADERS) 
		set(RBDOOM3_PRECOMPILED_SOURCES ${RBDOOM3_SOURCES})
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${TIMIDITY_SOURCES} ${ZLIB_SOURCES} ${BINKDEC_SOURCES} ${IMGUI_SOURCES} ${MIKKTSPACE_SOURCES} ${OGGVORBIS_SOURCES} ${OPTICK_SOURCES})
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib/minizip/ioapi.c)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTDecoder.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder_SSE2.cpp)
	
		#foreach( src_file ${RBDOOM3_PRECOMPILED_SOURCES} )
		#	message(STATUS "-include precompiled.h for ${src_file}")
		#endforeach()
	
		set_source_files_properties(
			${RBDOOM3_PRECOMPILED_SOURCES}
			PROPERTIES
			COMPILE_FLAGS "/Yuprecompiled.h"
			OBJECT_DEPENDS "precompiled.pch"
		)
		
		set_source_files_properties(framework/precompiled.cpp
			PROPERTIES
			COMPILE_FLAGS "/Ycprecompiled.h"
			OBJECT_OUTPUTS "precompiled.pch"
		)
	endif()
	
	# SRS - disable certain MSVC warnings for select third-party source libraries, consider updating versions in the future?
	set_source_files_properties(
			${OGGVORBIS_SOURCES}
			PROPERTIES
			COMPILE_FLAGS "/wd4101 /wd4267"	# C4101: unreferenced local variable, C4267: type conversion with possible loss of data
			)
	
	list(APPEND RBDOOM3_SOURCES ${WIN32_RESOURCES})
	
	add_executable(${APP_NAME} WIN32 ${RBDOOM3_INCLUDES} ${RBDOOM3_SOURCES})
	add_dependencies(${APP_NAME} idlib)
    
    if(USE_DX12)
		add_dependencies(${APP_NAME} nvrhi_d3d12)
	endif()
	
	if(USE_VULKAN)
		add_dependencies(${APP_NAME} nvrhi_vk)
	endif()
	target_link_libraries(${APP_NAME}
		idlib
		${DirectX_LIBRARIES}
		dbghelp
		#eaxguid
		iphlpapi
		winmm
		wsock32
		Shlwapi
		${Vulkan_LIBRARIES}
		${OpenAL_LIBRARIES}
		${FFmpeg_LIBRARIES}
        ${ZLIB_LIBRARY}
		${CMAKE_DL_LIBS}
		)
		
	# set MSVC default debug directory on executable	
	if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.8")
		set_target_properties(${APP_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/..")
	endif()
	
	#CMAKE_BINARY_DIR
	if(CMAKE_CL_64)
		install(TARGETS ${APP_NAME}
			RUNTIME DESTINATION .)
	else()
		install(TARGETS ${APP_NAME}
			RUNTIME DESTINATION .)
	endif()	

else()

	list(APPEND RBDOOM3_SOURCES
		${SYS_INCLUDES} ${SYS_SOURCES})
	
	if(UNIX)
		if(FFMPEG)
			find_package(FFMPEG REQUIRED)
			add_definitions(-DUSE_FFMPEG)
			
			include_directories(${FFMPEG_INCLUDE_DIR})
		endif()

		find_package(SDL2 REQUIRED)
		include_directories(${SDL2_INCLUDE_DIRS})
		set(SDLx_LIBRARY ${SDL2_LIBRARIES})

        if(APPLE)
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

			# SRS - Added support for OpenAL Soft headers on OSX (vs default macOS SDK headers)
            if(APPLE AND NOT OPENAL_INCLUDE_DIR MATCHES "SDKs/MacOSX.*\.sdk")
				include_directories(${OPENAL_INCLUDE_DIR})
				add_definitions(-DUSE_OPENAL_SOFT_INCLUDES)
			endif()

			list(APPEND RBDOOM3_INCLUDES ${OPENAL_INCLUDES})
			list(APPEND RBDOOM3_SOURCES ${OPENAL_SOURCES})
				
			if(DOOM_CLASSIC)
				list(APPEND RBDOOM3_SOURCES	${DOOMCLASSIC_OPENAL_SOURCES})
			endif()
		else()
			list(APPEND RBDOOM3_INCLUDES ${STUBAUDIO_INCLUDES})
			list(APPEND RBDOOM3_SOURCES ${STUBAUDIO_SOURCES})
			
			if(DOOM_CLASSIC)
				list(APPEND RBDOOM3_SOURCES	${DOOMCLASSIC_STUBAUDIO_SOURCES})
			endif()
		endif()
		
		if(USE_VULKAN)
            list(APPEND RBDOOM3_INCLUDES ${RENDERER_NVRHI_INCLUDES})
            list(APPEND RBDOOM3_SOURCES ${RENDERER_NVRHI_SOURCES})

            #set(Vulkan_LIBRARIES ${Vulkan_LIBRARY})
        endif()
			
	endif()

	list(REMOVE_DUPLICATES RBDOOM3_SOURCES)
	
	if(USE_PRECOMPILED_HEADERS)
		# we need to recreate the precompiled header for RBDoom3BFG
		# (i.e. can't use the one created for idlib before)
		# because some definitions (e.g. -D__IDLIB__ -D__DOOM_DLL__) differ

		set(RBDOOM3_PRECOMPILED_SOURCES ${RBDOOM3_SOURCES})
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${TIMIDITY_SOURCES} ${ZLIB_SOURCES} ${BINKDEC_SOURCES} ${IMGUI_SOURCES} ${MIKKTSPACE_SOURCES} ${OGGVORBIS_SOURCES} ${OPTICK_SOURCES})
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib/minizip/ioapi.c)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTDecoder.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder.cpp)
		list(REMOVE_ITEM RBDOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder_SSE2.cpp)

		foreach( src_file ${RBDOOM3_PRECOMPILED_SOURCES} )
			#message(STATUS "-include precompiled.h for ${src_file}")
			set_source_files_properties(
				${src_file}
				PROPERTIES
				COMPILE_FLAGS "-include ${CMAKE_CURRENT_SOURCE_DIR}/idlib/precompiled.h"
				)
		endforeach()

		# precompiled magic for GCC/clang, adapted from https://gist.github.com/573926
		STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
		SET(_compiler_FLAGS "${${_flags_var_name}} -std=c++${CMAKE_CXX_STANDARD}")
		GET_DIRECTORY_PROPERTY(_directory_flags INCLUDE_DIRECTORIES)
		FOREACH(item ${_directory_flags})
			LIST(APPEND _compiler_FLAGS " -I${item}")
		ENDFOREACH(item)

		GET_DIRECTORY_PROPERTY(_directory_flags COMPILE_OPTIONS)
		LIST(APPEND _compiler_FLAGS ${_directory_flags})
		SEPARATE_ARGUMENTS(_compiler_FLAGS)

		GET_DIRECTORY_PROPERTY(_directory_flags COMPILE_DEFINITIONS)
		FOREACH(item ${_directory_flags})
			LIST(APPEND _compiler_FLAGS "-D${item}")
		ENDFOREACH(item)

		if(OPTICK)
			LIST(APPEND _compiler_FLAGS -DUSE_OPTICK=1)
		else()
			LIST(APPEND _compiler_FLAGS -DUSE_OPTICK=0)
		endif()

		add_custom_target(precomp_header_rbdoom3bfg ALL
				COMMAND ${CMAKE_CXX_COMPILER} ${_compiler_FLAGS} -x c++-header idlib/precompiled.h -o idlib/precompiled.h.gch
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				COMMENT "Creating idlib/precompiled.h.gch for ${APP_NAME}"
				)

		# make sure this is run after creating idlib
		add_dependencies(precomp_header_rbdoom3bfg idlib)
	endif()

	# SRS - disable certain gcc/clang warnings for select third-party source libraries, consider updating versions in the future?
	set_source_files_properties(
			${ZLIB_SOURCES}
			${MINIZIP_SOURCES}
			PROPERTIES
			COMPILE_FLAGS "-Wno-stringop-overread -Wno-deprecated-non-prototype"
			)
		
	# SRS - if using gcc compiler enable gnu extensions for ##__VA_ARGS__ support within optick profiler (i.e. __STRICT_ANSI__ not set)
	if(CMAKE_COMPILER_IS_GNUCC)
		set_source_files_properties(
				${OPTICK_SOURCES}
				PROPERTIES
				COMPILE_FLAGS "-std=gnu++${CMAKE_CXX_STANDARD}"
				)
	endif()
	
	if(MACOSX_BUNDLE)
		# define contents of macOS app bundle Resources folder
		set(MACOS_RESOURCES sys/posix/res/Doom3BFG.icns ../base)
		source_group("Resources" FILES ${MACOS_RESOURCES})
		set_source_files_properties(${MACOS_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

		# enable Xcode to display the icons file properly
		set_source_files_properties(sys/posix/res/Doom3BFG.icns PROPERTIES XCODE_EXPLICIT_FILE_TYPE .icns)

		# add macOS bundle properties to executable
		set(MACOSX_BUNDLE_BUNDLE_NAME RBDoom-3-BFG)
		set(MACOSX_BUNDLE_BUNDLE_VERSION 1.6.0)
		set(MACOSX_BUNDLE_SHORT_VERSION_STRING 1.6.0)
		set(MACOSX_BUNDLE_ICON_FILE Doom3BFG.icns)
		set(MACOSX_BUNDLE_INFO_STRING "")					# to be added by author if desired
		set(MACOSX_BUNDLE_COPYRIGHT "")						# to be added by author if desired

		add_executable(${APP_NAME} MACOSX_BUNDLE ${MACOS_RESOURCES} ${RBDOOM3_INCLUDES} ${RBDOOM3_SOURCES})
	else()
		add_executable(${APP_NAME} WIN32 ${RBDOOM3_INCLUDES} ${RBDOOM3_SOURCES})
	endif()

	if(USE_VULKAN)
        add_dependencies(${APP_NAME} nvrhi_vk)
    endif()

	if (USE_PRECOMPILED_HEADERS)
		# make sure precompiled header is created before executable is compiled
		add_dependencies(${APP_NAME} precomp_header_rbdoom3bfg)
		
		if(WIN32)
			set(remove_command "del")
		else()
			set(remove_command "rm")
		endif()

		# delete precompiled header file after executable is compiled: command line build case
		if(CMAKE_GENERATOR MATCHES "Makefiles" OR CMAKE_GENERATOR MATCHES "Ninja")
			add_custom_target(rm_precomp_header ALL
					COMMAND ${remove_command} "idlib/precompiled.h.gch"
					WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
					COMMENT "remove idlib/precompiled.h.gch"
					)
			add_dependencies(rm_precomp_header ${APP_NAME})

		# delete precompiled header file after executable is compiled: IDE build case (e.g. Xcode)
		else()
			add_custom_command(TARGET ${APP_NAME} POST_BUILD
					# SRS - added wildcards to remove tmp files from cmake ZERO_CHECK regeneration
					COMMAND ${remove_command} "idlib/precompiled.h*.gch*"
					WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
					COMMENT "remove idlib/precompiled.h.gch"
			)
		endif()
	endif()

	if(NOT WIN32)
        if(NOT APPLE)
			set(RT_LIBRARY rt)
		endif()

		target_link_libraries(${APP_NAME}
			idlib
			${Vulkan_LIBRARY}
			pthread
			${RT_LIBRARY}
			${SDLx_LIBRARY}
			${OPENAL_LIBRARY}
			${FFMPEG_LIBRARIES}
            ${ZLIB_LIBRARY}
	    	${CMAKE_DL_LIBS}
			)
	endif()
endif()

if(USE_DX12)
	target_compile_definitions(${APP_NAME} PUBLIC USE_DX12=1)
	target_link_libraries(${APP_NAME} nvrhi_d3d12)
endif()

if(USE_DXIL_ON_DX12)
	target_compile_definitions(${APP_NAME} PRIVATE USE_DXIL_ON_DX12=1)
endif()

if(USE_VULKAN)
	target_compile_definitions(${APP_NAME} PUBLIC USE_VK=1)
	target_link_libraries(${APP_NAME} nvrhi_vk)
endif()

if(OPTICK)
	target_compile_definitions(${APP_NAME} PUBLIC USE_OPTICK=1)
else()
	target_compile_definitions(${APP_NAME} PUBLIC USE_OPTICK=0)
endif()

# needs to come after nvrhi_d3d11 etc. for link order
target_link_libraries(${APP_NAME} nvrhi)
target_link_libraries(${APP_NAME} ShaderMakeBlob ${MASKED_OCCLUSION_LIBRARY} ${ISPC_TEXCOMP_LIBRARY})

add_subdirectory(shaders)
add_dependencies(Shaders ShaderMake)
add_dependencies(${APP_NAME} Shaders ${MASKED_OCCLUSION_LIBRARY} ${ISPC_TEXCOMP_LIBRARY})

