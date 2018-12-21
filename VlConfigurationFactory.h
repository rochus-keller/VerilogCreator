#ifndef VLCONFIGURATIONFACTORY_H
#define VLCONFIGURATIONFACTORY_H

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

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/runconfiguration.h>

namespace Vl
{
    class BuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
    {
        Q_OBJECT
    public:
        explicit BuildConfigurationFactory(QObject *parent = 0);

        int priority(const ProjectExplorer::Target *parent) const;
        QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const;
        int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const;
        QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                            const QString &projectPath) const;
        ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                    const ProjectExplorer::BuildInfo *info) const;

        bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
        ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
        bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
        ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);

    private:
        bool canHandle(const ProjectExplorer::Target *t) const;

    };

    class MakeStepFactory : public ProjectExplorer::IBuildStepFactory
    {
        Q_OBJECT
    public:
        explicit MakeStepFactory(QObject *parent = 0);

        bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
        ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);
        bool canClone(ProjectExplorer::BuildStepList *parent,
                      ProjectExplorer::BuildStep *source) const;
        ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                          ProjectExplorer::BuildStep *source);
        bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
        ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent,
                                            const QVariantMap &map);

        QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *bc) const;
        QString displayNameForId(Core::Id id) const;
    };

    class RunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
    {
        Q_OBJECT

    public:
        explicit RunConfigurationFactory(QObject *parent = 0);

        bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const;
        bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
        bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const;
        ProjectExplorer::RunConfiguration* clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product);

        QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const;
        QString displayNameForId(Core::Id id) const;

    private:
        ProjectExplorer::RunConfiguration* doCreate(ProjectExplorer::Target *parent, Core::Id id);
        ProjectExplorer::RunConfiguration* doRestore(ProjectExplorer::Target *parent,
                                                     const QVariantMap &map);
    };
}

#endif // VLCONFIGURATIONFACTORY_H
