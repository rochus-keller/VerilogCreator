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

#include "VlHighlighter.h"
#include "VlModelManager.h"
#include <Verilog/VlPpLexer.h>
#include <texteditor/textdocumentlayout.h>
#include <QBuffer>
using namespace Vl;
using namespace TextEditor;

VerilogHighlighter::VerilogHighlighter(QTextDocument* parent) :
    SyntaxHighlighter(parent)
{
    for( int i = 0; i < C_Max; i++ )
    {
        d_format[i].setFontWeight(QFont::Normal);
        //d_format[i].setUnderlineStyle(QTextCharFormat::NoUnderline);
        d_format[i].setForeground(Qt::black);
        d_format[i].setBackground(Qt::transparent);
    }
    d_format[C_Num].setForeground(QColor(0, 153, 153));
    d_format[C_Str].setForeground(QColor(208, 16, 64));
    d_format[C_Cmt].setForeground(QColor(153, 153, 136));
    d_format[C_Kw].setForeground(QColor(68, 85, 136));
    d_format[C_Kw].setFontWeight(QFont::Bold);
    d_format[C_Op].setForeground(QColor(153, 0, 0));
    d_format[C_Op].setFontWeight(QFont::Bold);
    d_format[C_Type].setForeground(QColor(153, 0, 115));
    d_format[C_Type].setFontWeight(QFont::Bold);
    d_format[C_Pp].setForeground(QColor(0, 134, 179));
    d_format[C_Pp].setFontWeight(QFont::Bold);

    d_format[C_Section].setForeground(QColor(0, 128, 0));
    d_format[C_Section].setBackground(QColor(230, 255, 230));
    //d_format[C_Section].setFontWeight(QFont::Bold);
    //d_format[C_Section].setFontUnderline(true);
    //d_format[C_Section].setUnderlineColor(QColor(0, 200, 0));
    //d_format[C_Section].setFontOverline(true);
}

QTextCharFormat VerilogHighlighter::formatForCategory(int c) const
{
    return d_format[c];
}

void VerilogHighlighter::highlightBlock(const QString& text)
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
        f.setProperty( TokenProp, int(Tok_Comment) );
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

    PpLexer lex;
    lex.setIgnoreAttrs(false);
    lex.setPackAttrs(false);
    lex.setIgnoreComments(false);
    lex.setPackComments(false);
    lex.setSendMacroUsage(true);
    if( ModelManager::instance()->getLastUsed() )
        lex.setCache( ModelManager::instance()->getLastUsed()->getFcache() );

    const QList<Token> tokens =  lex.tokens(text.mid(start));
    for( int i = 0; i < tokens.size(); ++i )
    {
        const Token &t = tokens.at(i);

        if( t.d_substituted )
            continue;

        QTextCharFormat f;
        if( t.d_type == Tok_Comment )
            f = formatForCategory(C_Cmt); // one line comment
        else if( t.d_type == Tok_Section )
        {
            braceDepth++;
            f = formatForCategory(C_Section);
            //f.setFontOverline(true);
            // setExtraAdditionalFormats funktioniert nicht
        }else if( t.d_type == Tok_SectionEnd )
        {
            braceDepth--;
            f = formatForCategory(C_Section);
            // f.setFontUnderline(true);
        }else if( t.d_type == Tok_Lcmt )
        {
            braceDepth++;
            f = formatForCategory(C_Cmt);
            lexerState = 1;
        }else if( t.d_type == Tok_Rcmt )
        {
            braceDepth--;
            f = formatForCategory(C_Cmt);
            lexerState = 0;
        }else if( t.d_type == Tok_Str )
            f = formatForCategory(C_Str);
        else if( tokenIsNumber(t.d_type) )
            f = formatForCategory(C_Num);
        else if( tokenIsDelimiter(t.d_type) )
        {
            switch( t.d_type )
            {
            case Tok_Lpar:
            case Tok_Lbrack:
            case Tok_Lbrace:
            //case Tok_Latt:
                parentheses.append(Parenthesis(Parenthesis::Opened, text[t.d_colNr-1], t.d_colNr-1 ));
                break;
            case Tok_Rpar:
            case Tok_Rbrack:
            case Tok_Rbrace:
            //case Tok_Ratt:
                parentheses.append(Parenthesis(Parenthesis::Closed, text[t.d_colNr-1], t.d_colNr-1 ));
                break;
            }
            if( t.d_type == Tok_LineCont )
                f = formatForCategory(C_Pp);
            else
            //if( tokenIsOperator(t.d_type) )
                f = formatForCategory(C_Op);
//            else if( tokenIsBracket(t.d_type) )
//                f = formatForCategory(C_Brack);
        }else if( tokenIsReservedWord(t.d_type) )
        {
            if( tokenIsBlockBegin(t.d_type) )
            {
                parentheses.append(Parenthesis(Parenthesis::Opened, text[t.d_colNr-1], t.d_colNr-1 ));
                ++braceDepth;
                // if a folding block opens at the beginning of a line, treat the entire line
                // as if it were inside the folding block
                if ( i == 0 && ( t.d_type == Tok_begin || t.d_type == Tok_fork ) )
                {
                    ++foldingIndent;
                    TextDocumentLayout::userData(currentBlock())->setFoldingStartIncluded(true);
                }
            }else if( tokenIsBlockEnd(t.d_type) )
            {
                const int pos = t.d_colNr-1 + ::strlen(tokenToString(t.d_type))-1;
                parentheses.append(Parenthesis(Parenthesis::Closed, text[pos], pos ));
                --braceDepth;
                if (braceDepth < foldingIndent) {
                    // unless we are at the end of the block, we reduce the folding indent
                    if (i == tokens.size()-1 || tokens.at(i+1).d_type == Tok_Semi )
                        TextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                    else
                        foldingIndent = qMin(braceDepth, foldingIndent);
                }
            }
            if( tokenIsType(t.d_type) )
                f = formatForCategory(C_Type);
            else
                f = formatForCategory(C_Kw);
        }else if( t.d_type == Tok_Ident )
            f = formatForCategory(C_Ident);
        else if( t.d_type == Tok_SysName )
            f = formatForCategory(C_Kw);
        else if( t.d_type == Tok_CoDi )
        {
            Directive di = matchDirective(t.d_val);
            switch(di)
            {
            case Cd_ifdef:
            case Cd_ifndef:
                braceDepth++;
                break;
            case Cd_else:
            case Cd_elsif:
                foldingIndent--;
                break;
            case Cd_endif:
                braceDepth--;
                break;
            default:
                break;
            }

            f = formatForCategory(C_Pp);
        }

        if( f.isValid() )
        {
            f.setProperty( TokenProp, int(t.d_type) );
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



