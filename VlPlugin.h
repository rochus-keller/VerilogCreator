#ifndef VERILOGCREATOR_H
#define VERILOGCREATOR_H

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

#include "verilogcreator_global.h"

#include <extensionsystem/iplugin.h>

namespace Vl { class EditorWidget1; }
class QAction;

namespace VerilogCreator {
    namespace Internal {

        class VerilogCreatorPlugin : public ExtensionSystem::IPlugin
        {
            Q_OBJECT
            Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "VerilogCreator.json")

        public:
            VerilogCreatorPlugin();
            ~VerilogCreatorPlugin();

            static VerilogCreatorPlugin* instance();

            bool initialize(const QStringList &arguments, QString *errorString);
            void extensionsInitialized();
            ShutdownFlag aboutToShutdown();

        public slots:
            void onFindUsages();
            void onGotoOuterBlock();
            void onReloadProject();

        protected:
            Vl::EditorWidget1* currentEditorWidget();
            void initializeToolsSettings();
            static VerilogCreatorPlugin* d_instance;
        private:
            QAction* d_findUsagesAction;
            QAction* d_gotoOuterBlockAction;
            QAction* d_reloadProject;
        };

    } // namespace Internal
} // namespace VerilogCreator

#endif // VERILOGCREATOR_H

