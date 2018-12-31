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

#include "VlIcarusConfiguration.h"
#include "VlProject.h"
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
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
using namespace Vl;

const char* IcarusBuildConfig::ID = "VerilogCreator.Icarus.BuildConfig";
const char* IcarusMakeStep::ID = "VerilogCreator.Icarus.MakeStep";
const char* IcarusCleanStep::ID = "VerilogCreator.Icarus.CleanStep";
const char* IcarusRunConfiguration::ID = "VerilogCreator.Icarus.RunConfig";
const char* IcarusRunConfiguration::Name = "Icarus Verilog";
static const char* s_cmdFileName = "cmdfile.txt";
static const char* s_compiledFileName = "compiled.vvp";
static const char* s_defaultBuildDir = "icarus_build";

IcarusBuildConfig::IcarusBuildConfig(ProjectExplorer::Target* parent)
    : ProjectExplorer::BuildConfiguration(parent,ID)
{

}

ProjectExplorer::NamedWidget* IcarusBuildConfig::createConfigWidget()
{
    return new IcarusBuildConfigWidget(this);
}

ProjectExplorer::BuildConfiguration::BuildType IcarusBuildConfig::buildType() const
{
    return BuildConfiguration::Unknown;
}

ProjectExplorer::BuildConfiguration* IcarusBuildConfig::create(ProjectExplorer::Target* parent, const ProjectExplorer::BuildInfo* info)
{
    IcarusBuildConfig *bc = new IcarusBuildConfig(parent);
    bc->setDisplayName(info->displayName);
    bc->setDefaultDisplayName(info->displayName);
    Utils::FileName name = info->buildDirectory;
    name.appendPath(s_defaultBuildDir);
    bc->setBuildDirectory(name);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    Q_ASSERT(buildSteps);
    IcarusMakeStep *makeStep = new IcarusMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    Q_ASSERT(cleanSteps);
    IcarusCleanStep *cleanMakeStep = new IcarusCleanStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);

    return bc;
}

IcarusBuildConfig::IcarusBuildConfig(ProjectExplorer::Target* parent, Core::Id id)
    : ProjectExplorer::BuildConfiguration(parent, id)
{

}

IcarusBuildConfig::IcarusBuildConfig(ProjectExplorer::Target* parent, IcarusBuildConfig* source):
    ProjectExplorer::BuildConfiguration(parent, source)
{
    cloneSteps(source);
}


IcarusBuildConfigWidget::IcarusBuildConfigWidget(IcarusBuildConfig* bc):d_conf(bc)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    d_buildPath = new Utils::PathChooser(this);
    d_buildPath->setHistoryCompleter(QLatin1String("VerilogCreator.Icarus.BuildDir.History"));
    d_buildPath->setEnabled(true);
    d_buildPath->setBaseFileName(bc->target()->project()->projectDirectory());
    d_buildPath->setEnvironment(bc->environment());
    d_buildPath->setPath(bc->rawBuildDirectory().toString());
    fl->addRow(tr("Build directory:"), d_buildPath );
#if VL_QTC_VER >= 0306
    connect(d_buildPath, SIGNAL(rawPathChanged(QString)),this,SLOT(buildDirectoryChanged()));
#else
    connect(d_buildPath, &Utils::PathChooser::changed,
            this, &IcarusBuildConfigWidget::buildDirectoryChanged);
#endif

    connect(bc, SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()) );

    setDisplayName(tr("General"));
}

void IcarusBuildConfigWidget::buildDirectoryChanged()
{
    d_conf->setBuildDirectory(Utils::FileName::fromString(d_buildPath->rawPath()));
}

void IcarusBuildConfigWidget::environmentChanged()
{
    d_buildPath->setEnvironment(d_conf->environment());
}

IcarusMakeStep::IcarusMakeStep(ProjectExplorer::BuildStepList* parent):
    AbstractProcessStep(parent, ID)
{
    setDefaultDisplayName(tr("iverilog") );
}

