#ifndef VLCONSTANTS_H
#define VLCONSTANTS_H

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

namespace Vl
{
    namespace Constants
    {
        const char LangVerilog[] = "Verilog";
        const char LangQmake[] = "Qmake";
        const char LangSdf[] = "Sdf";
        const char EditorId1[] = "Verilog.Editor";
        const char TaskId[] = "Verilog.TaskId";
        const char EditorDisplayName1[] = "Verilog Editor";
        const char EditorId2[] = "Verilog.Project.Editor";
        const char EditorDisplayName2[] = "Verilog Project Editor";
        const char MimeType[] = "text/x-verilog";
        const char ProjectMimeType[] = "text/x-verilogcreator-project";
        const char EditorId3[] = "Verilog.Sdf.Editor";
        const char EditorDisplayName3[] = "Sdf Editor";
        const char SdfMimeType[] = "text/x-verilogcreator-sdf";
        const char SettingsId[] = "Verilog.Settings";
        const char EditorContextMenuId1[] = "VerilogEditor.ContextMenu";
        const char EditorContextMenuId2[] = "VerilogProjectEditor.ContextMenu";
        const char ToolsMenuId[] = "VerilogTools.ToolsMenu";
        const char FindUsagesCmd[] = "VerilogEditor.FindUsages";
        const char GotoOuterBlockCmd[] = "VerilogEditor.GotoOuterBlockCmd";
        const char ReloadProjectCmd[] = "VerilogEditor.ReloadProjectCmd";
    }
}

#endif // VLCONSTANTS_H
