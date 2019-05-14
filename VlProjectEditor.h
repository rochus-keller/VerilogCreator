#ifndef VLPROJECTEDITOR_H
#define VLPROJECTEDITOR_H

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
#include <texteditor/codeassist/keywordscompletionassist.h>
#include <texteditor/textdocument.h>
#include <texteditor/syntaxhighlighter.h>

namespace Vl
{

    class Editor2 : public TextEditor::BaseTextEditor
    {
        Q_OBJECT
    public:
        explicit Editor2();

    };

    class EditorFactory2 : public TextEditor::TextEditorFactory
    {
        Q_OBJECT
    public:
        EditorFactory2();
    };

    class EditorDocument2 : public TextEditor::TextDocument
    {
        Q_OBJECT
    public:
        EditorDocument2();
    };

    class EditorWidget2 : public TextEditor::TextEditorWidget
    {
        Q_OBJECT
    public:
        EditorWidget2();
    protected:
        void contextMenuEvent(QContextMenuEvent *e) Q_DECL_OVERRIDE;

    };

    class Highlighter2 : public TextEditor::SyntaxHighlighter
    {
    public:
        enum ProfileFormats {
            ProfileVariableFormat,
            ProfileFunctionFormat,
            ProfileCommentFormat,
            ProfileVisualWhitespaceFormat,
            NumProfileFormats
        };

        explicit Highlighter2(const TextEditor::Keywords &keywords);
        void highlightBlock(const QString &text);

    private:
        const TextEditor::Keywords m_keywords;
    };
}

#endif // VLPROJECTEDITOR_H
