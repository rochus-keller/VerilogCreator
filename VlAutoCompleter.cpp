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

#include "VlAutoCompleter.h"
#include "VlConstants.h"
#include <QTextCursor>
#include <QTextBlock>
#include <QtDebug>
using namespace Vl;

// borrowed parts from RubyCreator

bool AutoCompleter::contextAllowsAutoParentheses(const QTextCursor& cursor, const QString& textToInsert) const
{
    if (isInComment(cursor))
        return false;

    QChar ch;

    if (!textToInsert.isEmpty())
        ch = textToInsert.at(0);

    switch( ch.toLatin1() )
    {
    case '"':
    case '(':
    case '[':
    case '{':
    case ')':
    case ']':
    case '}':
        return true;
    default:
        return false;
    }
}

bool AutoCompleter::contextAllowsElectricCharacters(const QTextCursor& cursor) const
{
    return TextEditor::AutoCompleter::contextAllowsElectricCharacters(cursor);
}

bool AutoCompleter::isInComment(const QTextCursor& cursor) const
{
    const int state = cursor.block().userState();
    if( state != -1 && state & 0xff ) // Zeichen für Multiline Comment aus Highlighter
        return true;

    const QString line = cursor.block().text();
    const int col = cursor.columnNumber();
    const int pos = line.indexOf(QLatin1String("//") );
    if (pos == -1 || pos > col )
        return false;
    else
        return true;
}

bool AutoCompleter::isInString(const QTextCursor& cursor) const
{
    return TextEditor::AutoCompleter::isInString(cursor);
}

QString AutoCompleter::insertMatchingBrace(const QTextCursor& cursor, const QString& text, QChar la, int* skippedChars) const
{
    if (text.length() != 1)
        return QString();

    const QChar ch = text.at(0);
    switch (ch.toLatin1())
    {
    case '"':
        if (la != ch)
            return QString(ch);
        ++*skippedChars;
        break;
    case '(':
        return QStringLiteral(")");

    case '[':
        return QStringLiteral("]");

    case '{':
        return QStringLiteral("}");

    case ')':
    case ']':
    case '}':
    case ';':
        if (la == ch)
            ++*skippedChars;
        break;

    default:
        break;
    } // end of switch

    return QString();
}

QString AutoCompleter::insertParagraphSeparator(const QTextCursor& cursor) const
{
    return TextEditor::AutoCompleter::insertParagraphSeparator(cursor);
}

