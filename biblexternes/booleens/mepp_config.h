
///////////////////////////////////////////////////////////////////////////
// Author: Martial TOLA
// Year: 2010
// CNRS-Lyon, LIRIS UMR 5205
///////////////////////////////////////////////////////////////////////////
#ifndef HEADER_MEPP_CONFIG
#define HEADER_MEPP_CONFIG

#if _WIN64 || __amd64__
#define PORTABLE_64_BIT
#define ARCHITECTURE QObject::tr("(64 bits)")
#ifdef _MSC_VER
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)
#endif
#else
#define PORTABLE_32_BIT
#define ARCHITECTURE QObject::tr("(32 bits)")
#endif

#define ORGANIZATION QObject::tr("LIRIS")
#define APPLICATION QObject::tr("MEPP")

#define MAINWINDOW_TITLE QObject::tr("MEPP : 3D MEsh Processing Platform")
#define BUILD_TYPE QObject::tr("${CMAKE_BUILD_TYPE} version")

#define EMPTY_MESH QObject::tr("empty mesh")
#define INTERNAL_MESH QObject::tr("internal mesh sample")

enum { Normal, Space, Time, Specific };
//enum { MaxManipulatedFrame = 19 };

#endif
