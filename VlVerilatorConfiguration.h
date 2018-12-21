#ifndef VVERILATORCONFIGURATION_H
#define VVERILATORCONFIGURATION_H

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

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <utils/pathchooser.h>

class QLabel;
class QCheckBox;

namespace Vl
{
    class VerilatorBuildConfig : public ProjectExplorer::BuildConfiguration
    {
        Q_OBJECT
    public:
        static const char* ID;
        static const char* Name;

        explicit VerilatorBuildConfig(ProjectExplorer::Target *parent);

        ProjectExplorer::NamedWidget* createConfigWidget();
        BuildType buildType() const;

        static ProjectExplorer::BuildConfiguration* create( ProjectExplorer::Target* parent,
                       const ProjectExplorer::BuildInfo* info );

    protected:
        VerilatorBuildConfig(ProjectExplorer::Target *parent, Core::Id id);
        VerilatorBuildConfig(ProjectExplorer::Target *parent, VerilatorBuildConfig *source);
        friend class BuildConfigurationFactory;
        friend class VerilatorBuildConfigWidget;
    };

    class VerilatorBuildConfigWidget : public ProjectExplorer::NamedWidget
    {
        Q_OBJECT

    public:
        VerilatorBuildConfigWidget(VerilatorBuildConfig *bc);
    protected slots:
        void buildDirectoryChanged();
        void environmentChanged();
    private:
        Utils::PathChooser* d_buildPath;
        VerilatorBuildConfig* d_conf;
    };

    class VerilatorMakeStep : public ProjectExplorer::AbstractProcessStep
    {
        Q_OBJECT
    public:
        static const char* ID;

        explicit VerilatorMakeStep(ProjectExplorer::BuildStepList *parent);

        bool init();
        void run(QFutureInterface<bool> &fi);
        bool immutable() const { return false; }
        ProjectExplorer::BuildStepConfigWidget* createConfigWidget();
        QVariantMap toMap() const;
    protected:
        QString makeCommand(const Utils::Environment &environment) const;
        bool fromMap(const QVariantMap &map);
    private:
        QString d_cmd;
        QString d_args;
        friend class VerilatorMakeStepWidget;
    };

    class VerilatorMakeStepWidget : public ProjectExplorer::BuildStepConfigWidget
    {
        Q_OBJECT

    public:
        VerilatorMakeStepWidget(VerilatorMakeStep *makeStep);
        QString displayName() const;
        QString summaryText() const;

    private slots:
        void makeLineEditTextEdited();
        void makeArgumentsLineEditTextEdited();
        void updateMakeOverrrideLabel();
        void updateDetails();

    private:
        VerilatorMakeStep* d_step;
        QString d_summary;
        QLabel* d_cmdLabel;
        QLineEdit* d_cmd;
        QLineEdit* d_args;
    };

    class VerilatorCleanStep : public ProjectExplorer::AbstractProcessStep
    {
        Q_OBJECT
    public:
        static const char* ID;

        explicit VerilatorCleanStep(ProjectExplorer::BuildStepList *parent);

        bool init();
        void run(QFutureInterface<bool> &fi);
        bool immutable() const { return true; }
        ProjectExplorer::BuildStepConfigWidget* createConfigWidget();
    };

    class VerilatorCleanStepWidget : public ProjectExplorer::BuildStepConfigWidget
    {
        Q_OBJECT

    public:
        VerilatorCleanStepWidget(VerilatorCleanStep *makeStep);
        QString displayName() const;
        QString summaryText() const;
    };
}

#endif // VVERILATORCONFIGURATION_H
