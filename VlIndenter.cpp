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

#include "VlIndenter.h"
#include "VlHighlighter.h"
#include <texteditor/tabsettings.h>
#include <texteditor/textdocumentlayout.h>
using namespace Vl;

static const int TAB_SIZE = 4;

Indenter::Indenter()
{

}

bool Indenter::isElectricCharacter(const QChar& ch) const
{
    Q_UNUSED(ch);
    return false;
}

void Indenter::indentBlock(QTextDocument* doc, const QTextBlock& block, const QChar& typedChar,
                           const TextEditor::TabSettings& tabSettings)
{
    // aus QtCreator texteditor normalindenter adaptiert
    // TODO: später mal einen intelligenteren Indenter wie bei C++, ist aber nice to have.
    Q_UNUSED(typedChar)

    // At beginning: Leave as is.
    if (block == doc->begin())
        return;

    const QTextBlock previous = block.previous();
    const QString previousText = previous.text();
    // Empty line indicates a start of a new paragraph. Leave as is.
    if (previousText.isEmpty() || previousText.trimmed().isEmpty())
        return;

    // Just use previous line.
    // Skip blank characters when determining the indentation
    int i = 0;
    while (i < previousText.size()) {
        if (!previousText.at(i).isSpace()) {
            tabSettings.indentLine(block, tabSettings.columnAt(previousText, i));
            break;
        }
        ++i;
    }
}

