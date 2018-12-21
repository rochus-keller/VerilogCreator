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

#include "VlConfigurationFactory.h"
#include "VlProject.h"
#include "VlIcarusConfiguration.h"
#include "VlVerilatorConfiguration.h"
#include "VlYosysConfiguration.h"
#include "VlTclConfiguration.h"
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <QDir>
using namespace Vl;

BuildConfigurationFactory::BuildConfigurationFactory(QObject* parent):
    IBuildConfigurationFactory(parent)
{

}

int BuildConfigurationFactory::priority(const ProjectExplorer::Target* parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<ProjectExplorer::BuildInfo*> BuildConfigurationFactory::availableBuilds(const ProjectExplorer::Target* parent) const
{
    QList<ProjectExplorer::BuildInfo *> result;
    /*
    ProjectExplorer::BuildInfo *info = new ProjectExplorer::BuildInfo(this);
    info->typeName = tr("Test Build 1");
    info->kitId = parent->kit()->id();
    result << info;
    info = new ProjectExplorer::BuildInfo(this);
    info->typeName = tr("Test Build 2");
    info->kitId = parent->kit()->id();
    result << info;
    */
    return result;
}

int BuildConfigurationFactory::priority(const ProjectExplorer::Kit* k, const QString& projectPath) const
{
    return 0;
}

QList<ProjectExplorer::BuildInfo*> BuildConfigurationFactory::availableSetups(const ProjectExplorer::Kit* k,
                                                                              const QString& projectPath) const
{
    // projectPath ist der Pfad zur vlpro-Datei
    QList<ProjectExplorer::BuildInfo*> result;
    ProjectExplorer::BuildInfo* info = new ProjectExplorer::BuildInfo(this);
    info->typeName = IcarusBuildConfig::ID;
    info->displayName = IcarusRunConfiguration::Name;
    info->buildDirectory = Utils::FileName::fromString(QFileInfo(projectPath).path());
    info->kitId = k->id();
    result << info;
    info = new ProjectExplorer::BuildInfo(this);
    info->typeName = VerilatorBuildConfig::ID;
    info->displayName = VerilatorBuildConfig::Name;
    info->buildDirectory = Utils::FileName::fromString(QFileInfo(projectPath).path());
    info->kitId = k->id();
    result << info;
    info = new ProjectExplorer::BuildInfo(this);
    info->typeName = YosysBuildConfig::ID;
    info->displayName = YosysBuildConfig::Name;
    info->buildDirectory = Utils::FileName::fromString(QFileInfo(projectPath).path());
    info->kitId = k->id();
    result << info;
    info = new ProjectExplorer::BuildInfo(this);
    info->typeName = TclBuildConfig::ID;
    info->displayName = TclRunConfiguration::Name;
    info->buildDirectory = Utils::FileName::fromString(QFileInfo(projectPath).path());
    info->kitId = k->id();
    result << info;
    return result;
}

ProjectExplorer::BuildConfiguration* BuildConfigurationFactory::create(ProjectExplorer::Target* parent,
                                                                    const ProjectExplorer::BuildInfo* info) const
{
    // Wird aufgerufen, wenn Projekt neu erstellt wird bzw. restore noch nicht vorhanden ist
    //qDebug() << "BuildConfigurationFactory::create" << info->typeName;
    if( info->typeName == IcarusBuildConfig::ID )
        return IcarusBuildConfig::create( parent, info );
    else if( info->typeName == VerilatorBuildConfig::ID )
        return VerilatorBuildConfig::create( parent, info );
    else if( info->typeName == TclBuildConfig::ID )
        return TclBuildConfig::create( parent, info );
    else if( info->typeName == YosysBuildConfig::ID )
        return YosysBuildConfig::create( parent, info );
    else
        return 0;
}

bool BuildConfigurationFactory::canClone(const ProjectExplorer::Target* parent, ProjectExplorer::BuildConfiguration* source) const
{
    // Wird nicht direkt aufgerufen, sondern clone wird direkt aufgerufen, wenn in GUI gewählt
    Q_UNUSED(parent);
    //qDebug() << "BuildConfigurationFactory::canClone" << source->id().toString();
    return false;
//    if (!canHandle(parent))
//        return false;
//    return source->id() == Constants::BuildConfigId || source->id() == IcarusBuildConfig::ID;
}

ProjectExplorer::BuildConfiguration* BuildConfigurationFactory::clone(ProjectExplorer::Target* parent, ProjectExplorer::BuildConfiguration* source)
{
    //qDebug() << "BuildConfigurationFactory::clone" << source->id().toString();
    if (!canClone(parent, source))
        return 0;
    if( IcarusBuildConfig* bc = qobject_cast<IcarusBuildConfig*>(source) )
        return new IcarusBuildConfig(parent, bc );
    else if( VerilatorBuildConfig* bc = qobject_cast<VerilatorBuildConfig*>(source) )
        return new VerilatorBuildConfig(parent, bc );
    else if( TclBuildConfig* bc = qobject_cast<TclBuildConfig*>(source) )
        return new TclBuildConfig(parent, bc );
    else if( YosysBuildConfig* bc = qobject_cast<YosysBuildConfig*>(source) )
        return new YosysBuildConfig(parent, bc );
    else
        return 0;
}

bool BuildConfigurationFactory::canRestore(const ProjectExplorer::Target* parent, const QVariantMap& map) const
{
    Q_UNUSED(parent);
    if (!canHandle(parent))
        return false;
    const Core::Id id = ProjectExplorer::idFromMap(map);
    return id == IcarusBuildConfig::ID || id == VerilatorBuildConfig::ID ||
            id == YosysBuildConfig::ID || id == TclBuildConfig::ID;
}

ProjectExplorer::BuildConfiguration* BuildConfigurationFactory::restore(ProjectExplorer::Target* parent, const QVariantMap& map)
{
    if (!canRestore(parent, map))
        return 0;
    ProjectExplorer::BuildConfiguration* bc = 0;
    const Core::Id id = ProjectExplorer::idFromMap(map);
    if( id == IcarusBuildConfig::ID )
        bc = new IcarusBuildConfig(parent);
    else if( id == VerilatorBuildConfig::ID )
        bc = new VerilatorBuildConfig(parent);
    else if( id == TclBuildConfig::ID )
        bc = new TclBuildConfig(parent);
    else if( id == YosysBuildConfig::ID )
        bc = new YosysBuildConfig(parent);
    if( bc && bc->fromMap(map) )
        return bc;
    if( bc )
        delete bc;
    return 0;
}

bool BuildConfigurationFactory::canHandle(const ProjectExplorer::Target* t) const
{
    Vl::Project* p = dynamic_cast<Vl::Project*>( t->project() );
    //qDebug() << t->toMap() << p;
    return p != 0;
}

MakeStepFactory::MakeStepFactory(QObject* parent):
    IBuildStepFactory(parent)
{

}

bool MakeStepFactory::canCreate(ProjectExplorer::BuildStepList* parent, Core::Id id) const
{
    Q_UNUSED(parent);
    if( id == TclStep::ID )
        return true;
    return false;
}

ProjectExplorer::BuildStep* MakeStepFactory::create(ProjectExplorer::BuildStepList* parent, Core::Id id)
{
    Q_UNUSED(parent);
    //qDebug() << "MakeStepFactory::create" << id.toString();
    if( id == TclStep::ID )
        return new TclStep(parent);
    else
        return 0;
}

bool MakeStepFactory::canClone(ProjectExplorer::BuildStepList* parent, ProjectExplorer::BuildStep* source) const
{
    return false; // source->id() == IcarusMakeStep::ID || source->id() == IcarusCleanStep::ID;
}

ProjectExplorer::BuildStep* MakeStepFactory::clone(ProjectExplorer::BuildStepList* parent, ProjectExplorer::BuildStep* source)
{
    return 0;
}

bool MakeStepFactory::canRestore(ProjectExplorer::BuildStepList* parent, const QVariantMap& map) const
{
    Q_UNUSED(parent);
    const Core::Id id = ProjectExplorer::idFromMap(map);
    return id == IcarusMakeStep::ID || id == IcarusCleanStep::ID ||
            id == VerilatorMakeStep::ID || id == VerilatorCleanStep::ID ||
            id == TclStep::ID ||
            id == YosysMakeStep::ID || id == YosysCleanStep::ID;
}

ProjectExplorer::BuildStep* MakeStepFactory::restore(ProjectExplorer::BuildStepList* parent, const QVariantMap& map)
{
    if (!canRestore(parent, map))
        return 0;
    ProjectExplorer::BuildStep* bs = 0;
    const Core::Id id = ProjectExplorer::idFromMap(map);
    if( id == IcarusMakeStep::ID )
        bs = new IcarusMakeStep(parent);
    else if( id == IcarusCleanStep::ID )
        bs = new IcarusCleanStep(parent);
    else if( id == VerilatorMakeStep::ID )
        bs = new VerilatorMakeStep(parent);
    else if( id == VerilatorCleanStep::ID )
        bs = new VerilatorCleanStep(parent);
    else if( id == TclStep::ID )
        bs = new TclStep(parent);
   else if( id == YosysMakeStep::ID )
        bs = new YosysMakeStep(parent);
    else if( id == YosysCleanStep::ID )
        bs = new YosysCleanStep(parent);
    if( bs && bs->fromMap(map) )
        return bs;
    if( bs )
        delete bs;
    return 0;
}

QList<Core::Id> MakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList* bc) const
{
    Q_UNUSED(bc);
    // Die hier angegebenen Steps erscheinen in der Create-Liste der Steps
    // canCreate wird dann gar nicht mehr abgefragt, sondern direkt create aufgerufen
    QList<Core::Id> result;
    result << TclStep::ID;
    return result;
}

