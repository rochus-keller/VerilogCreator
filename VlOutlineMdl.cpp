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

#include "VlOutlineMdl.h"
#include "VlModelManager.h"
#include <Verilog/VlSynTree.h>
#include <QPixmap>
#include <QtDebug>
using namespace Vl;

OutlineMdl1::OutlineMdl1(QObject *parent) : QAbstractItemModel(parent),d_crm(0)
{

}

void OutlineMdl1::setFile(const QString& f)
{
    if( d_file == f )
        return;
    beginResetModel();
    d_rows.clear();
    if( d_crm )
        disconnect( d_crm, SIGNAL(sigFileUpdated(QString)), this, SLOT( onCrmUpdated(QString) ) );
    d_file = f;
    d_crm = ModelManager::instance()->getModelForCurrentProjectOrDirPath(f);
    fillTop();
    if( d_crm )
        connect( d_crm, SIGNAL(sigFileUpdated(QString)), this, SLOT( onCrmUpdated(QString) ) );
    endResetModel();
}

const CrossRefModel::Symbol*OutlineMdl1::getSymbol(const QModelIndex& index) const
{
    if( !index.isValid() || d_crm == 0 )
        return 0;
    const int id = index.internalId();
    if( id == 0 )
        return 0;
    Q_ASSERT( id <= d_rows.size() );
    return d_rows[id-1].d_sym.data();
}

QModelIndex OutlineMdl1::findSymbol(const CrossRefModel::Symbol* s)
{
    if( s == 0 || s->tok().d_sourcePath != d_file )
        return QModelIndex();

    for( int i = 0; i < d_rows.size(); i++ )
    {
        if( d_rows[i].d_sym.data() == s )
            return createIndex( i+1, 0, (quint32)i+1 );
    }
    return QModelIndex();
}

QModelIndex OutlineMdl1::findSymbol(quint32 line, quint16 col)
{
    for( int i = 0; i < d_rows.size(); i++ )
    {
        if( d_rows[i].d_sym->tok().d_lineNr == line && d_rows[i].d_sym->tok().d_colNr <= col )
            return createIndex( i+1, 0, (quint32)i+1 );
    }
    return QModelIndex();
}

QModelIndex OutlineMdl1::index(int row, int column, const QModelIndex& parent) const
{
    if( parent.isValid() || row < 0 )
        return QModelIndex();

    return createIndex( row, column, (quint32)row );
}

QModelIndex OutlineMdl1::parent(const QModelIndex& index) const
{
    return QModelIndex();
}

int OutlineMdl1::rowCount(const QModelIndex& parent) const
{
    if( parent.isValid() )
        return 0;
    else
        return d_rows.size() + 1; // +1 wegen <no symbol>
}

int OutlineMdl1::columnCount(const QModelIndex& parent) const
{
    return 1;
}


QVariant OutlineMdl1::data(const QModelIndex& index, int role) const
{
    if( !index.isValid() || d_crm == 0 )
        return QVariant();

    const int id = index.internalId();
    if( id == 0 )
    {
        switch (role)
        {
        case Qt::DisplayRole:
            if( !d_rows.isEmpty() )
                return tr("<Select Symbol>");
            else
                return tr("<No Symbols>");
        default:
            return QVariant();
        }
    }
    Q_ASSERT( id <= d_rows.size() );
    const CrossRefModel::Symbol* s = d_rows[id-1].d_sym.data();
    switch( role )
    {
    case Qt::DisplayRole:
        return d_rows[id-1].d_name.data();
    case Qt::ToolTipRole:
        return QVariant();
    case Qt::DecorationRole:
        switch( s->tok().d_type )
        {
        case SynTree::R_module_declaration:
        case SynTree::R_udp_declaration:
            return QPixmap(":/verilogcreator/images/block.png");
        case SynTree::R_module_or_udp_instance:
            return QPixmap(":/verilogcreator/images/var.png");
        case SynTree::R_task_declaration:
        case SynTree::R_function_declaration:
            return QPixmap(":/verilogcreator/images/func.png");
        case Tok_Section:
            return QPixmap(":/verilogcreator/images/category.png");
        }
        return QVariant();
    }
    return QVariant();
}

Qt::ItemFlags OutlineMdl1::flags(const QModelIndex& index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; //  | Qt::ItemIsDragEnabled;
}

