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
#include "VlProjectFile.h"
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
    d_tcl = new TclEngine(this);
    d_tcl->setGetVar(tclGetVar,this);
}

QStringList Project::getConfig(const QString& key) const
{
    return d_config.value(key);
}

QStringList Project::getIncDirs() const
{
    return d_incDirs;
}

QString Project::getTopMod() const
{
    QStringList topmods = d_config.value("TOPMOD");
    if( !topmods.isEmpty() )
        return topmods.first();
    else
        return QString();
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
    return d_srcFiles + d_libFiles;
}

void Project::populateDir(const QDir& dir, ProjectExplorer::FolderNode* parent )
{
    // Obsolet
    QStringList files = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );

    foreach( const QString& f, files )
    {
        QDir newDir( dir.absoluteFilePath(f) );
        ProjectExplorer::FolderNode* newFolder =
                new ProjectExplorer::FolderNode(Utils::FileName::fromString(newDir.dirName() ) );
        parent->addFolderNodes(QList<ProjectExplorer::FolderNode*>() << newFolder);
        populateDir( newDir, newFolder );
    }

    files = dir.entryList( QStringList() << QString("*.v")
                                           << QString("*.vl"),
                                           QDir::Files, QDir::Name );
    for( int i = 0; i < files.size(); i++ )
    {
        files[i] = dir.absoluteFilePath(files[i]);
        parent->addFileNodes(QList<ProjectExplorer::FileNode*>() <<
                             new ProjectExplorer::FileNode(Utils::FileName::fromString(files[i]),
                                                                ProjectExplorer::SourceType, false));
    }
    d_srcFiles += files;
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
    d_srcFiles.clear();
    d_libFiles.clear();
    d_incDirs.clear();
    d_config.clear();

    d_root->addFileNodes(QList<ProjectExplorer::FileNode*>() <<
                         new ProjectExplorer::FileNode(Utils::FileName::fromString(fileName),
                                                       ProjectExplorer::ProjectFileType, false ) );

    ProjectExplorer::FolderNode* libsFolder = new ProjectExplorer::FolderNode(Utils::FileName::fromString("Libraries"));
    d_root->addFolderNodes(QList<ProjectExplorer::FolderNode*>() << libsFolder);

    ProjectExplorer::FolderNode* sourceFolder = new ProjectExplorer::FolderNode(Utils::FileName::fromString("Sources"));
    d_root->addFolderNodes(QList<ProjectExplorer::FolderNode*>() << sourceFolder);

    d_config["SRCEXT"] << ".v"; // Preset
    d_config["LIBEXT"] << ".v";

    ProjectFile p( d_config );
    if( !p.read(fileName) )
        return; // TODO: Error Message

    d_config = p.variables();
    // qDebug() << d_config; // TEST

    QStringList defs = d_config.value("DEFINES");
    defs.sort();
    for( int i = 0; i < defs.size(); i++ )
    {
        defs[i] = "`define " + defs[i];
    }
    if( !mdl->parseString(  defs.join('\n'), fileName ) )
    {
        return;
    }

    const QString oldCur = QDir::currentPath();
    QDir::setCurrent(QFileInfo(fileName).path());

    const QStringList incDirs = d_config.value("INCDIRS");
    foreach( const QString& d, incDirs )
    {
        QFileInfo info(d);
        QString path;
        if( info.isRelative() )
            path = QDir::cleanPath( QDir::current().absoluteFilePath(d) );
        else
            path = info.canonicalPath();
        if( !d_incDirs.contains(path) )
        {
            mdl->getIncs()->addDir( QDir( path ) );
            d_incDirs.append(path);
        }
    }

    QStringList filter = d_config["LIBEXT"];
    for( int i = 0; i < filter.size(); i++ )
        filter[i] = "*" + filter[i];

    QSet<QString> files;

    QStringList libFiles = d_config.value("LIBFILES");
    QStringList libDirs = d_config.value("LIBDIRS");

    findFilesInDirs( libDirs, filter, files );
    QDir lastDir = QDir::current();
    foreach( const QString& f, libFiles )
    {
        QFileInfo info(f);
        if( info.isDir() )
        {
            if( info.isRelative() )
                lastDir = QDir::current().absoluteFilePath(f);
            else
                lastDir = info.absoluteDir();
        }else
        {
            if( info.isRelative() )
                files.insert( lastDir.absoluteFilePath(f) );
            else
                files.insert(f);
        }
    }
    libFiles = files.toList();
    libFiles.sort();

    fillNode( libFiles, libsFolder );
    d_libFiles = libFiles;

    filter = d_config["SRCEXT"];
    for( int i = 0; i < filter.size(); i++ )
        filter[i] = "*" + filter[i];

    files.clear();
    QStringList srcFiles = d_config.value("SRCFILES");
    QStringList srcDirs = d_config.value("SRCDIRS");
    if( srcFiles.isEmpty() && ( srcDirs.isEmpty() || srcDirs.first().startsWith('-') ) )
        srcDirs.prepend(".*");

    findFilesInDirs( srcDirs, filter, files );
    lastDir = QDir::current();
    foreach( const QString& f, srcFiles )
    {
        QFileInfo info(f);
        if( info.isDir() )
        {
            if( info.isRelative() )
                lastDir = QDir::current().absoluteFilePath(f);
            else
                lastDir = info.absoluteDir();
        }else
        {
            if( info.isRelative() )
                files.insert( lastDir.absoluteFilePath(f) );
            else
                files.insert(f);
        }
    }
    srcFiles = files.toList();
    srcFiles.sort();

    fillNode( srcFiles, sourceFolder );

    d_srcFiles = srcFiles;

    mdl->updateFiles( d_srcFiles + d_libFiles );
    emit fileListChanged();

    QDir::setCurrent(oldCur);
}

