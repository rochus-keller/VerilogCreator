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

#include "VlProjectManager.h"
#include "VlConstants.h"
#include "VlProject.h"
#include <QFileInfo>
using namespace Vl;

#if VL_QTC_VER < 0405
ProjectManager::ProjectManager()
{

}

QString Vl::ProjectManager::mimeType() const
{
    return QLatin1String(Constants::ProjectMimeType);
}

ProjectExplorer::Project*Vl::ProjectManager::openProject(const QString& fileName, QString* errorString)
{
    if (!QFileInfo(fileName).isFile()) {
        *errorString = tr("Failed opening project '%1': Project is not a file").arg(fileName);
        return 0;
    }

    return new Project(this, fileName);
}
#endif
