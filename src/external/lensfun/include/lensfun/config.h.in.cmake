#ifndef __CONFIG_H__
#define __CONFIG_H__

/* config file for Lensfun library
   generated by CMake
*/

#define CONF_PACKAGE "lensfun"
#define CONF_VERSION "@VERSION_MAJOR@.@VERSION_MINOR@.@VERSION_MICRO@.@VERSION_BUGFIX@"

#if defined(__APPLE__)
    #define PLATFORM_OSX
#elif defined(_WIN32)
    #define PLATFORM_WINDOWS
#elif defined(__unix__)
    #define PLATFORM_LINUX
#endif


#cmakedefine CMAKE_COMPILER_IS_GNUCC
#ifdef CMAKE_COMPILER_IS_GNUCC
#define CONF_COMPILER_GCC 1
#endif

#if defined(PLATFORM_LINUX)
  #define SYSTEM_DB_PATH "${CMAKE_INSTALL_FULL_DATAROOTDIR}/photoflow/lensfun"
  #define SYSTEM_DB_UPDATE_PATH "/${CMAKE_INSTALL_LOCALSTATEDIR}/lib/lensfun-updates"
#elif defined(PLATFORM_WINDOWS)
  #define SYSTEM_DB_PATH "${CMAKE_INSTALL_DATAROOTDIR}/lensfun"
  #define SYSTEM_DB_UPDATE_PATH "${CMAKE_INSTALL_DATAROOTDIR}/lensfun-updates"
#elif defined(PLATFORM_OSX)
  #define SYSTEM_DB_PATH "${CMAKE_INSTALL_FULL_DATAROOTDIR}/photoflow/lensfun"
  #define SYSTEM_DB_UPDATE_PATH "/${CMAKE_INSTALL_LOCALSTATEDIR}/lib/lensfun-updates"
#endif

#define DATABASE_SUBDIR "version_${LENSFUN_DB_VERSION}"

// add a macro to know we're compiling Lensfun, not a client library
#define CONF_LENSFUN_INTERNAL

#cmakedefine VECTORIZATION_SSE
#cmakedefine VECTORIZATION_SSE2

#cmakedefine HAVE_ENDIAN_H

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#define GLIB_VERSION_MIN_REQUIRED      (${LENSFUN_GLIB_REQUIREMENT_MACRO})

// to avoid usage of API that is not in GLIB_VERSION_MIN_REQUIRED version
// and make it easy to detect when to bump requirements:
#define GLIB_VERSION_MAX_ALLOWED      (GLIB_VERSION_MIN_REQUIRED)

#endif // __CONFIG_H__
