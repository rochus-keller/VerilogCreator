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

#include "VlProjectEditor.h"
#include "VlConstants.h"
#include <QApplication>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <QtDebug>
#include <QMenu>
using namespace Vl;
using namespace TextEditor;

Editor2::Editor2()
{
    addContext(Constants::LangQmake);
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

EditorDocument2::EditorDocument2()
{
    setId(Constants::EditorId2);
}

EditorWidget2::EditorWidget2()
{

}

void EditorWidget2::contextMenuEvent(QContextMenuEvent* e)
{
    QPointer<QMenu> menu(new QMenu(this));

    Core::ActionContainer *mcontext = Core::ActionManager::actionContainer(Vl::Constants::EditorContextMenuId2);
    QMenu *contextMenu = mcontext->menu();

    foreach (QAction *action, contextMenu->actions()) {
        menu->addAction(action);
    }

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    if (!menu)
        return;
    delete menu;
}

static std::pair<int, TextEditor::TextStyle> make(int i,TextEditor::TextStyle s){
    return std::pair<int, TextEditor::TextStyle>(i,s);}

Highlighter2::Highlighter2(const Keywords& keywords):m_keywords(keywords)
{
    static QVector<TextStyle> categories;
    categories << C_TYPE << C_KEYWORD << C_COMMENT << C_VISUAL_WHITESPACE;
    setTextFormatCategories(categories);
}

void Highlighter2::highlightBlock(const QString& text)
{
    if (text.isEmpty())
        return;

    QString buf;
    bool inCommentMode = false;

    QTextCharFormat emptyFormat;
    int i = 0;
    for (;;) {
        const QChar c = text.at(i);
        if (inCommentMode) {
            setFormat(i, 1, formatForCategory(ProfileCommentFormat));
        } else {
            if (c.isLetter() || c == QLatin1Char('_') || c == QLatin1Char('.') || c.isDigit()) {
                buf += c;
                setFormat(i - buf.length()+1, buf.length(), emptyFormat);
                if (!buf.isEmpty() && m_keywords.isFunction(buf))
                    setFormat(i - buf.length()+1, buf.length(), formatForCategory(ProfileFunctionFormat));
                else if (!buf.isEmpty() && m_keywords.isVariable(buf))
                    setFormat(i - buf.length()+1, buf.length(), formatForCategory(ProfileVariableFormat));
            } else if (c == QLatin1Char('(')) {
                if (!buf.isEmpty() && m_keywords.isFunction(buf))
                    setFormat(i - buf.length(), buf.length(), formatForCategory(ProfileFunctionFormat));
                buf.clear();
            } else if (c == QLatin1Char('#')) {
                inCommentMode = true;
                setFormat(i, 1, formatForCategory(ProfileCommentFormat));
                buf.clear();
            } else {
                if (!buf.isEmpty() && m_keywords.isVariable(buf))
                    setFormat(i - buf.length(), buf.length(), formatForCategory(ProfileVariableFormat));
                buf.clear();
            }
        }
        i++;
        if (i >= text.length())
            break;
    }

    applyFormatToSpaces(text, formatForCategory(ProfileVisualWhitespaceFormat));
}
