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

#include "VlTclConfiguration.h"
#include "VlProject.h"
#include "VlTclEngine.h"
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/gccparser.h>
#include <utils/qtcprocess.h>
#include <utils/detailswidget.h>
#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
using namespace Vl;

const char* TclBuildConfig::ID = "VerilogCreator.Tcl.BuildConfig";
const char* TclStep::ID = "VerilogCreator.Tcl.MakeStep";
const char* TclRunConfiguration::ID = "VerilogCreator.Tcl.RunConfig";
const char* TclRunConfiguration::Name = "Tcl Script";
static const char* s_defaultBuildDir = "tcl_build";

TclBuildConfig::TclBuildConfig(ProjectExplorer::Target* parent)
    : ProjectExplorer::BuildConfiguration(parent,ID)
{
}

TclBuildConfig::~TclBuildConfig()
{
}

ProjectExplorer::NamedWidget* TclBuildConfig::createConfigWidget()
{
    return new TclBuildConfigWidget(this);
}

ProjectExplorer::BuildConfiguration::BuildType TclBuildConfig::buildType() const
{
    return BuildConfiguration::Unknown;
}

ProjectExplorer::BuildConfiguration* TclBuildConfig::create(ProjectExplorer::Target* parent, const ProjectExplorer::BuildInfo* info)
{
    TclBuildConfig *bc = new TclBuildConfig(parent);
    bc->setDisplayName(info->displayName);
    bc->setDefaultDisplayName(info->displayName);
    Utils::FileName name = info->buildDirectory;
    name.appendPath(s_defaultBuildDir);
    bc->setBuildDirectory(name);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    Q_ASSERT(buildSteps);
    TclStep *makeStep = new TclStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    Q_ASSERT(cleanSteps);
    TclStep *cleanMakeStep = new TclStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);

    return bc;
}

TclBuildConfig::TclBuildConfig(ProjectExplorer::Target* parent, Core::Id id)
    : ProjectExplorer::BuildConfiguration(parent, id)
{
}

TclBuildConfig::TclBuildConfig(ProjectExplorer::Target* parent, TclBuildConfig* source):
    ProjectExplorer::BuildConfiguration(parent, source)
{
    cloneSteps(source);
}


TclBuildConfigWidget::TclBuildConfigWidget(TclBuildConfig* bc):d_conf(bc)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    d_buildPath = new Utils::PathChooser(this);
    d_buildPath->setHistoryCompleter(QLatin1String("VerilogCreator.Tcl.BuildDir.History"));
    d_buildPath->setEnabled(true);
    d_buildPath->setBaseFileName(bc->target()->project()->projectDirectory());
    d_buildPath->setEnvironment(bc->environment());
    d_buildPath->setPath(bc->rawBuildDirectory().toString());
    fl->addRow(tr("Build directory:"), d_buildPath );
#if VL_QTC_VER >= 0306
    connect(d_buildPath, SIGNAL(rawPathChanged(QString)),this,SLOT(buildDirectoryChanged()));
#else
    connect(d_buildPath, &Utils::PathChooser::changed,
            this, &TclBuildConfigWidget::buildDirectoryChanged);
#endif

    connect(bc, SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()) );

    setDisplayName(tr("General"));
}

void TclBuildConfigWidget::buildDirectoryChanged()
{
    d_conf->setBuildDirectory(Utils::FileName::fromString(d_buildPath->rawPath()));
}

void TclBuildConfigWidget::environmentChanged()
{
    d_buildPath->setEnvironment(d_conf->environment());
}

TclStep::TclStep(ProjectExplorer::BuildStepList* parent):
    AbstractProcessStep(parent, ID)
{
    setDefaultDisplayName(tr("Tcl") );
}

bool TclStep::init()
{
    ProjectExplorer::BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    if (!bc)
        emit addTask(ProjectExplorer::Task::buildConfigurationMissingTask());

    ProjectExplorer::ProcessParameters *pp = processParameters();
    // pp->setMacroExpander( bc->macroExpander() );
    QDir buildDir = bc->buildDirectory().toString();
    pp->setWorkingDirectory( buildDir.path() );
    Utils::Environment env = bc->environment();
    pp->setEnvironment(env);
    pp->setCommand(makeCommand(bc->environment()));

    Vl::Project* p = dynamic_cast<Vl::Project*>( project() );
    Q_ASSERT( p != 0 );

    if( !p->getTcl()->isReady() )
    {
        emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                       tr( "The Tcl interpreter is not ready."),
                       Utils::FileName(), -1,
                       ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM) );
        return false;
    }

    if( d_scriptFile.trimmed().isEmpty() )
    {
        emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Warning,
                       tr( "No Tcl script configured"),
                       Utils::FileName(), -1,
                       ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM) );
        return false;
    }

    QFileInfo info(d_scriptFile);
    if( !info.exists() || !info.isReadable() )
    {
        emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                       tr( "Cannot access Tcl script file %1.").arg( QDir::toNativeSeparators(d_scriptFile)),
                       Utils::FileName(), -1,
                       ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM) );
        return false;
    }

    if( !buildDir.exists() )
    {
        if (!buildDir.mkpath(buildDir.absolutePath()))
        {
            emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                           tr( "Unable to create the directory %1.").arg( QDir::toNativeSeparators(buildDir.absolutePath())),
                           Utils::FileName(), -1,
                           ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM) );
            return false;
        }
    }
    pp->resolveAll();

    setIgnoreReturnValue(false);

    // setOutputParser(new ProjectExplorer::GnuMakeParser());
    if( outputParser() )
        outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init();
}

