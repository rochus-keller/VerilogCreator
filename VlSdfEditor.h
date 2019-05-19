#ifndef VLSDFEDITOR_H
#define VLSDFEDITOR_H

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

#include <texteditor/texteditor.h>
#include <texteditor/codeassist/keywordscompletionassist.h>
#include <texteditor/textdocument.h>
#include <texteditor/syntaxhighlighter.h>

namespace Vl
{
    class Editor3 : public TextEditor::BaseTextEditor
    {
        Q_OBJECT
    public:
        explicit Editor3();

    };

    class EditorFactory3 : public TextEditor::TextEditorFactory
    {
        Q_OBJECT
    public:
        EditorFactory3();
    };

    class EditorDocument3 : public TextEditor::TextDocument
    {
        Q_OBJECT
    public:
        EditorDocument3();
    };

    class EditorWidget3 : public TextEditor::TextEditorWidget
    {
        Q_OBJECT
    public:
        EditorWidget3() {}
    protected:
        void contextMenuEvent(QContextMenuEvent *e) Q_DECL_OVERRIDE;

    };

    class Highlighter3 : public TextEditor::SyntaxHighlighter
    {
    public:
        Highlighter3(QTextDocument* = 0);
        void highlightBlock(const QString &text);
    private:
        QTextCharFormat formatForCategory(int i) const { return d_format[i]; }
        enum Category { C_Num, C_Str, C_Kw, C_Ident, C_Op, C_Cmt, C_Max };
        QTextCharFormat d_format[C_Max];
    };
}

#endif // VLSDFEDITOR_H
