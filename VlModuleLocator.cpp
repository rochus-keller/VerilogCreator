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

#include "VlModuleLocator.h"
#include "VlModelManager.h"
#include <coreplugin/editormanager/editormanager.h>
#include <QDir>
using namespace Vl;

ModuleLocator::ModuleLocator()
{
    setId("VerilogModules");
    setDisplayName(tr("Verilog modules and UDPs in global namespace"));
    setShortcutString(QString(QLatin1Char('m')));
    setIncludedByDefault(false);
}

QList<Core::LocatorFilterEntry> ModuleLocator::matchesFor(QFutureInterface<Core::LocatorFilterEntry>& future,
                                                          const QString& entry)
{
    Q_UNUSED(future);

    QList<Core::LocatorFilterEntry> res;

    CrossRefModel* mdl = ModelManager::instance()->getModelForCurrentProject();
    if( mdl == 0 )
        return res;

    QDir path( ModelManager::instance()->getPathOf(mdl) );

    CrossRefModel::IdentDeclRefList l = mdl->getGlobalNames();

    QStringMatcher matcher(entry, Qt::CaseInsensitive); // ByteArrayMatcher is case sensitive instead

    QPixmap icon(":/verilogcreator/images/block.png");
    foreach(const CrossRefModel::IdentDeclRef& id, l )
    {
        const QString name = QString::fromLatin1(id->tok().d_val);
        if( matcher.indexIn( name ) != -1 )
        {
            res << Core::LocatorFilterEntry( this, name, QVariant::fromValue(CrossRefModel::SymRef(id)),icon);
            res.last().extraInfo = path.relativeFilePath( id->tok().d_sourcePath );
        }
    }
    return res;
}

void ModuleLocator::accept(Core::LocatorFilterEntry selection) const
{
    CrossRefModel::SymRef sym = selection.internalData.value<CrossRefModel::SymRef>();
    Core::EditorManager::openEditorAt( sym->tok().d_sourcePath,
                                       sym->tok().d_lineNr - 1, sym->tok().d_colNr + 0 );
}

void ModuleLocator::refresh(QFutureInterface<void>& future)
{
    Q_UNUSED(future);
}