bool IcarusMakeStep::init()
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
    QFile cmdfile( buildDir.absoluteFilePath(s_cmdFileName) );
    if( !cmdfile.open(QIODevice::WriteOnly) )
    {
        emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                       tr("Unable to create the file %1.").arg( QDir::toNativeSeparators(cmdfile.fileName())),
                       Utils::FileName(), -1,
                       ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM) );
        return false;
    }

    cmdfile.write("# this file is automatically generated; do not edit\n");
    foreach( const QString& f, p->getSrcFiles() )
    {
        cmdfile.write( f.toUtf8() );
        cmdfile.write( "\n" );
    }
    foreach( const QString& f, p->getLibFiles() )
    {
        cmdfile.write( "-l " );
        cmdfile.write( f.toUtf8() );
        cmdfile.write( "\n" );
    }
    QSet<QString> undefs = QSet<QString>::fromList(p->getConfig("BUILD_UNDEFS"));
    foreach( const QString& f, p->getConfig("DEFINES") )
    {
        const QString def = f.trimmed();
        const int pos = def.indexOf(QRegExp("\\s"));
        const QString key = ( pos == -1 ? def : def.left(pos) );
        const QString val = ( pos == -1 ? QString() : def.mid(pos+1).trimmed() );
        if( !undefs.contains(key) )
        {
            cmdfile.write( "+define+" );
            cmdfile.write(key.toUtf8());
            if( !val.isEmpty() )
            {
                cmdfile.write( "=" );
                cmdfile.write(val.toUtf8());
            }
            cmdfile.write( "\n" );
        }
    }
    foreach( const QString& f, p->getIncDirs() )
    {
        cmdfile.write( "+incdir+" );
        cmdfile.write( f.trimmed().toUtf8() );
        cmdfile.write( "\n" );
    }

    QString args;
    const QString topmodule = p->getTopMod().trimmed();
    if( !topmodule.isEmpty() )
        args += QString("-s %1 ").arg( topmodule );
    args += QString("-c %1 ").arg( Utils::QtcProcess::quoteArg( cmdfile.fileName() ) );
    args += QString("-o %1 ").arg( Utils::QtcProcess::quoteArg( buildDir.absoluteFilePath( s_compiledFileName ) ) );
    Utils::QtcProcess::addArgs( &args, d_args );

    pp->setArguments( args );

    pp->resolveAll();

    setIgnoreReturnValue(false);

    // setOutputParser(new ProjectExplorer::GnuMakeParser());
    if( outputParser() )
        outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init();
}

void IcarusMakeStep::run(QFutureInterface<bool>& fi)
{
    AbstractProcessStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget* IcarusMakeStep::createConfigWidget()
{
    return new IcarusMakeStepWidget(this);
}

static const char* MAKE_ARGUMENTS_KEY = "VerilogCreator.Icarus.MakeStep.Args";
static const char* MAKE_COMMAND_KEY = "VerilogCreator.Icarus.MakeStep.Cmd";

QVariantMap IcarusMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), d_args);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), d_cmd);
    return map;
}

bool IcarusMakeStep::fromMap(const QVariantMap& map)
{
    d_args = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
    d_cmd = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();

    return BuildStep::fromMap(map);
}

QString IcarusMakeStep::makeCommand(const Utils::Environment& environment) const
{
    QString command = d_cmd;
    if (command.isEmpty())
        command = environment.searchInPath("iverilog").toString();
    return command;
}

