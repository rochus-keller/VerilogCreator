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

#include "VlPlugin.h"
#include "VlProjectManager.h"
#include "VlEditorFactory.h"
#include "VlConstants.h"
#include "VlModelManager.h"
#include "VlEditorWidget.h"
#include "VlConfigurationFactory.h"
#include "VlProject.h"
#include "VlOutlineWidget.h"
#include "VlModuleLocator.h"
#include "VlCompletionAssistProvider.h"
#include "VlSymbolLocator.h"
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projecttree.h>
#include <texteditor/texteditorsettings.h>
#include <utils/mimetypes/mimedatabase.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditorconstants.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>
#include <QtDebug>

using namespace VerilogCreator::Internal;

VerilogCreatorPlugin* VerilogCreatorPlugin::d_instance = 0;

// NOTE: damit *.v mit diesem Plugin assoziiert wird, muss man in QtCreator/Options/Environment die bereits
// bestehende Bindung von text/x-verilog *.v löschen!!!!
// Problem gelöst, indem ich hier Mime text/x-verilog verwende und bestehendes damit überschreibe.

VerilogCreatorPlugin::VerilogCreatorPlugin()
{
    d_instance = this;
}

VerilogCreatorPlugin::~VerilogCreatorPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members

    d_instance = 0;
}

VerilogCreatorPlugin*VerilogCreatorPlugin::instance()
{
    return d_instance;
}

bool VerilogCreatorPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(errorString);
    Q_UNUSED(arguments);

    Vl::ModelManager::instance();

    initializeToolsSettings();

    addAutoReleasedObject(new Vl::EditorFactory1);
    addAutoReleasedObject(new Vl::EditorFactory2);
    addAutoReleasedObject(new Vl::OutlineWidgetFactory);
    addAutoReleasedObject(new Vl::ModuleLocator);
    addAutoReleasedObject(new Vl::SymbolLocator);
    addAutoReleasedObject(new Vl::ProjectManager);
    addAutoReleasedObject(new Vl::MakeStepFactory);
    addAutoReleasedObject(new Vl::BuildConfigurationFactory);
    addAutoReleasedObject(new Vl::RunConfigurationFactory);

//    TODO Core::IWizardFactory::registerFactoryCreator([]() {
//        return QList<Core::IWizardFactory *>() << new ProjectWizard;
//    });

    //addAutoReleasedObject(new ProjectWizard);

    //addAutoReleasedObject(new Vl::CompletionAssistProvider);

    Core::Context context(Vl::Constants::EditorId1);
    Core::ActionContainer *contextMenu1 = Core::ActionManager::createMenu(Vl::Constants::EditorContextMenuId1);
    Core::ActionContainer * contextMenu2 = Core::ActionManager::createMenu(Vl::Constants::EditorContextMenuId2);
    Core::Command *cmd;
    Core::ActionContainer * toolsMenu = Core::ActionManager::createMenu(Vl::Constants::ToolsMenuId);
    toolsMenu->menu()->setTitle(tr("&Verilog"));
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(toolsMenu);


    cmd = Core::ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    contextMenu1->addAction(cmd);
    toolsMenu->addAction(cmd);

    d_findUsagesAction = new QAction(tr("Find Usages"), this);
    cmd = Core::ActionManager::registerAction(d_findUsagesAction, Vl::Constants::FindUsagesCmd, context);
    cmd->setKeySequence(QKeySequence(tr("Ctrl+Shift+U")));
    connect(d_findUsagesAction, SIGNAL(triggered()), this, SLOT(onFindUsages()));
    contextMenu1->addAction(cmd);
    toolsMenu->addAction(cmd);

    d_gotoOuterBlockAction = new QAction(tr("Go to Outer Block"), this);
    cmd = Core::ActionManager::registerAction(d_gotoOuterBlockAction, Vl::Constants::GotoOuterBlockCmd, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("ALT+Up")));
    connect(d_gotoOuterBlockAction, SIGNAL(triggered()), this, SLOT(onGotoOuterBlock()));
    contextMenu1->addAction(cmd);
    toolsMenu->addAction(cmd);

    Core::Command *sep = contextMenu1->addSeparator();

    cmd = Core::ActionManager::command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu1->addAction(cmd);
    contextMenu2->addAction(cmd);

    cmd = Core::ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu1->addAction(cmd);
    contextMenu2->addAction(cmd);

    ProjectExplorer::TaskHub::addCategory(Vl::Constants::TaskId, tr("Verilog Parser"));


    Core::ActionContainer *mproject = Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    d_reloadProject = new QAction(tr("Reload Project"), this);
    cmd = Core::ActionManager::registerAction(d_reloadProject, Vl::Constants::ReloadProjectCmd,
                                              Core::Context(ProjectExplorer::Constants::C_PROJECT_TREE));
    connect(d_reloadProject, SIGNAL(triggered()), this, SLOT(onReloadProject()));
    mproject->addAction(cmd, ProjectExplorer::Constants::G_PROJECT_FILES);

    return true;
}

void VerilogCreatorPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag VerilogCreatorPlugin::aboutToShutdown()
{
    delete Vl::ModelManager::instance();
    return SynchronousShutdown;
}

void VerilogCreatorPlugin::onFindUsages()
{
    if (Vl::EditorWidget1 *editorWidget = currentEditorWidget())
        editorWidget->onFindUsages();

}

void VerilogCreatorPlugin::onGotoOuterBlock()
{
    if (Vl::EditorWidget1 *editorWidget = currentEditorWidget())
        editorWidget->onGotoOuterBlock();
}

void VerilogCreatorPlugin::onReloadProject()
{
    Vl::Project* currentProject = dynamic_cast<Vl::Project*>( ProjectExplorer::ProjectTree::currentProject() );
    if( currentProject )
    {
        currentProject->reload();
    }
}

Vl::EditorWidget1*VerilogCreatorPlugin::currentEditorWidget()
{
    return qobject_cast<Vl::EditorWidget1*>(Core::EditorManager::currentEditor()->widget());
}

void VerilogCreatorPlugin::initializeToolsSettings()
{
    Utils::MimeDatabase::addMimeTypes(QLatin1String(":/VlEditor/VlEditor.mimetypes.xml"));

    Utils::MimeDatabase db;
    //qDebug() << db.mimeTypesForFileName("test.v");
    TextEditor::TextEditorSettings::registerMimeTypeForLanguageId(Vl::Constants::MimeType, Vl::Constants::LangVerilog);
    TextEditor::TextEditorSettings::registerMimeTypeForLanguageId(Vl::Constants::ProjectMimeType, Vl::Constants::LangQmake);

}

