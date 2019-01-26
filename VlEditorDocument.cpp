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

#include "VlEditorDocument.h"
#include "VlConstants.h"
#include <QtDebug>
#include <Verilog/VlCrossRefModel.h>
#include <coreplugin/idocument.h>
#include <utils/fileutils.h>
#include "VlModelManager.h"
using namespace Vl;

static const int s_processIntervalMs = 150;

EditorDocument1::EditorDocument1():d_opening(false)
{
    setId(Constants::EditorId1);
    // hier ist der Name noch nicht bekannt:
    // qDebug() << "doc created" << filePath().toString() << displayName();

    connect( this, SIGNAL(contentsChanged()), this, SLOT(onChangedContents()) );
    d_processorTimer.setSingleShot(true);
    d_processorTimer.setInterval(s_processIntervalMs);
    connect(&d_processorTimer, SIGNAL(timeout()), this, SLOT(onProcess()));

}

EditorDocument1::~EditorDocument1()
{
    // hier ist der Name bekannt:
    // qDebug() << "doc deleted" << filePath().toString() << displayName() ;
    ModelManager::instance()->getFileCache()->removeFile( filePath().toString() );
}

#if VL_QTC_VER >= 0306
TextEditor::TextDocument::OpenResult EditorDocument1::open(QString* errorString, const QString& fileName, const QString& realFileName)
#else
bool EditorDocument1::open(QString* errorString, const QString& fileName, const QString& realFileName)
#endif
{
    //qDebug() << "before open" << fileName << realFileName;
    d_opening = true;
#if VL_QTC_VER >= 0306
    const TextDocument::OpenResult res = TextDocument::open(errorString, fileName, realFileName );
#else
    const bool res = TextDocument::open(errorString, fileName, realFileName );
#endif
    // wird nach EditorWidget::finalizeInitialization aufgerufen!
    d_opening = false;
    // qDebug() << "after open";
    return res;
}

bool EditorDocument1::save(QString* errorString, const QString& fileName, bool autoSave)
{
    const bool res = TextDocument::save(errorString,fileName, autoSave);
    if( !autoSave )
        ModelManager::instance()->getFileCache()->removeFile( filePath().toString() );
    return res;
}

void EditorDocument1::onChangedContents()
{
    if( d_opening )
        emit sigLoaded();
    else
        d_processorTimer.start(s_processIntervalMs);
}

void EditorDocument1::onProcess()
{
    emit sigStartProcessing();
    const QString file = filePath().toString();
    ModelManager::instance()->getFileCache()->addFile( file, plainText().toLatin1() );
    CrossRefModel* mdl = ModelManager::instance()->getModelForCurrentProject();
    if( mdl == 0 )
        mdl = ModelManager::instance()->getModelForDir(file);
    mdl->updateFiles( QStringList() << file );
}



EditorDocument2::EditorDocument2()
{
    setId(Constants::EditorId2);
}
