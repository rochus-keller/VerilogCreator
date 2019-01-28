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

#include "VlOutlineWidget.h"
#include "VlEditorWidget.h"
#include "VlEditor.h"
#include "VlOutlineMdl.h"
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>
#include <QtDebug>
#include <texteditor/textdocument.h>
#include <utils/fileutils.h>
#include <coreplugin/editormanager/editormanager.h>
using namespace Vl;

OutlineWidget::OutlineWidget(EditorWidget1 *parent) : TextEditor::IOutlineWidget(),
    d_edit(parent),d_enableCursorSync(true),d_blockCursorSync(false)
{
    d_tree = new OutlineTreeView(this);
    QVBoxLayout* box = new QVBoxLayout(this);
    box->setMargin(0);
    box->setSpacing(0);
    box->addWidget(d_tree);
    // TODO box->addWidget(Core::ItemViewFind::createSearchableWrapper(d_tree));

    d_mdl = new OutlineMdl2( this );
    const QString fileName = parent->textDocument()->filePath().toString();
    d_tree->setModel(d_mdl);
    connect(d_mdl,SIGNAL(modelReset()), d_tree, SLOT(expandAll()) );
    d_mdl->setFile(fileName);

    connect(d_tree, SIGNAL(activated(QModelIndex)), this, SLOT(onItemActivated(QModelIndex)));
    connect(d_edit,SIGNAL(sigGotoSymbol(quint32,quint16)), this, SLOT(onGotoSymbol(quint32,quint16)) );

    //qDebug() << "OutlineWidget creator";
}

OutlineWidget::~OutlineWidget()
{
    //qDebug() << "OutlineWidget destructor";
}

QList<QAction*> OutlineWidget::filterMenuActions() const
{
    return QList<QAction*>();
}

void OutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    d_enableCursorSync = syncWithCursor;
    if (d_enableCursorSync)
        ; // updateSelectionInTree(m_editor->outline()->modelIndex());
}

void OutlineWidget::onItemActivated(QModelIndex index)
{
    if (!index.isValid())
        return;

    const CrossRefModel::Symbol* sym = d_mdl->getSymbol(index);
    if( sym )
    {
        d_blockCursorSync = true;

        Core::EditorManager::cutForwardNavigationHistory();
        Core::EditorManager::addCurrentPositionToNavigationHistory();

        // line has to be 1 based, column 0 based!
        d_edit->gotoLine( sym->tok().d_lineNr, sym->tok().d_colNr - 1);
        d_blockCursorSync = false;
    }
    d_edit->setFocus();
}

void OutlineWidget::onGotoSymbol(quint32 line, quint16 col)
{
    if (!( d_enableCursorSync && !d_blockCursorSync ) )
        return;

    QModelIndex index = d_mdl->findSymbol( line, col );
    d_blockCursorSync = true;
    d_tree->setCurrentIndex(index);
    d_tree->scrollTo(index);
    d_blockCursorSync = false;
}

bool OutlineWidgetFactory::supportsEditor(Core::IEditor* editor) const
{
    if (qobject_cast<Editor1*>(editor))
        return true;
    return false;
}

TextEditor::IOutlineWidget*OutlineWidgetFactory::createWidget(Core::IEditor* editor)
{
    Editor1* e = qobject_cast<Editor1*>(editor);
    EditorWidget1* w = qobject_cast<EditorWidget1*>(e->widget());

    OutlineWidget *widget = new OutlineWidget(w);

    return widget;
}


OutlineTreeView::OutlineTreeView(QWidget* parent):
    Utils::NavigationTreeView(parent)
{
    setExpandsOnDoubleClick(false);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSortingEnabled(false);
}

void OutlineTreeView::contextMenuEvent(QContextMenuEvent* event)
{
    if (!event)
        return;

    QMenu contextMenu;

    contextMenu.addAction(tr("Expand All"), this, SLOT(expandAll()));
    contextMenu.addAction(tr("Collapse All"), this, SLOT(collapseAll()));

    contextMenu.exec(event->globalPos());

    event->accept();
}