void OutlineMdl1::onCrmUpdated(const QString& file)
{
    if( file != d_file )
        return;
    beginResetModel();
    d_rows.clear();
    fillTop();
    endResetModel();
}

void OutlineMdl1::fillTop()
{
    if( d_crm == 0 )
        return;
    CrossRefModel::IdentDeclRefList globals = d_crm->getGlobalNames(d_file);
    foreach( const CrossRefModel::IdentDeclRef& sym, globals )
    {
        const CrossRefModel::Branch* b = sym->decl();
        if( b->tok().d_type == SynTree::R_module_declaration ||
                b->tok().d_type == SynTree::R_udp_declaration )
        {
            Slot s;
            s.d_sym = b;
            s.d_name = sym->tok().d_val;
            d_rows.append( s );
            fillSubs( b, s.d_name );
        }
    }
    foreach( const CrossRefModel::Section& t, d_crm->getSections(d_file) )
    {
        if( !t.d_title.isEmpty() )
        {
            Slot s;
            s.d_sym = new CrossRefModel::Symbol(Token(Tok_Section,t.d_lineFrom,1,0,t.d_title));
            s.d_name = t.d_title;
            d_rows.append( s );
        }
    }
    std::sort( d_rows.begin(), d_rows.end() );
}

void OutlineMdl1::fillSubs( const CrossRefModel::Branch* b, const QByteArray& name)
{
    foreach( const CrossRefModel::SymRef& sym, b->children() )
    {
        if( sym->tok().d_sourcePath == d_file &&
                ( sym->tok().d_type == SynTree::R_task_declaration ||
                sym->tok().d_type == SynTree::R_function_declaration ||
                ( sym->tok().d_type == SynTree::R_module_or_udp_instance && !sym->tok().d_val.isEmpty() ) ) )
        {
            Slot s;
            s.d_sym = sym;
            s.d_name = name + "." + sym->tok().d_val;
            if( sym->tok().d_type == SynTree::R_module_or_udp_instance )
                s.d_name += " : " + sym->toBranch()->super()->tok().d_val;
            d_rows.append( s );
        }
        const CrossRefModel::Branch* b2 = sym->toBranch();
        if( b2 )
        {
            QByteArray name2 = name;
            if( b2->tok().d_type != SynTree::R_module_or_udp_instantiation && !b2->tok().d_val.isEmpty() )
                name2 += "." + b2->tok().d_val;
            fillSubs( b2, name2 );
        }
    }
}

OutlineMdl2::OutlineMdl2(QObject *parent) :
    QAbstractItemModel(parent),d_crm(0)
{

}

void OutlineMdl2::setFile(const QString& f)
{
    if( d_file == f )
        return;
    beginResetModel();
    d_root = Slot();
    if( d_crm )
        disconnect( d_crm, SIGNAL(sigFileUpdated(QString)), this, SLOT( onCrmUpdated(QString) ) );
    d_file = f;
    d_crm = ModelManager::instance()->getModelForCurrentProjectOrDirPath(f);
    fillTop();
    if( d_crm )
        connect( d_crm, SIGNAL(sigFileUpdated(QString)), this, SLOT( onCrmUpdated(QString) ) );
    endResetModel();
}

const CrossRefModel::Symbol*OutlineMdl2::getSymbol(const QModelIndex& index) const
{
    if( !index.isValid() || d_crm == 0 )
        return 0;
    Slot* s = static_cast<Slot*>( index.internalPointer() );
    Q_ASSERT( s != 0 );
    return s->d_sym.constData();
}

QModelIndex OutlineMdl2::findSymbol(quint32 line, quint16 col)
{
    return findSymbol( &d_root, line, col );
}

QVariant OutlineMdl2::data(const QModelIndex& index, int role) const
{
    if( !index.isValid() || d_crm == 0 )
        return QVariant();

    Slot* s = static_cast<Slot*>( index.internalPointer() );
    Q_ASSERT( s != 0 );
    switch( role )
    {
    case Qt::DisplayRole:
        if( s->d_sym->tok().d_type == SynTree::R_module_or_udp_instance )
            return s->d_sym->tok().d_val + " : " + s->d_sym->toBranch()->super()->tok().d_val;
        else
            return s->d_sym->tok().d_val; // + " " + QByteArray::number(s->d_sym->tok().d_lineNr);
    case Qt::ToolTipRole:
        return QVariant();
    case Qt::DecorationRole:
        switch( s->d_sym->tok().d_type )
        {
        case SynTree::R_module_declaration:
        case SynTree::R_udp_declaration:
            return QPixmap(":/verilogcreator/images/block.png");
        case SynTree::R_module_or_udp_instance:
            return QPixmap(":/verilogcreator/images/var.png");
        case SynTree::R_task_declaration:
        case SynTree::R_function_declaration:
            return QPixmap(":/verilogcreator/images/func.png");
        case Tok_Section:
            return QPixmap(":/verilogcreator/images/category.png");
        }
        return QVariant();
    }
    return QVariant();
}

