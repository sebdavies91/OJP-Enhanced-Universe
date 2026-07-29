cmake_minimum_required(VERSION 3.10)

# For checks in subdirectories
set(InSDK TRUE)

# Project & Versioning
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ProjectName "OBP" CACHE STRING "Project Name")
project(${ProjectName})

# Languages
enable_language(C)

if(CMAKE_CONFIGURATION_TYPES)
	set(MultiConfigGenerator TRUE)
endif()

# Architecture
if(WIN32)
    set(ARCH_X86 ON)
    set(GlobalDefines ${GlobalDefines} "WIN32")
    if(CMAKE_SIZEOF_VOID_P MATCHES "8")
        set(GlobalDefines ${GlobalDefines} "ARCH_X86_64" "WIN64")
        set(Architecture "x64")
        set(ARCH_X86_64 ON)
    else()
        set(GlobalDefines ${GlobalDefines} "ARCH_X86")
        set(Architecture "x86")
        set(ARCH_X86_32 ON)
    endif()
elseif(APPLE)
    set(GlobalDefines ${GlobalDefines} "MACOS_X")

    if(CMAKE_OSX_ARCHITECTURES)
        if(CMAKE_OSX_ARCHITECTURES MATCHES "^x86_64" OR CMAKE_OSX_ARCHITECTURES MATCHES "^arm64")
            if( NOT CMAKE_OSX_ARCHITECTURES MATCHES CMAKE_SYSTEM_PROCESSOR )
                message(STATUS "Forcing architecture to ${CMAKE_OSX_ARCHITECTURES} on ${CMAKE_SYSTEM_PROCESSOR}")
            endif()
            set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_OSX_ARCHITECTURES}" CACHE STRING "")
        else()
            message(FATAL_ERROR "Unsupported macOS architecture")
        endif()
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
        set(GlobalDefines ${GlobalDefines} "ARCH_ARM64")
        set(Architecture "arm64")
        set(ARCH_ARM64 ON)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
        set(GlobalDefines ${GlobalDefines} "ARCH_X86")
        set(Architecture "i386")
        set(ARCH_X86 ON)
        set(ARCH_X86_32 ON)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86.64$")
        set(GlobalDefines ${GlobalDefines} "ARCH_X86_64")
        set(Architecture "x86_64")
        set(ARCH_X86 ON)
        set(ARCH_X86_64 ON)
    endif()
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
        set(GlobalDefines ${GlobalDefines} "ARCH_ARM64")
        set(Architecture "arm64")
        set(ARCH_ARM64 ON)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        set(GlobalDefines ${GlobalDefines} "ARCH_ARM32")
        set(Architecture "arm")
        set(ARCH_ARM32 ON)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
        set(GlobalDefines ${GlobalDefines} "ARCH_X86")
        set(Architecture "i386")
        set(ARCH_X86 ON)
        set(ARCH_X86_32 ON)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86.64|amd64)$")
        set(GlobalDefines ${GlobalDefines} "ARCH_X86_64")
        set(Architecture "x86_64")
        set(ARCH_X86 ON)
        set(ARCH_X86_64 ON)
    else()
        set(Architecture "${CMAKE_SYSTEM_PROCESSOR}")
        set(ARCH_UNKNOWN ON)
    endif()
endif()

message(STATUS "Architecture is ${Architecture}")
if(ARCH_UNKNOWN)
    message(WARNING "Building for unsupported Architecture ${Architecture}.")
endif()

# Binary names
set(SDKGAME "jampgame${Architecture}")
set(SDKCGAME "cgame${Architecture}")
set(SDKUI "ui${Architecture}")

# Directories
set(SRCDir "${CMAKE_CURRENT_SOURCE_DIR}") #should be /source on the repo

if(MultiConfigGenerator)
	# Visual Studio, Xcode etc. support selecting build type in the IDE and automatically append
	# the configuration type to the path
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
else()
	if(NOT CMAKE_BUILD_TYPE)
		message(STATUS "No build type selected, default to Release")
		set(CMAKE_BUILD_TYPE "Release")
	endif()

	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/${CMAKE_BUILD_TYPE})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/${CMAKE_BUILD_TYPE})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/${CMAKE_BUILD_TYPE})
endif()

set(GlobalDefines ${GlobalDefines} "_JK2")

if(APPLE)
set(GlobalDefines ${GlobalDefines} "MAC_PORT")
endif()

set(DebugDefines ${DebugDefines} "DEBUG")
set(ReleaseDefines "FINAL_BUILD" "NDEBUG")

if(MSVC)
    # Warnings
    # 4018 = 'expression' : signed/unsigned mismatch
    # 4091 = 'typedef ': ignored on left of 'enum-name' when no variable is declared
    # 4244 = 'conversion' : conversion from 'type1' to 'type2', possible loss of data
    # 4996 = 'function' : was declared deprecated
    # 4267 = 'var' : conversion from 'size_t' to 'type', possible loss of data
    # 4305 = 'identifier' : truncation from 'type1' to 'type2'
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4018 /wd4091 /wd4244 /wd4996 /wd4267 /wd4305")

    # Multithreaded
    set(CMAKE_C_FLAGS_RELEASE "/MT")

    # Platform
    set(GlobalDefines ${GlobalDefines} "UNICODE" "_UNICODE")

    # Optimization
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP") # multiprocessor compilation
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /Oi /Ot /Ob2") # maximize speed

    if(ARCH_X86_32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
    endif()

    # Set the same options for the build including symbols
    set(CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELEASE})
elseif ((${CMAKE_C_COMPILER_ID} STREQUAL GNU) OR (${CMAKE_C_COMPILER_ID} MATCHES Clang|AppleClang))
    # Additional flags for debug configuration
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb3")

    # we need sse optimizations in debug configuration, too because of the inline assembler which is sse code
    if(ARCH_X86)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
    endif()

    # Optimization
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

    # General
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

    # Warnings
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unknown-pragmas")

    if(${CMAKE_C_COMPILER_ID} STREQUAL GNU)
        if(ARCH_X86)
            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -mfpmath=sse")
        endif()

        # Link only libs actually needed
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wl,--as-needed")
    endif()
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

add_subdirectory(cgame)
add_subdirectory(game)
add_subdirectory(ui)