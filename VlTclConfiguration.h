#ifndef VLTCLCONFIGURATION_H
#define VLTCLCONFIGURATION_H

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
    class TclBuildConfig : public ProjectExplorer::BuildConfiguration
    {
        Q_OBJECT
    public:
        static const char* ID;

        explicit TclBuildConfig(ProjectExplorer::Target *parent);
        ~TclBuildConfig();

        ProjectExplorer::NamedWidget* createConfigWidget();
        BuildType buildType() const;

        static ProjectExplorer::BuildConfiguration* create( ProjectExplorer::Target* parent,
                       const ProjectExplorer::BuildInfo* info );

    protected:
        TclBuildConfig(ProjectExplorer::Target *parent, Core::Id id);
        TclBuildConfig(ProjectExplorer::Target *parent, TclBuildConfig *source);
        friend class BuildConfigurationFactory;
        friend class TclBuildConfigWidget;
    };

    class TclBuildConfigWidget : public ProjectExplorer::NamedWidget
    {
        Q_OBJECT

    public:
        TclBuildConfigWidget(TclBuildConfig *bc);
    protected slots:
        void buildDirectoryChanged();
        void environmentChanged();
    private:
        Utils::PathChooser* d_buildPath;
        TclBuildConfig* d_conf;
    };

    class TclStep : public ProjectExplorer::AbstractProcessStep
    {
        Q_OBJECT
    public:
        static const char* ID;

        explicit TclStep(ProjectExplorer::BuildStepList *parent);

        bool init();
        void run(QFutureInterface<bool> &fi);
        bool immutable() const { return false; }
        ProjectExplorer::BuildStepConfigWidget* createConfigWidget();
        QVariantMap toMap() const;
    protected:
        QString makeCommand(const Utils::Environment &environment) const;
        bool fromMap(const QVariantMap &map);
        static void writeLog(const QByteArray& msg, bool err, void* data);
    private:
        QString d_scriptFile;
        friend class TclMakeStepWidget;
    };

    class TclMakeStepWidget : public ProjectExplorer::BuildStepConfigWidget
    {
        Q_OBJECT

    public:
        TclMakeStepWidget(TclStep *makeStep);
        QString displayName() const;
        QString summaryText() const;

    private slots:
        void makeLineEditTextEdited();
        void updateDetails();
        void onBrowse();

    private:
        TclStep* d_step;
        QString d_summary;
        QLineEdit* d_scriptFile;
    };

    class TclRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
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
        explicit TclRunConfiguration(ProjectExplorer::Target *target);
        explicit TclRunConfiguration(ProjectExplorer::Target *target, TclRunConfiguration *rc);
        virtual bool fromMap(const QVariantMap &map);
        friend class RunConfigurationFactory;
        friend class TclRunConfigWidget;
    private:
        QString d_scriptFile;
        bool d_terminal;
    };

    class TclRunConfigWidget : public QWidget
    {
        Q_OBJECT
    public:
        TclRunConfigWidget( TclRunConfiguration* );
    private slots:
        void cmdEdited();
        void termChanged();
    private:
        TclRunConfiguration* d_cfg;
        QLineEdit* d_scriptFile;
        QCheckBox* d_term;
    };
}

#endif // VLTCLCONFIGURATION_H
