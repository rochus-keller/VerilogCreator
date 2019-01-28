#ifndef VLOUTLINEMDL_H
#define VLOUTLINEMDL_H

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

#include <QAbstractItemModel>
#include <Verilog/VlCrossRefModel.h>

namespace Vl
{
    class OutlineMdl1 : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        explicit OutlineMdl1(QObject *parent = 0);

        void setFile( const QString& );

        const CrossRefModel::Symbol* getSymbol( const QModelIndex & ) const;
        QModelIndex findSymbol( const CrossRefModel::Symbol* );
        QModelIndex findSymbol( quint32 line, quint16 col );

        // overrides
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent(const QModelIndex &child) const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;

    protected slots:
        void onCrmUpdated(const QString&);

    protected:
        void fillTop();
        void fillSubs(const CrossRefModel::Branch*, const QByteArray& name );

    private:
        struct Slot
        {
            CrossRefModel::SymRef d_sym;
            QByteArray d_name;
            bool operator<( const Slot& rhs ) const { return qstricmp( d_name, rhs.d_name ) < 0; }
        };
        QList<Slot> d_rows;
        QString d_file;
        CrossRefModel* d_crm;
    };

    class OutlineMdl2 : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        explicit OutlineMdl2(QObject *parent = 0);

        void setFile( const QString& );
        const CrossRefModel::Symbol* getSymbol( const QModelIndex & ) const;
        QModelIndex findSymbol( quint32 line, quint16 col );

        // overrides
        int columnCount ( const QModelIndex & parent = QModelIndex() ) const { return 1; }
        QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
        QModelIndex parent ( const QModelIndex & index ) const;
        int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        Qt::ItemFlags flags ( const QModelIndex & index ) const;

    protected slots:
        void onCrmUpdated(const QString& file);

    private:
        struct Slot
        {
            CrossRefModel::SymRef d_sym;
            QList<Slot*> d_children;
            Slot* d_parent;
            Slot(Slot* p = 0):d_parent(p){ if( p ) p->d_children.append(this); }
            ~Slot() { foreach( Slot* s, d_children ) delete s; }
        };
        void fill(Slot* super, const CrossRefModel::Symbol* sym, QListIterator<CrossRefModel::Section>&);
        void fillTop();
        QModelIndex findSymbol(Slot*, quint32 line, quint16 col ) const;
        Slot d_root;
        QString d_file;
        CrossRefModel* d_crm;
    };
}

#endif // VLOUTLINEMDL_H