QModelIndex OutlineMdl2::parent ( const QModelIndex & index ) const
{
    if( index.isValid() )
    {
        Slot* s = static_cast<Slot*>( index.internalPointer() );
        Q_ASSERT( s != 0 );
        if( s->d_parent == &d_root )
            return QModelIndex();
        // else
        Q_ASSERT( s->d_parent != 0 );
        Q_ASSERT( s->d_parent->d_parent != 0 );
        return createIndex( s->d_parent->d_parent->d_children.indexOf( s->d_parent ), 0, s->d_parent );
    }else
        return QModelIndex();
}

int OutlineMdl2::rowCount ( const QModelIndex & parent ) const
{
    if( parent.isValid() )
    {
        Slot* s = static_cast<Slot*>( parent.internalPointer() );
        Q_ASSERT( s != 0 );
        return s->d_children.size();
    }else
        return d_root.d_children.size();
}

QModelIndex OutlineMdl2::index ( int row, int column, const QModelIndex & parent ) const
{
    const Slot* s = &d_root;
    if( parent.isValid() )
    {
        s = static_cast<Slot*>( parent.internalPointer() );
        Q_ASSERT( s != 0 );
    }
    if( row < s->d_children.size() && column < columnCount( parent ) )
        return createIndex( row, column, s->d_children[row] );
    else
        return QModelIndex();
}

Qt::ItemFlags OutlineMdl2::flags( const QModelIndex & index ) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; //  | Qt::ItemIsDragEnabled;
}

void OutlineMdl2::onCrmUpdated(const QString& file)
{
    if( file != d_file )
        return;
    beginResetModel();
    d_root = Slot();
    fillTop();
    endResetModel();
}

void OutlineMdl2::fillTop()
{
    if( d_crm == 0 )
        return;
    CrossRefModel::SectionList sections = d_crm->getSections(d_file);
    QListIterator<CrossRefModel::Section> i(sections);
    CrossRefModel::SymRefList globals = d_crm->getGlobalSyms(d_file);

    foreach( const CrossRefModel::SymRef& sym, globals )
    {
        fill( &d_root, sym.data(), i );
    }
}

QModelIndex OutlineMdl2::findSymbol(OutlineMdl2::Slot* slot, quint32 line, quint16 col) const
{
    for( int i = 0; i < slot->d_children.size(); i++ )
    {
        Slot* s = slot->d_children[i];
        if( s->d_sym->tok().d_lineNr == line && s->d_sym->tok().d_colNr <= col )
            return createIndex( i, 0, s );
        QModelIndex index = findSymbol( s, line, col );
        if( index.isValid() )
            return index;
    }
    return QModelIndex();

}

void OutlineMdl2::fill(Slot* super, const CrossRefModel::Symbol* sym , QListIterator<CrossRefModel::Section>& sec)
{
    while( sec.hasNext() && sym->tok().d_lineNr >= sec.peekNext().d_lineFrom )
    {
        CrossRefModel::Section section = sec.next();
        if( !section.d_title.isEmpty() )
        {
            Slot* s = new Slot();
            s->d_parent = super;
            s->d_sym = new CrossRefModel::Symbol(Token(Tok_Section,section.d_lineFrom,1,0,section.d_title));
            super->d_children.append( s );
        }
    }
    switch( sym->tok().d_type )
    {
    case SynTree::R_module_declaration:
    case SynTree::R_udp_declaration:
    case SynTree::R_task_declaration:
    case SynTree::R_function_declaration:
    case SynTree::R_module_or_udp_instance:
        if( !sym->tok().d_val.isEmpty() )
        {
            Slot* s = new Slot();
            s->d_parent = super;
            s->d_sym = sym;
            super->d_children.append( s );
            super = s;
        }
        break;
    }

    foreach( const CrossRefModel::SymRef& sub, sym->children() )
    {
        fill(super, sub.data(), sec );
    }
}

