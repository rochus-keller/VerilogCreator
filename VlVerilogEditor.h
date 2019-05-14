#ifndef VLVERILOGEDITOR_H
#define VLVERILOGEDITOR_H

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

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <utils/treeviewcombobox.h>
#include <QTimer>

namespace Core { class SearchResultItem; }

namespace Vl
{
    class Editor1 : public TextEditor::BaseTextEditor
    {
        Q_OBJECT
    public:
        explicit Editor1();

    };

    class EditorFactory1 : public TextEditor::TextEditorFactory
    {
        Q_OBJECT
    public:
        EditorFactory1();
    };

    class EditorDocument1 : public TextEditor::TextDocument
    {
        Q_OBJECT
    public:
        EditorDocument1();
        ~EditorDocument1();

        // overrides
        TextDocument::OpenResult open(QString *errorString, const QString &fileName, const QString &realFileName);
        bool save(QString *errorString, const QString &fileName, bool autoSave);

    signals:
        void sigLoaded();
        void sigStartProcessing();

    protected slots:
        void onChangedContents();
        void onProcess();
    private:
        QTimer d_processorTimer;
        bool d_opening;
    };

    class EditorWidget1 : public TextEditor::TextEditorWidget
    {
        Q_OBJECT
    public:
        EditorWidget1();
        ~EditorWidget1();

        void finalizeInitialization(); // override

    signals:
        void sigGotoSymbol( quint32 line, quint16 col );

    public slots:
        void onFindUsages();
        void onGotoOuterBlock();
        void onFileUpdated( const QString& );
        void onStartProcessing();

    protected:
        Link findLinkAt(const QTextCursor &, bool resolveTarget = true,
                        bool inNextSplit = false) Q_DECL_OVERRIDE;
        void contextMenuEvent(QContextMenuEvent *e) Q_DECL_OVERRIDE;

    protected slots:
        void onUpdateIfDefsOut();
        void onUpdateCodeWarnings();
        void onCursor();
        void onOpenEditor(const Core::SearchResultItem &item);
        void onDocReady();
        void gotoSymbolInEditor();
        void updateToolTip();
    private:
        Utils::TreeViewComboBox* d_outline;
    };

}

#endif // VLVERILOGEDITOR_H
