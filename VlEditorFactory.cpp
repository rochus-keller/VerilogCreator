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

#include "VlEditorFactory.h"
#include "VlEditor.h"
#include "VlEditorDocument.h"
#include "VlConstants.h"
#include "VlHighlighter.h"
#include "VlEditorWidget.h"
#include "VlIndenter.h"
#include "VlHoverHandler.h"
#include "VlAutoCompleter.h"
#include "VlCompletionAssistProvider.h"
#include <QApplication>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>
#include <QtDebug>
using namespace Vl;

EditorFactory1::EditorFactory1()
{
    setId(Constants::EditorId1);
    setDisplayName(qApp->translate("OpenWith::Editors", Constants::EditorDisplayName1));
    addMimeType(Constants::MimeType);
    // addMimeType(Constants::ProjectMimeType);

    setDocumentCreator([]() { return new EditorDocument1; });
    setIndenterCreator([]() { return new Indenter; });
    setEditorWidgetCreator([]() { return new EditorWidget1; });
    setEditorCreator([]() { return new Editor1; });
    setAutoCompleterCreator([]() { return new AutoCompleter; });
    setCompletionAssistProvider(new CompletionAssistProvider);
    setSyntaxHighlighterCreator([]() { return new Highlighter1; });
    setCommentStyle(Utils::CommentDefinition::CppStyle);
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);
    setMarksVisible(true);
    addHoverHandler(new HoverHandler);

    setEditorActionHandlers(TextEditor::TextEditorActionHandler::Format
                          | TextEditor::TextEditorActionHandler::UnCommentSelection
                          | TextEditor::TextEditorActionHandler::UnCollapseAll
                          | TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor
                            );
}





EditorFactory2::EditorFactory2()
{
    setId(Constants::EditorId2);
    setDisplayName(qApp->translate("OpenWith::Editors", Constants::EditorDisplayName2));
    addMimeType(Constants::ProjectMimeType);

    setDocumentCreator([]() { return new EditorDocument2; });
    // setIndenterCreator([]() { return new Indenter; });
    setEditorWidgetCreator([]() { return new EditorWidget2; });
    setEditorCreator([]() { return new Editor2; });
    //setAutoCompleterCreator([]() { return new AutoCompleter; });
    //setCompletionAssistProvider(new CompletionAssistProvider);

    QStringList vars;
    vars << "INCDIRS" << "DEFINES" << "LIBDIRS" << "LIBFILES" << "LIBEXT"
               << "SRCDIRS" << "SRCFILES" << "SRCEXT" << "CONFIG" << "TOPMOD"
               << "BUILD_UNDEFS" << "VLTR_UNDEFS" << "VLTR_ARGS" << "YOSYS_UNDEFS" << "YOSYS_CMDS";
    vars.sort();
    QStringList funcs;
    funcs << "member" << "first" << "last" << "cat" << "fromfile" << "eval" << "list" << "sprintf"
                  << "join" << "split" << "basename" << "dirname" << "section" << "find" << "system"
                  << "unique" << "quote" << "escape_expand" << "upper" << "lower" << "re_escape"
                  << "files" << "prompt" << "replace" << "requires" << "greaterThan" << "lessThan"
                  << "equals" << "isEqual" << "exists" << "export" << "clear" << "unset" << "eval"
                  << "CONFIG" << "if" << "isActiveConfig" << "system" << "return" << "break"
                  << "next" << "defined" << "contains" << "infile" << "count" << "isEmpty"
                  << "include" << "load" << "debug" << "error" << "message" << "warning";
    funcs.sort();

    TextEditor::Keywords keywords( vars, funcs , QMap<QString, QStringList>() );
    setSyntaxHighlighterCreator([keywords]() { return new Highlighter2(keywords); });
    setCommentStyle(Utils::CommentDefinition::HashStyle);
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);
    setMarksVisible(true);
    //addHoverHandler(new HoverHandler);

    setEditorActionHandlers(TextEditor::TextEditorActionHandler::Format
                          | TextEditor::TextEditorActionHandler::UnCommentSelection
                          | TextEditor::TextEditorActionHandler::UnCollapseAll
                          | TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor
                            );
}
