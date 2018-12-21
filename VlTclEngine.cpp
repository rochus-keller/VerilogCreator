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

#include "VlTclEngine.h"
#include <tcl/tcl.h>
#include <QtDebug>
#include <QLibrary>
using namespace Vl;

static TclEngine* s_inst = 0;
static int s_refCount = 0;

// Inspired by https://github.com/JPNaude/Qtilities/blob/master/src/Examples/TclScriptingExample/qtclconsole.cpp

static int writeStdout(ClientData, CONST char * buf, int toWrite, int *errorCode);

static int writeStderr(ClientData, CONST char * buf, int toWrite, int *errorCode);

static int closeChannel(ClientData, Tcl_Interp *)
{
    return EINVAL;
}

static Tcl_ChannelType consoleOutputChannelType =
{
    (char*)"console1", /* const char* typeName*/
    NULL, /*Tcl_ChannelTypeVersion version*/
    closeChannel /*NULL*/, /*Tcl_DriverCloseProc* closeProc*/
    NULL, /*Tcl_DriverInputProc* inputProc*/
    writeStdout, /*Tcl_DriverOutputProc* outputProc*/
    NULL, /*Tcl_DriverSeekProc* seekProc*/
    NULL, /*Tcl_DriverSetOptionProc*  setOptionProc*/
    NULL, /*Tcl_DriverGetOptionProc* getOptionProc*/
    NULL, /*Tcl_DriverWatchProc* watchProc*/
    NULL, /*Tcl_DriverGetHandleProc* getHandleProc*/
    NULL, /*Tcl_DriverClose2Proc* close2Proc*/
    NULL, /*Tcl_DriverBlockModeProc* blockModeProc*/
    NULL, /*Tcl_DriverFlushProc* flushProc*/
    NULL, /*Tcl_DriverHandlerProc * handlerProc*/
    NULL, /*Tcl_DriverWideSeekProc * wideSeekProc*/
    NULL, /*Tcl_DriverThreadActionProc* threadActionProc*/
    NULL /*Tcl_DriverTruncateProc* truncateProc*/
};

static Tcl_ChannelType consoleErrorChannelType =
{
    (char*)"console2", /* const char* typeName*/
   NULL, /*Tcl_ChannelTypeVersion version*/
   closeChannel /*NULL*/, /*Tcl_DriverCloseProc* closeProc*/
   NULL, /*Tcl_DriverInputProc* inputProc*/
   writeStderr, /*Tcl_DriverOutputProc* outputProc*/
   NULL, /*Tcl_DriverSeekProc* seekProc*/
   NULL, /*Tcl_DriverSetOptionProc*  setOptionProc*/
   NULL, /*Tcl_DriverGetOptionProc* getOptionProc*/
   NULL, /*Tcl_DriverWatchProc* watchProc*/
   NULL, /*Tcl_DriverGetHandleProc* getHandleProc*/
   NULL, /*Tcl_DriverClose2Proc* close2Proc*/
   NULL, /*Tcl_DriverBlockModeProc* blockModeProc*/
   NULL, /*Tcl_DriverFlushProc* flushProc*/
   NULL, /*Tcl_DriverHandlerProc * handlerProc*/
   NULL, /*Tcl_DriverWideSeekProc * wideSeekProc*/
   NULL, /*Tcl_DriverThreadActionProc* threadActionProc*/
   NULL /*Tcl_DriverTruncateProc* truncateProc*/
};

