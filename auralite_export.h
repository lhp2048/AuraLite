
#ifndef AURALITE_EXPORT_H_
#define AURALITE_EXPORT_H_

#pragma once

// Static-library build is the default for AuraLite.Base / AuraLite.UI.
#if defined(AURALITE_STATIC) || !defined(AURALITE_DLL)
#  define AURALITE_EXPORT
#elif defined(AURALITE_IMPLEMENTATION)
#  define AURALITE_EXPORT __declspec(dllexport)
#else
#  define AURALITE_EXPORT __declspec(dllimport)
#endif

#endif  // AURALITE_EXPORT_H_
