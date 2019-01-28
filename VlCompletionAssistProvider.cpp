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

#include "VlCompletionAssistProvider.h"
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <coreplugin/id.h>
#include <QtDebug>

namespace Vl
{
    class CompletionAssistProcessor : public TextEditor::IAssistProcessor
    {
    public:
        CompletionAssistProcessor() {}
        ~CompletionAssistProcessor()
        {
            qDebug() << "CompletionAssistProcessor deleted";
        }

        TextEditor::IAssistProposal* perform(const TextEditor::AssistInterface *interface)
        {
            m_interface.reset(interface);

            qDebug() << "CompletionAssistProcessor::perform";
            return 0;
        }
        TextEditor::IAssistProposal *immediateProposal(const TextEditor::AssistInterface *)
        {
            qDebug() << "CompletionAssistProcessor::immediateProposal";
            return 0;
        }
    private:
        QScopedPointer<const TextEditor::AssistInterface> m_interface;
    };
}

using namespace Vl;

TextEditor::IAssistProvider::RunType CompletionAssistProvider::runType() const
{
    return TextEditor::IAssistProvider::Synchronous;
}

bool CompletionAssistProvider::supportsEditor(Core::Id editorId) const
{
    qDebug() << "CompletionAssistProvider::supportsEditor" << editorId;
    return false;
}

TextEditor::IAssistProcessor*CompletionAssistProvider::createProcessor() const
{
    qDebug() << "CompletionAssistProvider::createProcessor";
    return new CompletionAssistProcessor();
}
