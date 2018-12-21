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
using namespace Vl;

OutlineMdl::OutlineMdl(QObject *parent) : QAbstractItemModel(parent),d_crm(0)
{

}

void OutlineMdl::setFile(const QString& f)
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

const CrossRefModel::Symbol*OutlineMdl::getSymbol(const QModelIndex& index) const
{
    if( !index.isValid() || d_crm == 0 )
        return 0;
    const int id = index.internalId();
    if( id == 0 )
        return 0;
    Q_ASSERT( id <= d_rows.size() );
    return d_rows[id-1].d_sym.data();
}

QModelIndex OutlineMdl::findSymbol(const CrossRefModel::Symbol* s)
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

QModelIndex OutlineMdl::index(int row, int column, const QModelIndex& parent) const
{
    if( parent.isValid() || row < 0 )
        return QModelIndex();

    return createIndex( row, column, (quint32)row );
}

QModelIndex OutlineMdl::parent(const QModelIndex& index) const
{
    return QModelIndex();
}

int OutlineMdl::rowCount(const QModelIndex& parent) const
{
    if( parent.isValid() )
        return 0;
    else
        return d_rows.size() + 1; // +1 wegen <no symbol>
}

int OutlineMdl::columnCount(const QModelIndex& parent) const
{
    return 1;
}


QVariant OutlineMdl::data(const QModelIndex& index, int role) const
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
        }
        return QVariant();
    }
    return QVariant();
}

Qt::ItemFlags OutlineMdl::flags(const QModelIndex& index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; //  | Qt::ItemIsDragEnabled;
}

void OutlineMdl::onCrmUpdated(const QString& file)
{
    if( file != d_file )
        return;
    beginResetModel();
    d_rows.clear();
    fillTop();
    endResetModel();
}

void OutlineMdl::fillTop()
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
    std::sort( d_rows.begin(), d_rows.end() );
}

void OutlineMdl::fillSubs( const CrossRefModel::Branch* b, const QByteArray& name)
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

