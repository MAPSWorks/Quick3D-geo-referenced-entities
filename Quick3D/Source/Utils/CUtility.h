
#pragma once

// Qt
#include <QMap>
#include <QString>
#include <QImage>

// Application
#include "quick3d_global.h"

//-------------------------------------------------------------------------------------------------

#ifndef _MAKELONG

/* Red�finition des formats de donn�es */
typedef unsigned char uint8;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;

// Pour GCC sur processeur 64 bits, long = 64 bits ! utiliser int pour des entiers de 32 bits
#if defined(__GNUC__) && (__WORDSIZE == 64)
typedef unsigned int uint32;
typedef int  int32;
#else
typedef unsigned long uint32;
typedef long int32;
#endif

#define _MAKEWORD(a, b)      ((uint16)(((uint8)(((uint32)(a)) & 0xff)) | ((uint16)((uint8)(((uint32)(b)) & 0xff))) << 8))
#define _MAKELONG(a, b)      ((uint32)(((uint16)(((uint32)(a)) & 0xffff)) | ((uint32)((uint16)(((uint32)(b)) & 0xffff))) << 16))
#define _LOWORD(l)           ((uint16)(((uint32)(l)) & 0xffff))
#define _HIWORD(l)           ((uint16)((((uint32)(l)) >> 16) & 0xffff))
#define _LOBYTE(w)           ((uint8)(((uint32)(w)) & 0xff))
#define _HIBYTE(w)           ((uint8)((((uint32)(w)) >> 8) & 0xff))

#endif

//-------------------------------------------------------------------------------------------------

class QUICK3D_EXPORT CUtility
{
public:

    //! Format method for QString
    static QString FormatString(const char *szFormat, va_list &arg_ptr) ;
    static QString FormatString(const char *szFormat, ...) ;
    static void FormatPath(QString &path);

    //! Creates the path in \a sName if it does not exist
    static void ensurePathExists(const QString& sName);

    //! Returns \c true if the process named \a sName exists
    static bool findProcessByName(const QString& sName);

    //! Main thread sleeps for \a iMilliSeconds
    static void sleep(int iMilliSeconds);

    //! Main thread sleeps for \a iMilliSeconds but processes events
    static void drowse(int iMilliSeconds);

    //! Returns a map of strings the key=value list in \a sParameters (like: "a=0; b=155; c=foo")
    static QMap<QString, QString> getMapFromCommaSeparatedKeyValues(const QString& sParameters);

    //! Converts \a image to a byte array
    static QByteArray convertQImageToByteArray(const QImage& image, const char* szFormat, int compressionRate);

    //! Converts \a data to a QImage
    static QImage convertByteArrayToQImage(const QByteArray& data, const char* szFormat);
};
