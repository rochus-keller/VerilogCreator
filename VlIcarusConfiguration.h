#ifndef VLICARUSCONFIGURATION_H
#define VLICARUSCONFIGURATION_H

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
    class IcarusBuildConfig : public ProjectExplorer::BuildConfiguration
    {
        Q_OBJECT
    public:
        static const char* ID;

        explicit IcarusBuildConfig(ProjectExplorer::Target *parent);

        ProjectExplorer::NamedWidget* createConfigWidget();
        BuildType buildType() const;

        static ProjectExplorer::BuildConfiguration* create( ProjectExplorer::Target* parent,
                       const ProjectExplorer::BuildInfo* info );

    protected:
        IcarusBuildConfig(ProjectExplorer::Target *parent, Core::Id id);
        IcarusBuildConfig(ProjectExplorer::Target *parent, IcarusBuildConfig *source);
        friend class BuildConfigurationFactory;
        friend class IcarusBuildConfigWidget;
    };

    class IcarusBuildConfigWidget : public ProjectExplorer::NamedWidget
    {
        Q_OBJECT

    public:
        IcarusBuildConfigWidget(IcarusBuildConfig *bc);
    protected slots:
        void buildDirectoryChanged();
        void environmentChanged();
    private:
        Utils::PathChooser* d_buildPath;
        IcarusBuildConfig* d_conf;
    };

    class IcarusMakeStep : public ProjectExplorer::AbstractProcessStep
    {
        Q_OBJECT
    public:
        static const char* ID;

        explicit IcarusMakeStep(ProjectExplorer::BuildStepList *parent);

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
        friend class IcarusMakeStepWidget;
    };

    class IcarusMakeStepWidget : public ProjectExplorer::BuildStepConfigWidget
    {
        Q_OBJECT

    public:
        IcarusMakeStepWidget(IcarusMakeStep *makeStep);
        QString displayName() const;
        QString summaryText() const;

    private slots:
        void makeLineEditTextEdited();
        void makeArgumentsLineEditTextEdited();
        void updateMakeOverrrideLabel();
        void updateDetails();

    private:
        IcarusMakeStep* d_step;
        QString d_summary;
        QLabel* d_cmdLabel;
        QLineEdit* d_cmd;
        QLineEdit* d_args;
    };

    class IcarusCleanStep : public ProjectExplorer::AbstractProcessStep
    {
        Q_OBJECT
    public:
        static const char* ID;

        explicit IcarusCleanStep(ProjectExplorer::BuildStepList *parent);

        bool init();
        void run(QFutureInterface<bool> &fi);
        bool immutable() const { return true; }
        ProjectExplorer::BuildStepConfigWidget* createConfigWidget();
    };

    class IcarusCleanStepWidget : public ProjectExplorer::BuildStepConfigWidget
    {
        Q_OBJECT

    public:
        IcarusCleanStepWidget(IcarusCleanStep *makeStep);
        QString displayName() const;
        QString summaryText() const;
    };

    class IcarusRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
    {
        Q_OBJECT
    public:
        static const char* ID;
        static const char* Name;

        virtual QWidget* createConfigurationWidget();
        virtual QString executable() const;
        virtual ProjectExplorer::ApplicationLauncher::Mode runMode() const;
        virtual QString workingDirectory() const;
        virtual QString commandLineArguments() const;
        virtual void addToBaseEnvironment(Utils::Environment &env) const;
        QVariantMap toMap() const;

    protected:
        explicit IcarusRunConfiguration(ProjectExplorer::Target *target);
        explicit IcarusRunConfiguration(ProjectExplorer::Target *target, IcarusRunConfiguration *rc);
        virtual bool fromMap(const QVariantMap &map);
        friend class RunConfigurationFactory;
        friend class IcarusRunConfigWidget;
    private:
        QString d_cmd;
        bool d_terminal;
    };

    class IcarusRunConfigWidget : public QWidget
    {
        Q_OBJECT
    public:
        IcarusRunConfigWidget( IcarusRunConfiguration* );
    private slots:
        void cmdEdited();
        void termChanged();
    private:
        IcarusRunConfiguration* d_cfg;
        QLineEdit* d_cmd;
        QCheckBox* d_term;
    };
}

#endif // VLICARUSCONFIGURATION_H
