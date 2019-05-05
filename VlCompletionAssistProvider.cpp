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
#include "VlModelManager.h"
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
#include <Verilog/VlCrossRefModel.h>
#include <Verilog/VlPpSymbols.h>
#include <Verilog/VlSynTree.h>

namespace Vl
{
    class CompletionAssistProcessor : public TextEditor::IAssistProcessor
    {
    public:
        CompletionAssistProcessor() {}
        TextEditor::IAssistProposal* perform(const TextEditor::AssistInterface *ai)
        {
            m_interface.reset(ai);

            if (ai->reason() == TextEditor::IdleEditor)
                return 0;

            const int curPos = ai->position();

            const QTextBlock block = ai->textDocument()->findBlock(curPos);
            const int lineNr = block.blockNumber() + 1;
            const int colNr = curPos - block.position() + 1;
            const QString seq = QChar(' ') + ai->textDocument()->findBlock(curPos).text().left(colNr-1);
            //const QString seq = ai->textAt(curPos, -CompletionAssistProvider::SeqLen );
            const QString fileName = ai->fileName();

            CrossRefModel* mdl = ModelManager::instance()->getModelForCurrentProject();
            if( mdl == 0 )
                return 0;

            const int prefixPos = CompletionAssistProvider::checkSequence(seq,0);
            if( prefixPos > 0 )
                return 0;

            enum { PlainIdent, MacroUse, DotExpand } what = PlainIdent;
            if( seq.size() + prefixPos - 1 >= 0 )
            {
                switch( seq[ seq.size() + prefixPos - 1 ].toLatin1() )
                {
                case '.':
                    what = DotExpand;
                    break;
                case '`':
                    what = MacroUse;
                    break;
                }
            }
            //const QString prefix = seq.right(-prefixPos);
            //qDebug() << prefix << what << seq;

            QList<TextEditor::AssistProposalItem *> proposals;

            if( what == MacroUse )
            {
                const QByteArrayList names = mdl->getSyms()->getNames();
                for( auto i = names.begin(); i != names.end(); ++i )
                {
                    auto proposal = new TextEditor::AssistProposalItem();
                    proposal->setText(QString::fromLatin1((*i)));
                    //proposal->setIcon(icon);
                    //proposal->setOrder(order);
                    proposals << proposal;

                }

            }else // PlainIdent or DotExpand
            {
                CrossRefModel::TreePath p = mdl->findSymbolBySourcePos( fileName, lineNr, colNr, false, true );
                if( p.isEmpty() )
                    return 0;

//                foreach( const CrossRefModel::SymRef& sym, p )
//                    qDebug() << SynTree::rToStr( sym->tok().d_type ) << sym->tok().d_lineNr << sym->tok().d_colNr;
//                if( p.size() > 1 )
//                {
//                    CrossRefModel::dump(p[1].constData(), 0, false);
//                    CrossRefModel::dump(p[0].constData(), 1, false);
//                    //qDebug() << SynTree::rToStr( p[0]->tok().d_type ) << p[0]->tok().d_lineNr << p[0]->tok().d_colNr;
//                }
//                if( !p.isEmpty() )
//                    CrossRefModel::dump(p.first().constData(), 0, true);

                CrossRefModel::Scope::Names2 names;

                if( what == DotExpand )
                {
                    CrossRefModel::ScopeRef module;
                    const CrossRefModel::Branch* branch = CrossRefModel::closestBranch(p);
                    if( branch && branch->tok().d_type == SynTree::R_module_or_udp_instantiation_ )
                    {
                        CrossRefModel::SymRef sym = mdl->findGlobal(branch->tok().d_val);
                        if( sym.constData() )
                            module = sym->toScope();
                    }else if( branch && branch->tok().d_type == SynTree::R_module_or_udp_instance_ )
                    {
                        CrossRefModel::SymRef sym = mdl->findGlobal(branch->super()->tok().d_val);
                        if( sym.constData() )
                            module = sym->toScope();
                    }
                    if( module.constData() )
                    {
                        //qDebug() << "fetching symbols of" << module->tok().d_val;
                        names = module->getNames2(false);
                    }

                }else // PlainIdent
                {
                    CrossRefModel::ScopeRef scope( CrossRefModel::closestScope(p) );
                    if( scope.constData() )
                    {
                        //qDebug() << "fetching symbols of" << scope->tok().d_val;
                        names = scope->getNames2();
                    }
                }

                QPixmap iMod(":/verilogcreator/images/block.png");
                QPixmap iVar(":/verilogcreator/images/var.png");
                QPixmap iFunc(":/verilogcreator/images/func.png");

                for( auto i = names.begin(); i != names.end(); ++i )
                {
                    auto proposal = new TextEditor::AssistProposalItem();
                    proposal->setText(QString::fromLatin1(i.key()));
                    const int declType = i.value()->decl()->tok().d_type;
                    proposal->setDetail( SynTree::rToStr( declType ) );
                    QPixmap icon;
                    switch(declType)
                    {
                    case SynTree::R_module_declaration:
                    case SynTree::R_udp_declaration:
                        icon = iMod;
                        break;
                    case SynTree::R_task_declaration:
                    case SynTree::R_function_declaration:
                        icon = iFunc;
                        break;
                    default:
                        icon = iVar;
                        break;
                    }
                    proposal->setIcon(icon);
                    //proposal->setOrder(order);
                    proposals << proposal;

                }
            }

            if( proposals.isEmpty() )
                return 0;

            TextEditor::GenericProposalModel *model = new TextEditor::GenericProposalModel();
            model->loadContent(proposals);
            TextEditor::IAssistProposal *proposal = new TextEditor::GenericProposal(curPos+prefixPos, model);
            return proposal;
        }
    private:
        QScopedPointer<const TextEditor::AssistInterface> m_interface;
    };
}

