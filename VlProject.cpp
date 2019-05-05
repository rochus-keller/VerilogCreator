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

#include "VlProject.h"
#include "VlProjectManager.h"
#include "VlModelManager.h"
#include <Verilog/VlProjectConfig.h>
#include "VlConstants.h"
#include <Verilog/VlCrossRefModel.h>
#include <Verilog/VlIncludes.h>
#include <Verilog/VlPpSymbols.h>
#include <Verilog/VlErrors.h>
#include <texteditor/textdocument.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <coreplugin/icontext.h>

namespace Vl
{
class ProjectNode : public ProjectExplorer::ProjectNode
{
public:
    ProjectNode(const Utils::FileName &projectFilePath)
        : ProjectExplorer::ProjectNode(projectFilePath)
    {
    }

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *) const Q_DECL_OVERRIDE
        { return QList<ProjectExplorer::ProjectAction>(); }

    bool canAddSubProject(const QString &) const Q_DECL_OVERRIDE { return false; }

    bool addSubProjects(const QStringList &) Q_DECL_OVERRIDE { return false; }

    bool removeSubProjects(const QStringList &) Q_DECL_OVERRIDE { return false; }

    bool addFiles(const QStringList &, QStringList*) Q_DECL_OVERRIDE { return false; }
    bool removeFiles(const QStringList &, QStringList*) Q_DECL_OVERRIDE { return false; }
    bool deleteFiles(const QStringList &) Q_DECL_OVERRIDE { return false; }
    bool renameFile(const QString &, const QString &) Q_DECL_OVERRIDE { return false; }
};
}
using namespace Vl;

const char* Project::ID = "VerilogCreator.Project";

Project::Project(ProjectManager* projectManager, const QString& fileName):
    d_projectManager(projectManager),d_root(0)
{
    setId(ID);
    setProjectContext(Core::Context("VerilogCreator.ProjectContext"));
    setProjectLanguages(Core::Context(Constants::LangVerilog));

    d_document = new TextEditor::TextDocument();
    d_document->setFilePath(Utils::FileName::fromString(fileName));
    d_name = QFileInfo(fileName).baseName();
    d_root = new ProjectNode(Utils::FileName::fromString(fileName));
    d_root->setDisplayName(d_name);
    loadProject(fileName);
    d_watcher.addPath(fileName);
    connect( &d_watcher, SIGNAL(fileChanged(QString)), this, SLOT(onFileChanged(QString)) );
}

void Project::reload()
{
    loadProject(d_document->filePath().toString());
}

QString Project::displayName() const
{
    return d_name;
}

Core::IDocument*Project::document() const
{
    return d_document;
}

ProjectExplorer::IProjectManager* Project::projectManager() const
{
    return d_projectManager;
}

ProjectExplorer::ProjectNode*Project::rootProjectNode() const
{
    return d_root;
}

QStringList Project::files(Project::FilesMode) const
{
    return d_config.getSrcFiles() + d_config.getLibFiles();
}

void Project::loadProject(const QString& fileName)
{
    d_root->removeFolderNodes( d_root->subFolderNodes() );
    d_root->removeFileNodes( d_root->fileNodes() );

    CrossRefModel* mdl = ModelManager::instance()->getModelForFile(fileName);
    mdl->clear();
    mdl->getIncs()->clear();
    mdl->getSyms()->clear();
    mdl->getErrs()->clear();

    d_root->addFileNodes(QList<ProjectExplorer::FileNode*>() <<
                         new ProjectExplorer::FileNode(Utils::FileName::fromString(fileName),
                                                       ProjectExplorer::ProjectFileType, false ) );

    ProjectExplorer::FolderNode* libsFolder = new ProjectExplorer::FolderNode(Utils::FileName::fromString("Libraries"));
    d_root->addFolderNodes(QList<ProjectExplorer::FolderNode*>() << libsFolder);

    ProjectExplorer::FolderNode* sourceFolder = new ProjectExplorer::FolderNode(Utils::FileName::fromString("Sources"));
    d_root->addFolderNodes(QList<ProjectExplorer::FolderNode*>() << sourceFolder);


    if( !d_config.loadFromFile(fileName) )
        return; // TODO: Error Message

    const QString oldCur = QDir::currentPath();
    QDir::setCurrent(QFileInfo(fileName).path());
    fillNode( d_config.getLibFiles(), libsFolder );
    fillNode( d_config.getSrcFiles(), sourceFolder );
    QDir::setCurrent(oldCur);

    d_config.setup( mdl );
    emit fileListChanged();
}

