#ifndef VLTCLENGINE_H
#define VLTCLENGINE_H

/*
* Copyright 2018 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the VerilogCreator plugin.
*
* The following is the license that applies to this copy of the
* plugin. For a license to use the plugin under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <QObject>

namespace Vl
{
    class TclEngine : public QObject
    {
    public:
        typedef QStringList (*GetVar)( const QByteArray& name, void* data );
        typedef void (*WriteLog)( const QByteArray& msg, bool err, void* data );

        explicit TclEngine(QObject *parent = 0);
        ~TclEngine();

        bool isReady() const;
        void setGetVar( GetVar, void* data );
        void setWriteLog( WriteLog, void* data );
        bool runFile( const QString& );
        QString getResult() const;

        static TclEngine* addRef();
        static void release();

        struct Imp;
    private:
        Imp* d_imp;
    };
}

#endif // VLTCLENGINE_H