using namespace Vl;

static inline bool isInIdentChar( QChar c )
{
    return c.isLetterOrNumber() || c == '_' || c == '$';
}

static inline bool isFirstIdentChar( QChar c )
{
    return c.isLetter() || c == '_' || c == '$';
}

int CompletionAssistProvider::checkSequence(const QString& s, int minLen )
{
    // returns >0 if nothing found
    // else returns position from s.size() leftwards, where text begins
    if( s.isEmpty() )
        return 1;

    for( int off = s.size() - 1; off >= 0; off-- )
    {
        QChar c = s[off];
        if( c == '.' || c == '`' )
            return off - s.size() + 1;
        else if( !isInIdentChar(c) )
        {
            off++;
            if( ( s.size() - off ) < minLen )
                break;
            if( off < s.size() && isFirstIdentChar( s[off] ) )
                return off - s.size();
            else
                return 0;
        }
    }
    return 1;
    // obsolet:
#if 0
    QChar c = s[s.size() - 1];
    //qDebug() << "isActivationCharSequence" << s;
    if( c == '.' || c == '`' )
        return 0;
    if( s.size() < 2 )
        return 1;
    c = s[s.size() - 2];
    if( c == '.' || c == '`' )
        return -1;
    if( s.size() < 3 )
        return 1;
    if( isInIdentChar(c) && isInIdentChar(s[s.size()-2]) && isFirstIdentChar(s[s.size() - 3]) )
    {
        if( s.size() < 4 || !isInIdentChar(s[s.size()-4]) )
            return -3; // Start of document; actually s includes strange content when at start of document
    }
    return 1;
#endif
}

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

bool CompletionAssistProvider::isActivationCharSequence(const QString& s) const
{
    return checkSequence(s,SeqLen-1) <= 0;
}

bool CompletionAssistProvider::isContinuationChar(const QChar& c) const
{
    //qDebug() << "isContinuationChar" << c;
    return c.isLetterOrNumber() || c == '_' || c == '$';
}
