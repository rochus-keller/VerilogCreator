#ifndef VLAUTOCOMPLETER_H
#define VLAUTOCOMPLETER_H

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

#include <texteditor/autocompleter.h>

namespace Vl
{
    class AutoCompleter : public TextEditor::AutoCompleter
    {
    public:
        AutoCompleter() {}

        virtual bool contextAllowsAutoParentheses(const QTextCursor &cursor,
                                                  const QString &textToInsert = QString()) const;
        virtual bool contextAllowsElectricCharacters(const QTextCursor &cursor) const;
        virtual bool isInComment(const QTextCursor &cursor) const;
        virtual bool isInString(const QTextCursor &cursor) const;
        virtual QString insertMatchingBrace(const QTextCursor &cursor,
                                            const QString &text,
                                            QChar la,
                                            int *skippedChars) const;
        virtual QString insertParagraphSeparator(const QTextCursor &cursor) const;
    };

}

#endif // VLAUTOCOMPLETER_H
