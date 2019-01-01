#ifndef VLPROJECTMANAGER_H
#define VLPROJECTMANAGER_H

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

#if VL_QTC_VER < 0405
#include <projectexplorer/iprojectmanager.h>

namespace Vl
{
    class ProjectManager : public ProjectExplorer::IProjectManager
    {
        Q_OBJECT
    public:
        explicit ProjectManager();

        virtual QString mimeType() const;
        virtual ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);

    };
}
#endif
#endif // VLPROJECTMANAGER_H