void Project::fillNode(const QStringList& files, ProjectExplorer::FolderNode* root)
{
    int i = 0;
    QString path;
    ProjectExplorer::FolderNode* cur = root;
    while( i < files.size() )
    {
        QFileInfo info(files[i]);
        if( info.path() != path )
        {
            path = info.path();
            if( info.dir() == QDir::current() )
                cur = root;
            else
            {
                cur = new ProjectExplorer::FolderNode(Utils::FileName::fromString(
                                                          QDir::current().relativeFilePath(path) ) );
                root->addFolderNodes(QList<ProjectExplorer::FolderNode*>() << cur );

            }
        }
        cur->addFileNodes(QList<ProjectExplorer::FileNode*>() <<
                     new ProjectExplorer::FileNode(Utils::FileName::fromString(files[i]),
                                                                    ProjectExplorer::SourceType, false));
        i++;
    }
}

#if VL_QTC_VER >= 0306
ProjectExplorer::Project::RestoreResult Project::fromMap(const QVariantMap &map, QString *errorMessage)
#else
bool Project::fromMap(const QVariantMap& map)
#endif
{
    // Diese Funktion wird von Explorer-Plugin immer aufgerufen, auch wenn .user noch nicht existiert

#if VL_QTC_VER >= 0306
    if (ProjectExplorer::Project::fromMap(map,errorMessage) != Project::RestoreResult::Ok )
        return Project::RestoreResult::Error;
#else
    if (!ProjectExplorer::Project::fromMap(map))
        return false;
#endif

    // aus GenericProject
    ProjectExplorer::Kit *defaultKit = ProjectExplorer::KitManager::defaultKit();
    if ( !activeTarget() )
        addTarget(createTarget(defaultKit) ); // defaultKit darf null sein, es kommt dann einfach ein Fehlertext im GUI

    // Sanity check: We need both a buildconfiguration and a runconfiguration!
    QList<ProjectExplorer::Target*> targetList = targets();
    foreach (ProjectExplorer::Target *t, targetList) {
        if (!t->activeBuildConfiguration()) {
            // see ProjectExplorer::BuildConfiguration und IBuildConfigurationFactory
            // see also BuildStep und AbstractProcessStep
            // Beispiele in GenericBuildConfiguration bzw. andere Klassen im GenericProjectManagerPlugin
            // see DefaultDeployConfiguration, LocalApplicationRunConfiguration

            //removeTarget(t);
            //continue;
            qWarning() << "Target" << t->displayName() << "has no build configuration";
        }
        // see DeployConfiguration
        if (!t->activeRunConfiguration())
            // see ProjectExplorer::RunConfiguration and subclasses!
            qWarning() << "Target" << t->displayName() << "has no run configuration";
        /* es wird automatisch QtSupport::CustomExecutableRunConfiguration gesetzt und das ist zugleich auch
         * die einzige verfügbare RunConfiguration
        else
            qDebug() << "Run Configuration:" << t->activeRunConfiguration()->metaObject()->className();
        QList<ProjectExplorer::IRunConfigurationFactory *> rcFactories =
                ProjectExplorer::IRunConfigurationFactory::find(activeTarget());
        foreach( ProjectExplorer::IRunConfigurationFactory * r, rcFactories )
            qDebug() << "Found" << r->metaObject()->className();
            */
    }

    // refresh(Everything);
#if VL_QTC_VER >= 0306
    return Project::RestoreResult::Ok;
#else
    return true;
#endif
}

void Project::onFileChanged(const QString& path)
{
    //qDebug() << "File changed" << path;
    const QString proPath = d_document->filePath().toString();
    if( path == proPath )
    {
        loadProject(path);
        d_watcher.addPath(path);
    }
}

