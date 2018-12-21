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

#include "VlModelManager.h"
#include "VlConstants.h"
#include <Verilog/VlErrors.h>
#include <Verilog/VlCrossRefModel.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/project.h>
#include <projectexplorer/taskhub.h>
#include <utils/fileutils.h>
using namespace Vl;

ModelManager* ModelManager::d_inst = 0;

ModelManager::ModelManager(QObject *parent) : QObject(parent),d_lastUsed(0)
{
    d_fcache = new FileCache(this);
    d_inst = this;
}

ModelManager::~ModelManager()
{
    // Lösche hier explizit damit nicht FileCache gelöscht wird während noch Threads laufen
    QHash<QString,CrossRefModel*>::const_iterator i;
    for( i = d_models.begin(); i != d_models.end(); ++i )
        delete i.value();
    d_inst = 0;
}

CrossRefModel*ModelManager::getModelForFile(const QString& fileName)
{
    if( fileName.isEmpty() )
        return d_lastUsed;
    CrossRefModel*& m = d_models[fileName];
    if( m == 0 )
    {
        m = new CrossRefModel(this,d_fcache);
        connect( m, SIGNAL(sigModelUpdated()), this, SLOT(onModelUpdated()) );
    }
    d_lastUsed = m;
    return m;
}

CrossRefModel*ModelManager::getModelForDir(const QString& dirPath, bool initIfEmpty )
{
    if( dirPath.isEmpty() )
        return 0;
    QFileInfo info(dirPath);
    CrossRefModel* mdl = getModelForFile( info.path() );
    if( initIfEmpty && mdl->isEmpty() )
    {
        QDir dir = info.dir();
        QStringList files = dir.entryList( QStringList() << QString("*.v")
                                               << QString("*.vl"), QDir::Files, QDir::Name );
        for( int i = 0; i < files.size(); i++ )
            files[i] = dir.absoluteFilePath(files[i]);
        mdl->updateFiles(files);
    }
    return mdl;
}

CrossRefModel*ModelManager::getModelForCurrentProject()
{
    CrossRefModel* mdl = 0;
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectTree::currentProject();
    if( currentProject )
        mdl = getModelForFile(currentProject->projectFilePath().toString());
    if( mdl == 0 )
        mdl = getLastUsed();
    return mdl;
}

CrossRefModel*ModelManager::getModelForCurrentProjectOrDirPath(const QString& dirPath, bool initIfEmpty)
{
    CrossRefModel* mdl = ModelManager::instance()->getModelForCurrentProject();
    if( mdl == 0 )
        mdl = ModelManager::instance()->getModelForDir(dirPath, initIfEmpty);
    return mdl;
}

ModelManager*ModelManager::instance()
{
    if( d_inst )
        return d_inst;
    new ModelManager();
    return d_inst;
}

void ModelManager::onModelUpdated()
{
    // TODO: optional ein- oder ausschaltbar

    CrossRefModel* mdl = static_cast<CrossRefModel*>( sender() );

    ProjectExplorer::TaskHub::clearTasks( Vl::Constants::TaskId );

    typedef QPair<QString,quint32> FileLine;
    typedef QPair<QString,bool> Message;
    typedef QMultiMap<FileLine,Message> Lines;
    Lines lines;
    Errors::EntriesByFile errs = mdl->getErrs()->getErrors();
    for( Errors::EntriesByFile::const_iterator j = errs.begin(); j != errs.end(); ++j )
    {
        foreach (const Errors::Entry& e, j.value() )
        {
            lines.insert( qMakePair( j.key(), e.d_line ), qMakePair( e.d_msg, true ) );
        }
    }
    Errors::EntriesByFile wrns = mdl->getErrs()->getWarnings();
    for( Errors::EntriesByFile::const_iterator j = wrns.begin(); j != wrns.end(); ++j )
    {
        foreach (const Errors::Entry& e, j.value() )
        {
            lines.insert( qMakePair( j.key(), e.d_line ), qMakePair( e.d_msg, false ) );
        }
    }

    for( Lines::const_iterator i = lines.begin(); i != lines.end(); ++i )
    {
        // TaskHub sortiert nicht selber
        ProjectExplorer::TaskHub::addTask( i.value().second ? ProjectExplorer::Task::Error : ProjectExplorer::Task::Warning,
                                           i.value().first,
                                           Vl::Constants::TaskId,
                                           Utils::FileName::fromString(i.key().first),
                                           i.key().second );
    }
}