QString MakeStepFactory::displayNameForId(Core::Id id) const
{
    Q_UNUSED(id);
    if( id == TclStep::ID )
        return tr("TCL Step");
    return QString();
}


RunConfigurationFactory::RunConfigurationFactory(QObject* parent)
{

}

bool RunConfigurationFactory::canCreate(ProjectExplorer::Target* parent, Core::Id id) const
{
    return true;
}

bool RunConfigurationFactory::canRestore(ProjectExplorer::Target* parent, const QVariantMap& map) const
{
    return true;
}

bool RunConfigurationFactory::canClone(ProjectExplorer::Target* parent, ProjectExplorer::RunConfiguration* product) const
{
    return false;
}

ProjectExplorer::RunConfiguration*RunConfigurationFactory::clone(ProjectExplorer::Target* parent, ProjectExplorer::RunConfiguration* product)
{
    return 0;
}

QList<Core::Id> RunConfigurationFactory::availableCreationIds(ProjectExplorer::Target* parent, ProjectExplorer::IRunConfigurationFactory::CreationMode mode) const
{
    Q_UNUSED(mode);
    QList<Core::Id> result;
    if( parent->project()->id() == Project::ID )
        result << IcarusRunConfiguration::ID; // << TclRunConfiguration::ID; //  << VerilatorRunConfiguration::ID;
    return result;
}

QString RunConfigurationFactory::displayNameForId(Core::Id id) const
{
    if( id == IcarusRunConfiguration::ID )
        return IcarusRunConfiguration::Name;
    else if( id == TclRunConfiguration::ID )
        return TclRunConfiguration::Name;
    else
        return QString();
}

ProjectExplorer::RunConfiguration* RunConfigurationFactory::doCreate(ProjectExplorer::Target* parent, Core::Id id)
{
    if( id == IcarusRunConfiguration::ID )
        return new IcarusRunConfiguration(parent);
    else if( id == TclRunConfiguration::ID )
        return new TclRunConfiguration(parent);
    else
        return 0;
}

ProjectExplorer::RunConfiguration* RunConfigurationFactory::doRestore(ProjectExplorer::Target* parent, const QVariantMap& map)
{
    const Core::Id id = ProjectExplorer::idFromMap(map);
    if( id == IcarusRunConfiguration::ID )
        return new IcarusRunConfiguration(parent);
    else if( id == TclRunConfiguration::ID )
        return new TclRunConfiguration(parent);
    else
        return 0;
}