void TclStep::run(QFutureInterface<bool>& fi)
{
    processStarted();

    Vl::Project* p = dynamic_cast<Vl::Project*>( project() );
    p->getTcl()->setWriteLog(writeLog,this);
    int res = 0;
    if( !p->getTcl()->runFile( d_scriptFile ) )
        res = -1;
    p->getTcl()->setWriteLog(0,0);

    // AbstractProcessStep::run(fi);
    // startet einen Prozess; wir machen das hier selber

    // processFinished( res, QProcess::NormalExit ); // uses m_process
    QString str = p->getTcl()->getResult().trimmed();
    if( !str.isEmpty() )
        str = " : " + str;
    emit addOutput(tr("The script \"%1\" exited with code %2%3").arg(d_scriptFile).arg(res).arg(str),
                   res == 0 ? BuildStep::MessageOutput : BuildStep::ErrorMessageOutput);
    const bool returnValue = processSucceeded( res, QProcess::NormalExit );

    fi.reportResult(returnValue);

    emit finished();
}

ProjectExplorer::BuildStepConfigWidget* TclStep::createConfigWidget()
{
    return new TclMakeStepWidget(this);
}

static const char* MAKE_SCRIPT_KEY = "VerilogCreator.Tcl.MakeStep.Script";

QVariantMap TclStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(MAKE_SCRIPT_KEY), d_scriptFile);
    return map;
}

bool TclStep::fromMap(const QVariantMap& map)
{
    d_scriptFile = map.value(QLatin1String(MAKE_SCRIPT_KEY)).toString();

    return BuildStep::fromMap(map);
}

void TclStep::writeLog(const QByteArray& msg, bool err, void* data)
{
    TclStep* p = static_cast<TclStep*>(data);
    if( err )
        p->stdError(QString::fromUtf8(msg));
    else
        p->stdOutput(QString::fromUtf8(msg));
}

QString TclStep::makeCommand(const Utils::Environment& ) const
{
    return d_scriptFile;
}

TclMakeStepWidget::TclMakeStepWidget(TclStep* makeStep):d_step(makeStep)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    QHBoxLayout* hbox = new QHBoxLayout();
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSizeConstraint(QLayout::SetMinimumSize);

    d_scriptFile = new QLineEdit(this);
    d_scriptFile->setText(makeStep->d_scriptFile);
    hbox->addWidget(d_scriptFile);

    QPushButton* b = new QPushButton(tr("Browse..."), this);
    hbox->addWidget(b);

    fl->addRow( new QLabel(tr("Script file:")), hbox );

    updateDetails();

    connect( d_scriptFile, &QLineEdit::textEdited,
            this, &TclMakeStepWidget::makeLineEditTextEdited);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));

    connect(makeStep->target()->project(), &ProjectExplorer::Project::environmentChanged,
            this, &TclMakeStepWidget::updateDetails);
    connect(makeStep->target()->project(), &ProjectExplorer::Project::fileListChanged,
            this, &TclMakeStepWidget::updateDetails);
    connect( b, SIGNAL(clicked(bool)), this, SLOT(onBrowse()) );
}

QString TclMakeStepWidget::displayName() const
{
    return tr("Tcl");
}

QString TclMakeStepWidget::summaryText() const
{
    return d_summary;
}

void TclMakeStepWidget::makeLineEditTextEdited()
{
    d_step->d_scriptFile = d_scriptFile->text();
    updateDetails();
}

void TclMakeStepWidget::updateDetails()
{
    ProjectExplorer::BuildConfiguration *bc = d_step->buildConfiguration();
    if (!bc)
        bc = d_step->target()->activeBuildConfiguration();

    Vl::Project* p = dynamic_cast<Vl::Project*>( d_step->target()->project() );
    Q_ASSERT( p != 0 );

    d_summary = tr("<b>Tcl</b>: %1").arg(bc->macroExpander()->expand(d_step->d_scriptFile));
    emit updateSummary();
}