IcarusMakeStepWidget::IcarusMakeStepWidget(IcarusMakeStep* makeStep):d_step(makeStep)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    d_cmd = new QLineEdit(this);
    d_cmd->setText(makeStep->d_cmd);
    d_cmdLabel = new QLabel(this);
    fl->addRow( d_cmdLabel, d_cmd );

    d_args = new QLineEdit(this);
    d_args->setText(makeStep->d_args);
    fl->addRow( tr("Additional arguments:"), d_args );

    updateMakeOverrrideLabel();
    updateDetails();

    connect( d_cmd, &QLineEdit::textEdited,
            this, &IcarusMakeStepWidget::makeLineEditTextEdited);
    connect( d_args, &QLineEdit::textEdited,
            this, &IcarusMakeStepWidget::makeArgumentsLineEditTextEdited);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrrideLabel()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));

    connect(makeStep->target(), SIGNAL(kitChanged()),
            this, SLOT(updateMakeOverrrideLabel()));

    connect(makeStep->target()->project(), &ProjectExplorer::Project::environmentChanged,
            this, &IcarusMakeStepWidget::updateMakeOverrrideLabel);
    connect(makeStep->target()->project(), &ProjectExplorer::Project::environmentChanged,
            this, &IcarusMakeStepWidget::updateDetails);
    connect(makeStep->target()->project(), &ProjectExplorer::Project::fileListChanged,
            this, &IcarusMakeStepWidget::updateDetails);
}

QString IcarusMakeStepWidget::displayName() const
{
    return tr("iverilog");
}

QString IcarusMakeStepWidget::summaryText() const
{
    return d_summary;
}

void IcarusMakeStepWidget::makeLineEditTextEdited()
{
    d_step->d_cmd = d_cmd->text();
    updateDetails();
}

void IcarusMakeStepWidget::makeArgumentsLineEditTextEdited()
{
    d_step->d_args = d_args->text();
    updateDetails();
}

void IcarusMakeStepWidget::updateMakeOverrrideLabel()
{
    ProjectExplorer::BuildConfiguration *bc = d_step->buildConfiguration();
    if (!bc)
        bc = d_step->target()->activeBuildConfiguration();

    d_cmdLabel->setText(tr("Override %1:").arg(QDir::toNativeSeparators(d_step->makeCommand(bc->environment()))));
}

void IcarusMakeStepWidget::updateDetails()
{
    ProjectExplorer::BuildConfiguration *bc = d_step->buildConfiguration();
    if (!bc)
        bc = d_step->target()->activeBuildConfiguration();

    Vl::Project* p = dynamic_cast<Vl::Project*>( d_step->target()->project() );
    Q_ASSERT( p != 0 );

    QString topmodule = p->getTopMod().trimmed();
    if( !topmodule.isEmpty() )
        topmodule = QString("-s %1 ").arg( topmodule );

    ProjectExplorer::ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(d_step->makeCommand(bc->environment()));
    param.setArguments( QString("-c %1 -o %2 %4%3").arg(s_cmdFileName)
                        .arg(s_compiledFileName).arg(d_step->d_args).arg(topmodule));
    d_summary = param.summary(displayName());
    emit updateSummary();
}


IcarusCleanStep::IcarusCleanStep(ProjectExplorer::BuildStepList* parent):
    AbstractProcessStep(parent, ID)
{

}

bool IcarusCleanStep::init()
{
    return true;
}

