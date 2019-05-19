#include "VlSdfEditor.h"
#include "VlConstants.h"
#include "VlIndenter.h"
#include <Sdf/SdfLexer.h>
#include <QApplication>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textdocumentlayout.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <QtDebug>
#include <QMenu>
using namespace Vl;
using namespace TextEditor;

Editor3::Editor3()
{
    addContext(Constants::LangSdf);
}

EditorFactory3::EditorFactory3()
{
    setId(Constants::EditorId3);
    setDisplayName(qApp->translate("OpenWith::Editors", Constants::EditorDisplayName3));
    addMimeType(Constants::SdfMimeType);

    setDocumentCreator([]() { return new EditorDocument3; });
    //setIndenterCreator([]() { return new Indenter; });
    setIndenterCreator([]() { return new VerilogIndenter; });
    setEditorWidgetCreator([]() { return new EditorWidget3; });
    setEditorCreator([]() { return new Editor3; });
    //setAutoCompleterCreator([]() { return new AutoCompleter; });
    //setCompletionAssistProvider(new CompletionAssistProvider);

    setSyntaxHighlighterCreator([]() { return new Highlighter3(); });
    setCommentStyle(Utils::CommentDefinition::CppStyle);
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

EditorDocument3::EditorDocument3()
{
    setId(Constants::EditorId3);
}


void EditorWidget3::contextMenuEvent(QContextMenuEvent* e)
{
    TextEditorWidget::contextMenuEvent(e); // TODO
}


Highlighter3::Highlighter3(QTextDocument* doc):SyntaxHighlighter(doc)
{
    for( int i = 0; i < C_Max; i++ )
    {
        d_format[i].setFontWeight(QFont::Normal);
        d_format[i].setForeground(Qt::black);
        d_format[i].setBackground(Qt::transparent);
    }
    d_format[C_Num].setForeground(QColor(0, 153, 153));
    const QColor brown( 147, 39, 39 );
    d_format[C_Str].setForeground(brown); // QColor(208, 16, 64));
    d_format[C_Cmt].setForeground(QColor(153, 153, 136));
    d_format[C_Kw].setForeground(QColor(68, 85, 136));
    //d_format[C_Kw].setFontWeight(QFont::Bold);
    d_format[C_Op].setForeground(Qt::red); // QColor(153, 0, 0));
    d_format[C_Op].setFontWeight(QFont::Bold);
    // d_format[C_Ident].setForeground(Qt::darkGreen);
}

void Highlighter3::highlightBlock(const QString& text)
{
    const int previousBlockState_ = previousBlockState();
    int lexerState = 0, initialBraceDepth = 0;
    if (previousBlockState_ != -1) {
        lexerState = previousBlockState_ & 0xff;
        initialBraceDepth = previousBlockState_ >> 8;
    }

    int braceDepth = initialBraceDepth;
    int foldingIndent = initialBraceDepth;

    if (TextBlockUserData *userData = TextDocumentLayout::testUserData(currentBlock())) {
        userData->setFoldingIndent(0);
        userData->setFoldingStartIncluded(false);
        userData->setFoldingEndIncluded(false);
    }

    int start = 0;
    if( lexerState == 1 )
    {
        // wir sind in einem Multi Line Comment
        // suche das Ende
        QTextCharFormat f = formatForCategory(C_Cmt);
        int pos = text.indexOf("*/");
        if( pos == -1 )
        {
            // the whole block ist part of the comment
            setFormat( start, text.size(), f );
            TextDocumentLayout::clearParentheses(currentBlock());
            TextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);
            setCurrentBlockState( (braceDepth << 8) | lexerState);
            return;
        }else
        {
            // End of Comment found
            pos += 2;
            setFormat( start, pos , f );
            lexerState = 0;
            braceDepth--;
            start = pos;
        }
    }

    Parentheses parentheses;
    parentheses.reserve(20);

    Sdf::Lexer lex;
    lex.setIgnoreComments(false);
    lex.setPackComments(false);

    const QList<Sdf::Token> tokens =  lex.tokens(text.mid(start));
    for( int i = 0; i < tokens.size(); ++i )
    {
        const Sdf::Token &t = tokens.at(i);

        QTextCharFormat f;
        if( t.d_type == Sdf::Tok_Comment )
            f = formatForCategory(C_Cmt); // one line comment
        else if( t.d_type == Sdf::Tok_Lcmt )
        {
            braceDepth++;
            f = formatForCategory(C_Cmt);
            lexerState = 1;
        }else if( t.d_type == Sdf::Tok_Rcmt )
        {
            braceDepth--;
            f = formatForCategory(C_Cmt);
            lexerState = 0;
        }else if( t.d_type == Sdf::Tok_Str )
            f = formatForCategory(C_Str);
        else if( t.d_type == Sdf::Tok_Int || t.d_type == Sdf::Tok_Real )
            f = formatForCategory(C_Num);
        else if( Sdf::tokenTypeIsLiteral(t.d_type) )
        {
            switch( t.d_type )
            {
            case Sdf::Tok_Lpar:
                ++braceDepth;
                parentheses.append(Parenthesis(Parenthesis::Opened, text[t.d_colNr-1], t.d_colNr-1 ));
//                if ( i == 0 )
//                {
//                    ++foldingIndent;
//                    TextDocumentLayout::userData(currentBlock())->setFoldingStartIncluded(true);
//                }
                break;
            case Sdf::Tok_Lbrack:
                parentheses.append(Parenthesis(Parenthesis::Opened, text[t.d_colNr-1], t.d_colNr-1 ));
                break;
            case Sdf::Tok_Rpar:
                parentheses.append(Parenthesis(Parenthesis::Closed, text[t.d_colNr-1], t.d_colNr-1 ));
                --braceDepth;
//                if (braceDepth < foldingIndent)
//                {
//                    // unless we are at the end of the block, we reduce the folding indent
//                    if (i == tokens.size()-1 )
//                        TextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
//                    else
//                        foldingIndent = qMin(braceDepth, foldingIndent);
//                }
                break;
            case Sdf::Tok_Rbrack:
                parentheses.append(Parenthesis(Parenthesis::Closed, text[t.d_colNr-1], t.d_colNr-1 ));
                break;
            }
            f = formatForCategory(C_Op);
        }else if( Sdf::tokenTypeIsKeyword(t.d_type) )
            f = formatForCategory(C_Kw);
        else if( t.d_type == Sdf::Tok_Ident )
            f = formatForCategory(C_Ident);

        if( f.isValid() )
        {
            setFormat( t.d_colNr-1, t.d_len, f );
        }
    }

    TextDocumentLayout::setParentheses(currentBlock(), parentheses);
    // if the block is ifdefed out, we only store the parentheses, but

    // do not adjust the brace depth.
    if (TextDocumentLayout::ifdefedOut(currentBlock())) {
        braceDepth = initialBraceDepth;
        foldingIndent = initialBraceDepth;
    }

    TextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);
    setCurrentBlockState((braceDepth << 8) | lexerState );
}