void TclMakeStepWidget::onBrowse()
{
    QString path = QDir::fromNativeSeparators(d_step->buildConfiguration()->
                                                    macroExpander()->expand(d_step->d_scriptFile));
    path = QFileDialog::getOpenFileName(this, tr("Select Tcl Script File"), path, "Tcl Script (*.tcl)" );
    if( !path.isEmpty() )
    {
        d_scriptFile->setText(path);
        makeLineEditTextEdited();
    }
}

// TODO: a dedicated run configuration for Tcl doesn't seem to make sense; might as well choose the
// custom executable configuration with tclsh

QWidget*TclRunConfiguration::createConfigurationWidget()
{
    return new TclRunConfigWidget(this);
}

QString TclRunConfiguration::executable() const
{
    static const char* defaultCmd = "tcl";
    if( !d_scriptFile.isEmpty() )
        return d_scriptFile;
    Utils::Environment env;
    ProjectExplorer::EnvironmentAspect *aspect = extraAspect<ProjectExplorer::EnvironmentAspect>();
    if (aspect)
        env = aspect->environment();
    const Utils::FileName exec = env.searchInPath(macroExpander()->expand(QByteArray(defaultCmd)),
                                                  QStringList(workingDirectory()));
    if( exec.isEmpty() )
        return defaultCmd;

    return exec.toString();
}

ProjectExplorer::ApplicationLauncher::Mode TclRunConfiguration::runMode() const
{
    return ( d_terminal ? ProjectExplorer::ApplicationLauncher::Console : ProjectExplorer::ApplicationLauncher::Gui );
    // Gui geht in Application Output
    // Console öffne ein neues Konsolenfenster
}

QString TclRunConfiguration::workingDirectory() const
{
    ProjectExplorer::EnvironmentAspect *aspect = extraAspect<ProjectExplorer::EnvironmentAspect>();
    if( aspect == 0 )
        return ".";
    return QDir::cleanPath(aspect->environment().expandVariables(
                macroExpander()->expand( QLatin1Literal("%{buildDir}") )));
}

QString TclRunConfiguration::commandLineArguments() const
{
    return QString();
}

void TclRunConfiguration::addToBaseEnvironment(Utils::Environment& env) const
{

}

static const char* RUN_COMMAND_KEY = "VerilogCreator.Tcl.RunConfig.Script";
static const char* TERM_COMMAND_KEY = "VerilogCreator.Tcl.RunConfig.Term";

QVariantMap TclRunConfiguration::toMap() const
{
    QVariantMap map(LocalApplicationRunConfiguration::toMap());

    map.insert(QLatin1String(RUN_COMMAND_KEY), d_scriptFile);
    map.insert(QLatin1String(TERM_COMMAND_KEY), d_terminal);

    return map;
}

TclRunConfiguration::TclRunConfiguration(ProjectExplorer::Target* target):
    LocalApplicationRunConfiguration(target, Core::Id(ID)), d_terminal(false)
{
    setDefaultDisplayName(Name);
    addExtraAspect(new ProjectExplorer::LocalEnvironmentAspect(this));
}

TclRunConfiguration::TclRunConfiguration(ProjectExplorer::Target* target, TclRunConfiguration* rc):
    LocalApplicationRunConfiguration(target, rc), d_terminal(false)
{
    setDefaultDisplayName(Name);
    d_scriptFile = rc->d_scriptFile;
    d_terminal = rc->d_terminal;
}

bool TclRunConfiguration::fromMap(const QVariantMap& map)
{
    d_scriptFile = map.value(QLatin1String(RUN_COMMAND_KEY)).toString();
    d_terminal = map.value(QLatin1String(TERM_COMMAND_KEY)).toBool();

    return LocalApplicationRunConfiguration::fromMap(map);
}

TclRunConfigWidget::TclRunConfigWidget(TclRunConfiguration* rc):d_cfg(rc)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);

    Utils::DetailsWidget* m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);

    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);

    QFormLayout *fl = new QFormLayout;
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    d_scriptFile = new QLineEdit(this);
    d_scriptFile->setText(rc->d_scriptFile);
    fl->addRow( tr("Tcl:"), d_scriptFile );

    d_term = new QCheckBox( tr("Run in terminal"), this);
    d_term->setChecked(rc->d_terminal);
    fl->addRow( "", d_term );

    detailsWidget->setLayout(fl);

    connect( d_scriptFile, SIGNAL(textEdited(QString)), this, SLOT(cmdEdited()));
    connect( d_term, SIGNAL(toggled(bool)), this, SLOT(termChanged()) );
}

void TclRunConfigWidget::cmdEdited()
{
    d_cfg->d_scriptFile = d_scriptFile->text();
}

void TclRunConfigWidget::termChanged()
{
    d_cfg->d_terminal = d_term->isChecked();
}
