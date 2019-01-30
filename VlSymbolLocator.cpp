/*
* Copyright 2019 Rochus Keller <mailto:me@rochus-keller.ch>
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

#include "VlSymbolLocator.h"
#include "VlModelManager.h"
#include <Verilog/VlSynTree.h>
#include <coreplugin/editormanager/editormanager.h>
using namespace Vl;

SymbolLocator::SymbolLocator()
{
    setId("Symbols");
    setDisplayName(tr("Verilog symbols in current document"));
    setShortcutString(QString(QLatin1Char('.')));
    setIncludedByDefault(false);
}

static bool lessThan(const Core::LocatorFilterEntry&s1, const Core::LocatorFilterEntry&s2)
{
    return s1.displayName.compare( s2.displayName, Qt::CaseInsensitive ) < 0 ||
            ( !(s2.displayName.compare( s1.displayName, Qt::CaseInsensitive ) < 0)
              && s1.extraInfo.compare( s2.extraInfo, Qt::CaseInsensitive ) < 0 );
}

QList<Core::LocatorFilterEntry> SymbolLocator::matchesFor(QFutureInterface<Core::LocatorFilterEntry>& future, const QString& entry)
{
    Q_UNUSED(future);

    QList<Core::LocatorFilterEntry> res;

    CrossRefModel* mdl = ModelManager::instance()->getModelForCurrentProject();
    if( mdl == 0 )
        return res;

    const QString fileName = Core::EditorManager::instance()->currentDocument() ?
            Core::EditorManager::instance()->currentDocument()->filePath().toString() : QString();
    if( fileName.isEmpty() )
        return res;

    CrossRefModel::IdentDeclRefList l = mdl->getGlobalNames();

    QStringMatcher matcher(entry, Qt::CaseInsensitive); // ByteArrayMatcher is case sensitive instead

    QPixmap iMod(":/verilogcreator/images/block.png");
    QPixmap iVar(":/verilogcreator/images/var.png");
    QPixmap iFunc(":/verilogcreator/images/func.png");

    foreach(const CrossRefModel::IdentDeclRef& id, l )
    {
        if( id->tok().d_sourcePath != fileName )
            continue;

        const QString name = QString::fromLatin1(id->tok().d_val);
        if( matcher.indexIn( name ) != -1 )
        {
            res << Core::LocatorFilterEntry( this, name, QVariant::fromValue(CrossRefModel::SymRef(id)),iMod);
            res.back().extraInfo = QString("(%1)").arg( SynTree::rToStr( id->decl()->tok().d_type ) );
        }
        const CrossRefModel::Scope* s = id->decl()->toScope();
        if( s )
        {
            foreach( const CrossRefModel::IdentDeclRef& id2, s->getNames() )
            {
                const QString name2 = QString::fromLatin1(id2->tok().d_val);
                if( matcher.indexIn( name2 ) != -1 )
                {
                    const int type = id2->decl()->tok().d_type;
                    QPixmap icon;
                    switch(type)
                    {
                    case SynTree::R_task_declaration:
                    case SynTree::R_function_declaration:
                        icon = iFunc;
                        break;
                    default:
                        icon = iVar;
                        break;
                    }
                    res << Core::LocatorFilterEntry( this, name2, QVariant::fromValue(CrossRefModel::SymRef(id2)), icon );
//                    res << Core::LocatorFilterEntry( this, QString("%1.%2").arg(name).arg(name2),
//                                                     QVariant::fromValue(CrossRefModel::SymRef(id2)), icon );
                    res.back().extraInfo = QString("%1 (%2)").arg(name).arg( SynTree::rToStr( type ) );
                    //res.back().extraInfo = QString("(%1)").arg( SynTree::rToStr( type ) );
                }
            }
        }

    }
    std::sort( res.begin(), res.end(), lessThan );
    return res;
}

void SymbolLocator::accept(Core::LocatorFilterEntry selection) const
{
    CrossRefModel::SymRef sym = selection.internalData.value<CrossRefModel::SymRef>();
    Core::EditorManager::openEditorAt( sym->tok().d_sourcePath,
                                       sym->tok().d_lineNr - 1, sym->tok().d_colNr + 0 );
}

void SymbolLocator::refresh(QFutureInterface<void>& future)
{
    Q_UNUSED(future);
}

