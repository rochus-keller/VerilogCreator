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

#include "VlHoverHandler.h"
#include "VlModelManager.h"
#include <Verilog/VlCrossRefModel.h>
#include <Verilog/VlPpSymbols.h>
#include <Verilog/VlIncludes.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/project.h>
#include <utils/fileutils.h>
#include <QTextBlock>
#include <QTextStream>
#include <QTextCursor>
#include <QtDebug>
using namespace Vl;

VerilogHoverHandler::VerilogHoverHandler()
{

}

static inline bool isCond( Directive di )
{
    return di == Cd_ifdef || di == Cd_ifndef || di == Cd_elsif || di == Cd_undef;
}

static inline QByteArray escape( QByteArray str )
{
    return str.trimmed();
}

static QString prettyPrint( const PpSymbols::Define& d )
{
    QString str;
    QTextStream out( &str, QIODevice::WriteOnly );

    out << "`define " << d.d_name;
    if( !d.d_args.isEmpty() )
    {
        out << "( ";
        out << d.d_args.join(", ");
        out << " ) ";
    }else
        out << "  ";

    foreach( const Token& t, d.d_toks )
    {
        if( t.d_type < TT_Specials )
            out << t.getName();
        else if( t.d_type  == Tok_Str )
            out << "\"" << escape(t.d_val) << "\"";
        else if( t.d_type == Tok_SysName )
            out << "$" << t.d_val;
        else
            out << t.d_val;
        out << " ";
    }

    return str;
}

void VerilogHoverHandler::identifyMatch(TextEditor::TextEditorWidget* editorWidget, int pos)
{
    QString text = editorWidget->extraSelectionTooltip(pos);

    if( !text.isEmpty() )
    {
        setToolTip(text);
        return;
    }
    // else

    QTextCursor cur(editorWidget->document());
    cur.setPosition(pos);

    const QString file = editorWidget->textDocument()->filePath().toString();
    CrossRefModel* mdl = ModelManager::instance()->getModelForCurrentProjectOrDirPath(file);

    const int line = cur.blockNumber() + 1;
    const int col = cur.columnNumber() + 1;

    int tokPos;
    QList<Token> toks = CrossRefModel::findTokenByPos( cur.block().text(), col, &tokPos,
                                                       mdl->getFcache()->supportSvExt(file) );
    if( tokPos != -1 )
    {
        const Token& t = toks[tokPos];
        if( t.d_type == Tok_CoDi && matchDirective(t.d_val) == Cd_Invalid )
        {
            PpSymbols::Define d = mdl->getSyms()->getSymbol( t.d_val );
            if( !d.d_name.isEmpty() )
                setToolTip( prettyPrint(d) );
        }else if( t.d_type == Tok_Str && tokPos > 0 && toks[tokPos-1].d_type == Tok_CoDi &&
                  matchDirective(toks[tokPos-1].d_val) == Cd_include )
        {
            const QString path = mdl->getIncs()->findPath( t.d_val, file );
            if( !path.isEmpty() )
                setToolTip( path );
        }else if( t.d_type == Tok_Ident && tokPos > 0 && toks[tokPos-1].d_type == Tok_CoDi &&
                  isCond( matchDirective( toks[tokPos-1].d_val) ) )
        {
            PpSymbols::Define d = mdl->getSyms()->getSymbol( t.d_val );
            if( !d.d_name.isEmpty() )
                setToolTip( prettyPrint(d) );
        }else if( t.d_type == Tok_Ident )
        {
            CrossRefModel::TreePath path = mdl->findSymbolBySourcePos( file, line, col );
            if( path.isEmpty() )
                return;
            CrossRefModel::IdentDeclRef decl = mdl->findDeclarationOfSymbol(path.first().data());
            if( decl.data() == 0 )
                return;
            // qDebug() << "declared" << decl->decl()->tok().d_sourcePath << decl->decl()->tok().d_lineNr;
            FileCache* fcache = ModelManager::instance()->getFileCache();
            const QByteArray line = fcache->fetchTextLineFromFile(
                        decl->decl()->tok().d_sourcePath, decl->decl()->tok().d_lineNr );
            QTextStream out(&text);
            QStringList parts = CrossRefModel::qualifiedNameParts(path,true);
            for( int l = 0; l < parts.size(); l++ )
            {
                if( l != 0 )
                    out << endl << QString(l*3,QChar(' ')) << QChar('.');
                out << parts[l];
            }
            int len = decl->decl()->tok().d_len;
            if( len == 0 )
                len = -1;
            // TODO: für Ports in alter Delkaration nicht optimal
            out << endl << QString::fromLatin1(line.mid(decl->decl()->tok().d_colNr-1,len).simplified())
                << " " << QString::fromLatin1(decl->tok().d_val);
            if( decl->decl()->tok().d_sourcePath != file )
                out << endl << tr("declared in ") << QFileInfo(decl->decl()->tok().d_sourcePath).fileName();
            setToolTip( text );
            // NOTE: mit html funktioniert es nicht!
        }
    }
}

