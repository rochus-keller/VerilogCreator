#ifndef VLOUTLINEWIDGET_H
#define VLOUTLINEWIDGET_H

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

#include <texteditor/ioutlinewidget.h>
#include <utils/navigationtreeview.h>

namespace Vl
{
    class EditorWidget1;
    class OutlineMdl2;

    class OutlineTreeView : public Utils::NavigationTreeView
    {
        Q_OBJECT
    public:
        OutlineTreeView(QWidget *parent);

        void contextMenuEvent(QContextMenuEvent *event);
    };

    class OutlineWidget : public TextEditor::IOutlineWidget
    {
        Q_OBJECT
    public:
        explicit OutlineWidget(EditorWidget1 *parent = 0);
        ~OutlineWidget();

        // overrides
        QList<QAction*> filterMenuActions() const;
        void setCursorSynchronization(bool syncWithCursor);
    protected slots:
        void onItemActivated(QModelIndex);
        void onGotoSymbol( quint32 line, quint16 col );
    private:
        EditorWidget1* d_edit;
        OutlineTreeView* d_tree;
        OutlineMdl2* d_mdl;
        bool d_enableCursorSync;
        bool d_blockCursorSync;
    };

    class OutlineWidgetFactory : public TextEditor::IOutlineWidgetFactory
    {
        Q_OBJECT
    public:
        bool supportsEditor(Core::IEditor *editor) const;
        TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor);
    };
}

#endif // VLOUTLINEWIDGET_H