void IcarusCleanStep::run(QFutureInterface<bool>& fi)
{
    ProjectExplorer::BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    if (!bc)
        emit addTask(ProjectExplorer::Task::buildConfigurationMissingTask());

    QDir buildDir = bc->buildDirectory().toString();

    if( buildDir.exists() )
    {
        QFile::remove( buildDir.absoluteFilePath(s_cmdFileName) );
        QFile::remove( buildDir.absoluteFilePath(s_compiledFileName) );
    }
    AbstractProcessStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget* IcarusCleanStep::createConfigWidget()
{
    return new IcarusCleanStepWidget(this);
}


IcarusCleanStepWidget::IcarusCleanStepWidget(IcarusCleanStep* makeStep)
{

}

QString IcarusCleanStepWidget::displayName() const
{
    return "clean";
}

QString IcarusCleanStepWidget::summaryText() const
{
    return tr("<b>remove</b> %1 and %2").arg(s_cmdFileName).arg(s_compiledFileName);
}


QWidget*IcarusRunConfiguration::createConfigurationWidget()
{
    return new IcarusRunConfigWidget(this);
}

QString IcarusRunConfiguration::executable() const
{
    static const char* defaultCmd = "vvp";
    if( !d_cmd.isEmpty() )
        return d_cmd;
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

ProjectExplorer::ApplicationLauncher::Mode IcarusRunConfiguration::runMode() const
{
    return ( d_terminal ? ProjectExplorer::ApplicationLauncher::Console : ProjectExplorer::ApplicationLauncher::Gui );
    // Gui geht in Application Output
    // Console öffne ein neues Konsolenfenster
}

QString IcarusRunConfiguration::workingDirectory() const
{
    ProjectExplorer::EnvironmentAspect *aspect = extraAspect<ProjectExplorer::EnvironmentAspect>();
    if( aspect == 0 )
        return ".";
    return QDir::cleanPath(aspect->environment().expandVariables(
                macroExpander()->expand( QLatin1Literal("%{buildDir}") )));
}

QString IcarusRunConfiguration::commandLineArguments() const
{
    return s_compiledFileName;
}

void IcarusRunConfiguration::addToBaseEnvironment(Utils::Environment& env) const
{

}

static const char* RUN_COMMAND_KEY = "VerilogCreator.Icarus.RunConfig.Cmd";
static const char* TERM_COMMAND_KEY = "VerilogCreator.Icarus.RunConfig.Term";

QVariantMap IcarusRunConfiguration::toMap() const
{
    QVariantMap map(LocalApplicationRunConfiguration::toMap());

    map.insert(QLatin1String(RUN_COMMAND_KEY), d_cmd);
    map.insert(QLatin1String(TERM_COMMAND_KEY), d_terminal);

    return map;
}

IcarusRunConfiguration::IcarusRunConfiguration(ProjectExplorer::Target* target):
    LocalApplicationRunConfiguration(target, Core::Id(ID)), d_terminal(false)
{
    setDefaultDisplayName(Name);
    addExtraAspect(new ProjectExplorer::LocalEnvironmentAspect(this));
}

IcarusRunConfiguration::IcarusRunConfiguration(ProjectExplorer::Target* target, IcarusRunConfiguration* rc):
    LocalApplicationRunConfiguration(target, rc), d_terminal(false)
{
    setDefaultDisplayName(Name);
    d_cmd = rc->d_cmd;
    d_terminal = rc->d_terminal;
}

bool IcarusRunConfiguration::fromMap(const QVariantMap& map)
{
    d_cmd = map.value(QLatin1String(RUN_COMMAND_KEY)).toString();
    d_terminal = map.value(QLatin1String(TERM_COMMAND_KEY)).toBool();

    return LocalApplicationRunConfiguration::fromMap(map);
}

IcarusRunConfigWidget::IcarusRunConfigWidget(IcarusRunConfiguration* rc):d_cfg(rc)
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

    d_cmd = new QLineEdit(this);
    d_cmd->setText(rc->d_cmd);
    fl->addRow( tr("Override vvp command:"), d_cmd );

    d_term = new QCheckBox( tr("Run in terminal"), this);
    d_term->setChecked(rc->d_terminal);
    fl->addRow( "", d_term );

    detailsWidget->setLayout(fl);

    connect( d_cmd, SIGNAL(textEdited(QString)), this, SLOT(cmdEdited()));
    connect( d_term, SIGNAL(toggled(bool)), this, SLOT(termChanged()) );
}

void IcarusRunConfigWidget::cmdEdited()
{
    d_cfg->d_cmd = d_cmd->text();
}

void IcarusRunConfigWidget::termChanged()
{
    d_cfg->d_terminal = d_term->isChecked();
}