struct TclEngine::Imp
{
    typedef Tcl_Interp* (*CreateInterp)();
    CreateInterp d_CreateInterp;
    typedef void (*DeleteInterp)(Tcl_Interp *interp);
    DeleteInterp d_DeleteInterp;
    typedef int	(*Eval)(Tcl_Interp *interp, const char *script);
    Eval d_Eval;
    typedef int	(*EvalFile)(Tcl_Interp *interp, const char *fileName);
    EvalFile d_EvalFile;
    typedef int	(*TraceVar2)(Tcl_Interp *interp, const char *part1, const char *part2, int flags,
                                  Tcl_VarTraceProc *proc, ClientData clientData);
    TraceVar2 d_TraceVar2;
    typedef Tcl_Command	(*CreateObjCommand)(Tcl_Interp *interp, const char *cmdName, Tcl_ObjCmdProc *proc,
                                             ClientData clientData, Tcl_CmdDeleteProc *deleteProc);
    CreateObjCommand d_CreateObjCommand;
    typedef void (*AppendElement)(Tcl_Interp *interp, const char *element);
    AppendElement d_AppendElement;
    typedef void (*WrongNumArgs)(Tcl_Interp *interp, int objc, Tcl_Obj *const objv[], const char *message);
    WrongNumArgs d_WrongNumArgs;
    typedef char * (*GetString)(Tcl_Obj *objPtr);
    GetString d_GetString;
    typedef void (*SetStdChannel)(Tcl_Channel channel, int type);
    SetStdChannel d_SetStdChannel;
    typedef void (*RegisterChannel)(Tcl_Interp *interp, Tcl_Channel chan);
    RegisterChannel d_RegisterChannel;
    typedef void (*SetErrno)(int err);
    SetErrno d_SetErrno;
    typedef Tcl_Channel	(*CreateChannel)(const Tcl_ChannelType *typePtr, const char *chanName, ClientData instanceData, int mask);
    CreateChannel d_CreateChannel;
    typedef int	(*SetChannelOption)(Tcl_Interp *interp, Tcl_Channel chan, const char *optionName, const char *newValue);
    SetChannelOption d_SetChannelOption;
    typedef char * (*GetStringResult)(Tcl_Interp *interp);
    GetStringResult d_GetStringResult;

    Imp():d_tcl(0),d_CreateInterp(0),d_DeleteInterp(0),d_Eval(0),d_EvalFile(0),d_TraceVar2(0),d_AppendElement(0),
        d_WrongNumArgs(0),d_GetString(0),d_SetStdChannel(0),d_RegisterChannel(0),d_SetErrno(0),
        d_CreateChannel(0),d_SetChannelOption(0),d_GetStringResult(0)
    {
        d_lib.setFileName("tcl");
        if( !d_lib.load() )
        {
            qCritical() << "Cannot load libtcl";

            return;
        }
        d_CreateInterp = (CreateInterp)d_lib.resolve("Tcl_CreateInterp");
        d_DeleteInterp = (DeleteInterp)d_lib.resolve("Tcl_DeleteInterp");
        d_Eval = (Eval)d_lib.resolve("Tcl_Eval");
        d_EvalFile = (EvalFile)d_lib.resolve("Tcl_EvalFile");
        d_TraceVar2 = (TraceVar2)d_lib.resolve("Tcl_TraceVar2");
        d_CreateObjCommand = (CreateObjCommand)d_lib.resolve("Tcl_CreateObjCommand");
        d_AppendElement = (AppendElement)d_lib.resolve("Tcl_AppendElement");
        d_WrongNumArgs = (WrongNumArgs)d_lib.resolve("Tcl_WrongNumArgs");
        d_GetString = (GetString)d_lib.resolve("Tcl_GetString");
        d_SetStdChannel = (SetStdChannel)d_lib.resolve("Tcl_SetStdChannel");
        d_RegisterChannel = (RegisterChannel)d_lib.resolve("Tcl_RegisterChannel");
        d_SetErrno = (SetErrno)d_lib.resolve("Tcl_SetErrno");
        d_CreateChannel = (CreateChannel)d_lib.resolve("Tcl_CreateChannel");
        d_SetChannelOption = (SetChannelOption)d_lib.resolve("Tcl_SetChannelOption");
        d_GetStringResult = (GetStringResult)d_lib.resolve("Tcl_GetStringResult");

        d_tcl = d_CreateInterp();
        d_CreateObjCommand(d_tcl, "get_vlpro_var", get_vlpro_var, this, 0 );

        Tcl_Channel outConsoleChannel = d_CreateChannel(&consoleOutputChannelType, "stdout", this, TCL_WRITABLE);
        if( outConsoleChannel )
        {
            d_SetChannelOption(NULL, outConsoleChannel, "-translation", "lf");
            d_SetChannelOption(NULL, outConsoleChannel, "-buffering", "none");
            d_RegisterChannel(d_tcl, outConsoleChannel);
            d_SetStdChannel(outConsoleChannel, TCL_STDOUT);
        }
        Tcl_Channel errConsoleChannel = d_CreateChannel(&consoleErrorChannelType, "stderr", this, TCL_WRITABLE);
        if( errConsoleChannel )
        {
            d_SetChannelOption(NULL, errConsoleChannel, "-translation", "lf");
            d_SetChannelOption(NULL, errConsoleChannel, "-buffering", "none");
            d_RegisterChannel(d_tcl, errConsoleChannel);
            d_SetStdChannel(errConsoleChannel, TCL_STDERR);
        }
    }
    ~Imp()
    {
        if( d_tcl )
            d_DeleteInterp(d_tcl);
    }

