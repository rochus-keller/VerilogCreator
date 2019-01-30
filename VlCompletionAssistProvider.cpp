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
#include "VlConstants.h"
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <coreplugin/id.h>
#include <QTextBlock>
#include <QTextDocument>
#include <QtDebug>

namespace Vl
{
    // borrowed from RubyCreator
    static const QString &nameFor(const QString &s)
    {
        return s;
    }
    template<typename T>
    static void addProposalFromSet(QList<TextEditor::AssistProposalItem*> &proposals,
                                   const T &container, const QString &myTyping,
                                   const QIcon &icon, int order = 0)
    {
        foreach (const typename T::value_type &item, container)
        {
            const QString &name = nameFor(item);
            if (myTyping == name)
                continue;

            auto proposal = new TextEditor::AssistProposalItem();

            int indexOfParenthesis = name.indexOf(QLatin1Char('('));
            if (indexOfParenthesis != -1) {
                proposal->setText(name.mid(0, indexOfParenthesis));
                proposal->setDetail(name);
            } else {
                proposal->setText(name);
            }

            proposal->setIcon(icon);
            proposal->setOrder(order);
            proposals << proposal;
        }
    }

    class CompletionAssistProcessor : public TextEditor::IAssistProcessor
    {
    public:
        CompletionAssistProcessor() {}
        ~CompletionAssistProcessor()
        {
            //qDebug() << "CompletionAssistProcessor deleted";
        }
        TextEditor::IAssistProposal* perform(const TextEditor::AssistInterface *ai)
        {
            m_interface.reset(ai);

            if (ai->reason() == TextEditor::IdleEditor)
                return 0;

            const int startPosition = ai->position();

            const QTextBlock block = ai->textDocument()->findBlock(startPosition);
            const int linePosition = startPosition - block.position();
            const QString line = ai->textDocument()->findBlock(startPosition).text();


            const QString myTyping = ai->textAt(startPosition, ai->position() - startPosition);
            const QString fileName = ai->fileName();

            qDebug() << "perform" << line << myTyping;

            QList<TextEditor::AssistProposalItem *> proposals;

            QStringList tmp;
            tmp << "alpha" << "beta" << "gamma";
            addProposalFromSet(proposals, tmp, myTyping, QIcon(), 1);

            if( proposals.isEmpty() )
                return 0;

            TextEditor::GenericProposalModel *model = new TextEditor::GenericProposalModel();
            model->loadContent(proposals);
            TextEditor::IAssistProposal *proposal = new TextEditor::GenericProposal(startPosition, model);
            return proposal;
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
    return editorId == Constants::EditorId1;
}

TextEditor::IAssistProcessor*CompletionAssistProvider::createProcessor() const
{
    return new CompletionAssistProcessor();
}

bool CompletionAssistProvider::isActivationCharSequence(const QString& sequence) const
{
    if( sequence.size() < SeqLen )
        return false;
    const QChar cur = sequence[SeqLen-1];
    const QChar before1 = sequence[SeqLen-2];
    const QChar before2 = sequence[SeqLen-3];
    //qDebug() << "isActivationCharSequence" << before2 << before1 << ch;
    if( cur == '.' || cur == '(' || cur == '`' )
        return true;
    if( false ) // cur.isLetter() || cur == '$' || cur == '_' || cur == '`' )
        return true;
    else
        return false;
}

bool CompletionAssistProvider::isContinuationChar(const QChar& c) const
{
    //qDebug() << "isContinuationChar" << c;
    return c.isLetterOrNumber() || c == '_' || c == '$';
}