static void filterFiles( QStringList& in, QSet<QString>& out, QSet<QString>& filter )
{
    if( filter.isEmpty() )
    {
        // Trivialfall
        foreach( const QString& s, in )
            out.insert(s);
    }else
    {
        foreach( const QString& s, in )
        {
            if( !filter.contains( QFileInfo(s).fileName() ) )
                out.insert(s);
        }
    }
    in.clear();
    filter.clear();
}

void Project::findFilesInDirs(const QStringList& dirs, const QStringList& filter, QSet<QString>& files)
{
    int i = 0;
    QStringList files2;
    QSet<QString> filter2;
    while( i < dirs.size() )
    {
        QString dir = dirs[i];
        if( dir.startsWith('-') )
        {
            // Element ist eine auszulassende Datei
            filter2.insert( dir.mid(1).trimmed() );
        }else
        {
            if( !files2.isEmpty() )
                filterFiles( files2, files, filter2 );
            // Element ist ein zu durchsuchendes Verzeichnis
            bool recursive = false;
            if( dir.endsWith( '*' ) )
            {
                recursive = true;
                dir.chop(1);
            }
            findFilesInDir(dir,filter,files2,recursive);
        }
        i++;
    }
    if( !files2.isEmpty() )
        filterFiles( files2, files, filter2 );
}

void Project::findFilesInDir(const QString& dirPath, const QStringList& filter, QStringList& out, bool recursive )
{
    QStringList files;

    QDir dir(dirPath);

    if( recursive )
    {
        files = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot );

        foreach( const QString& f, files )
        {
            findFilesInDir( dir.absoluteFilePath(f), filter, out, recursive );
        }
    }

    if( !filter.isEmpty() )
        files = dir.entryList( filter, QDir::Files );
    else
        files.clear();
    for( int i = 0; i < files.size(); i++ )
    {
        out.append( dir.absoluteFilePath(files[i]) );
    }
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
                cur = new ProjectExplorer::FolderNode(Utils::FileName::fromString( QDir::current().relativeFilePath(path) ) );
                root->addFolderNodes(QList<ProjectExplorer::FolderNode*>() << cur );

            }
        }
        cur->addFileNodes(QList<ProjectExplorer::FileNode*>() <<
                     new ProjectExplorer::FileNode(Utils::FileName::fromString(files[i]),
                                                                    ProjectExplorer::SourceType, false));
        i++;
    }
}

QStringList Project::tclGetVar(const QByteArray& name, void* data)
{
    //qDebug() << "tclGetVar called" << name;
    const QByteArray lower = name.toLower();
    Project* p = static_cast<Project*>(data);
    if( lower == "srcfiles" )
        return p->getSrcFiles();
    else if( lower == "libfiles" )
        return p->getLibFiles();
    else if( lower == "incdirs" )
        return p->getIncDirs();
    else
        return p->getConfig( name );
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
    qDebug() << "File changed" << path;
    const QString proPath = d_document->filePath().toString();
    if( path == proPath )
    {
        loadProject(path);
        d_watcher.addPath(path);
    }
}