    static int get_vlpro_var(ClientData clientData, Tcl_Interp *interp,
        int objc, struct Tcl_Obj *const *objv)
    {
        Imp* imp = static_cast<Imp*>(clientData);
        if( objc != 2 )
        {
            //qDebug() << "get_vlpro_var wrong number of args" << objc;
            imp->d_WrongNumArgs( interp, 1, objv, "string" );
            return TCL_ERROR;
        }
        const char* name = imp->d_GetString(objv[1]);
        //qDebug() << "get_vlpro_var called" << name;
        if( imp->d_getVar.d_cb )
        {
            const QStringList res = imp->d_getVar.d_cb( name,imp->d_getVar.d_data );
            for( int i = 0; i < res.size(); i++ )
            {
                imp->d_AppendElement( interp, res[i].toUtf8().data() );
            }
        }
        return TCL_OK;
    }

    QLibrary d_lib;
    Tcl_Interp* d_tcl;
    struct GetVarCb {
        GetVarCb():d_cb(0),d_data(0){}
        TclEngine::GetVar d_cb;
        void* d_data;
    } d_getVar;
    struct WriteLogCb{
        WriteLogCb():d_cb(0),d_data(0){}
        TclEngine::WriteLog d_cb;
        void* d_data;
    } d_writeLog;
};

static int writeStdout(ClientData clientData, CONST char * buf, int toWrite, int *errorCode)
{
    TclEngine::Imp* imp = static_cast<TclEngine::Imp*>(clientData);
    *errorCode = 0;
    imp->d_SetErrno(0);
    if( imp->d_writeLog.d_cb )
    {
        imp->d_writeLog.d_cb( QByteArray::fromRawData(buf,toWrite), false, imp->d_writeLog.d_data );
        return toWrite;
    }else
        return 0;
}

static int writeStderr(ClientData clientData, CONST char * buf, int toWrite, int *errorCode)
{
    TclEngine::Imp* imp = static_cast<TclEngine::Imp*>(clientData);
    *errorCode = 0;
    imp->d_SetErrno(0);
    if( imp->d_writeLog.d_cb )
    {
        imp->d_writeLog.d_cb( QByteArray::fromRawData(buf,toWrite), true, imp->d_writeLog.d_data );
        return toWrite;
    }else
        return 0;
}

TclEngine::TclEngine(QObject *parent) : QObject(parent)
{
    d_imp = new Imp();
}

TclEngine::~TclEngine()
{
    delete d_imp;
}

bool TclEngine::isReady() const
{
    return d_imp->d_tcl != 0;
}

void TclEngine::setGetVar(TclEngine::GetVar gv, void* data)
{
    d_imp->d_getVar.d_cb = gv;
    d_imp->d_getVar.d_data = data;
}

void TclEngine::setWriteLog(TclEngine::WriteLog w, void* data)
{
    d_imp->d_writeLog.d_cb = w;
    d_imp->d_writeLog.d_data = data;
}

bool TclEngine::runFile(const QString& path)
{
    return d_imp->d_EvalFile( d_imp->d_tcl, path.toUtf8().data() ) == TCL_OK;
}

QString TclEngine::getResult() const
{
    return QString::fromUtf8( d_imp->d_GetStringResult(d_imp->d_tcl) );
}

TclEngine*TclEngine::addRef()
{
    if( s_refCount <= 0 )
    {
        //qDebug() << "TCL create";
        s_refCount = 0;
        s_inst = new TclEngine();
    }
    s_refCount++;
    return s_inst;
}

void TclEngine::release()
{
    s_refCount--;
    if( s_refCount == 0 )
    {
        //qDebug() << "TCL delete";
        delete s_inst;
        s_inst = 0;
    }
}

