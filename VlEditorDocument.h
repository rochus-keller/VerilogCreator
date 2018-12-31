#ifndef VLEDITORDOCUMENT_H
#define VLEDITORDOCUMENT_H

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

#include <texteditor/textdocument.h>

#include <QTimer>

namespace Vl
{
    class EditorDocument1 : public TextEditor::TextDocument
    {
        Q_OBJECT
    public:
        EditorDocument1();
        ~EditorDocument1();

        // overrides
#if VL_QTC_VER >= 0306
        TextDocument::OpenResult open(QString *errorString, const QString &fileName, const QString &realFileName);
#else
        bool open(QString *errorString, const QString &fileName, const QString &realFileName);
#endif
        bool save(QString *errorString, const QString &fileName, bool autoSave);

    signals:
        void sigLoaded();
        void sigStartProcessing();

    protected slots:
        void onChangedContents();
        void onProcess();
    private:
        QTimer d_processorTimer;
        bool d_opening;
    };

    class EditorDocument2 : public TextEditor::TextDocument
    {
        Q_OBJECT
    public:
        EditorDocument2();
    };
}

#endif // VLEDITORDOCUMENT_H
