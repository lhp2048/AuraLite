
#ifndef MXUI_EXPORT_H_
#define MXUI_EXPORT_H_

#pragma once

// Static-library build is the default for Mx.Base / Mx.UILegacy / Mx.UI.
#if defined(MXUI_STATIC) || !defined(MXUI_DLL)
#  define MXUI_EXPORT
#elif defined(MXUI_IMPLEMENTATION)
#  define MXUI_EXPORT __declspec(dllexport)
#else
#  define MXUI_EXPORT __declspec(dllimport)
#endif

#endif  // MXUI_EXPORT_H_
