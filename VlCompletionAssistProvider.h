#ifndef VLCOMPLETIONASSISTPROVIDER_H
#define VLCOMPLETIONASSISTPROVIDER_H

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

#include <texteditor/codeassist/assistenums.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <Verilog/VlToken.h>

namespace Vl
{
    class CompletionAssistProvider : public TextEditor::CompletionAssistProvider
    {
        Q_OBJECT
    public:
        enum { SeqLen = 4 };

        static int checkSequence(const QString& , int minLen = 1 );

        // overrides
        RunType runType() const;
        bool supportsEditor(Core::Id editorId) const;
        TextEditor::IAssistProcessor* createProcessor() const;
        int activationCharSequenceLength() const { return SeqLen; }
        bool isActivationCharSequence(const QString &sequence) const;
        bool isContinuationChar(const QChar &c) const;
    };
}

#endif // VLCOMPLETIONASSISTPROVIDER_H
