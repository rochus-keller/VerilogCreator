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
#include <QtDebug>
using namespace Vl;


bool AutoCompleter::contextAllowsAutoParentheses(const QTextCursor& cursor, const QString& textToInsert) const
{
    qDebug() << "AutoCompleter::contextAllowsAutoParentheses";
    return TextEditor::AutoCompleter::contextAllowsAutoParentheses(cursor,textToInsert);
}

bool AutoCompleter::contextAllowsElectricCharacters(const QTextCursor& cursor) const
{
    qDebug() << "AutoCompleter::contextAllowsElectricCharacters";
    return TextEditor::AutoCompleter::contextAllowsElectricCharacters(cursor);
}

bool AutoCompleter::isInComment(const QTextCursor& cursor) const
{
    qDebug() << "AutoCompleter::isInComment";
    return TextEditor::AutoCompleter::isInComment(cursor);
}

bool AutoCompleter::isInString(const QTextCursor& cursor) const
{
    qDebug() << "AutoCompleter::isInString";
    return TextEditor::AutoCompleter::isInString(cursor);
}

QString AutoCompleter::insertMatchingBrace(const QTextCursor& cursor, const QString& text, QChar la, int* skippedChars) const
{
    qDebug() << "AutoCompleter::insertMatchingBrace" << text << la;
    return TextEditor::AutoCompleter::insertMatchingBrace(cursor, text,la, skippedChars);
}

QString AutoCompleter::insertParagraphSeparator(const QTextCursor& cursor) const
{
    qDebug() << "AutoCompleter::insertParagraphSeparator";
    return TextEditor::AutoCompleter::insertParagraphSeparator(cursor);
}

