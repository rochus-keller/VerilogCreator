#ifndef VLMODULELOCATOR_H
#define VLMODULELOCATOR_H

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

#include <coreplugin/locator/ilocatorfilter.h>

namespace Vl
{
    class ModuleLocator : public Core::ILocatorFilter
    {
        Q_OBJECT
    public:
        explicit ModuleLocator();

        // overrides
        QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                                   const QString &entry);
        void accept(Core::LocatorFilterEntry selection) const;
        void refresh(QFutureInterface<void> &future);

    };
}

#endif // VLMODULELOCATOR_H
