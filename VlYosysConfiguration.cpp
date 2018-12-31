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

#include "VlYosysConfiguration.h"
#include "VlProject.h"
#include "VlModelManager.h"
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

const char* YosysBuildConfig::ID = "VerilogCreator.Yosys.BuildConfig";
const char* YosysMakeStep::ID = "VerilogCreator.Yosys.MakeStep";
const char* YosysCleanStep::ID = "VerilogCreator.Yosys.CleanStep";
const char* YosysBuildConfig::Name = "Yosys";
static const char* s_cmdFileName = "cmdfile.txt";
static const char* s_simDir = ".";
static const char* s_defaultBuildDir = "yosys_build";

YosysBuildConfig::YosysBuildConfig(ProjectExplorer::Target* parent)
    : ProjectExplorer::BuildConfiguration(parent,ID)
{

}

ProjectExplorer::NamedWidget* YosysBuildConfig::createConfigWidget()
{
    return new YosysBuildConfigWidget(this);
}

ProjectExplorer::BuildConfiguration::BuildType YosysBuildConfig::buildType() const
{
    return BuildConfiguration::Unknown;
}

ProjectExplorer::BuildConfiguration* YosysBuildConfig::create(ProjectExplorer::Target* parent, const ProjectExplorer::BuildInfo* info)
{
    YosysBuildConfig *bc = new YosysBuildConfig(parent);
    bc->setDisplayName(info->displayName);
    bc->setDefaultDisplayName(info->displayName);
    Utils::FileName name = info->buildDirectory;
    name.appendPath(s_defaultBuildDir);
    bc->setBuildDirectory(name);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    Q_ASSERT(buildSteps);
    YosysMakeStep *makeStep = new YosysMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    Q_ASSERT(cleanSteps);
    YosysCleanStep *cleanMakeStep = new YosysCleanStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);

    return bc;
}

YosysBuildConfig::YosysBuildConfig(ProjectExplorer::Target* parent, Core::Id id)
    : ProjectExplorer::BuildConfiguration(parent, id)
{

}

YosysBuildConfig::YosysBuildConfig(ProjectExplorer::Target* parent, YosysBuildConfig* source):
    ProjectExplorer::BuildConfiguration(parent, source)
{
    cloneSteps(source);
}


YosysBuildConfigWidget::YosysBuildConfigWidget(YosysBuildConfig* bc):d_conf(bc)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    d_buildPath = new Utils::PathChooser(this);
    d_buildPath->setHistoryCompleter(QLatin1String("VerilogCreator.Yosys.BuildDir.History"));
    d_buildPath->setEnabled(true);
    d_buildPath->setBaseFileName(bc->target()->project()->projectDirectory());
    d_buildPath->setEnvironment(bc->environment());
    d_buildPath->setPath(bc->rawBuildDirectory().toString());
    fl->addRow(tr("Build directory:"), d_buildPath );
#if VL_QTC_VER >= 0306
    connect(d_buildPath, SIGNAL(rawPathChanged(QString)),this,SLOT(buildDirectoryChanged()));
#else
    connect(d_buildPath, &Utils::PathChooser::changed,
            this, &YosysBuildConfigWidget::buildDirectoryChanged);
#endif

    connect(bc, SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()) );

    setDisplayName(tr("General"));
}

void YosysBuildConfigWidget::buildDirectoryChanged()
{
    d_conf->setBuildDirectory(Utils::FileName::fromString(d_buildPath->rawPath()));
}

void YosysBuildConfigWidget::environmentChanged()
{
    d_buildPath->setEnvironment(d_conf->environment());
}

YosysMakeStep::YosysMakeStep(ProjectExplorer::BuildStepList* parent):
    AbstractProcessStep(parent, ID)
{
    setDefaultDisplayName(tr("yosys") );
    d_args = "-p \"proc; opt; memory; opt; fsm; opt; techmap; opt; write_blif result.blif\"";
}

static inline QString _path( const QString& in )
{
    if( in.contains(QChar(' ')) )
        return QString("\"%1\"").arg(in);
    else
        return in;
}

bool YosysMakeStep::init()
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

    const QSet<QString> undefs = QSet<QString>::fromList(p->getConfig("BUILD_UNDEFS")) +
            QSet<QString>::fromList(p->getConfig("YOSYS_UNDEFS"));
    foreach( const QString& f, p->getConfig("DEFINES") )
    {
        const QString def = f.trimmed();
		const int pos = def.indexOf(QRegExp("\\s"));
		const QString key = ( pos == -1 ? def : def.left(pos) );
		const QString val = ( pos == -1 ? QString() : def.mid(pos+1).trimmed() );
        if( !undefs.contains(key) )
        {
            cmdfile.write("verilog_defaults -add -D");
            cmdfile.write(key.toUtf8());
            if( !val.isEmpty() )
            {
                cmdfile.write("=");
                cmdfile.write(val.toUtf8());
            }
            cmdfile.write("\n");
        }
    }

    foreach( const QString& f, p->getIncDirs() )
    {
        cmdfile.write("verilog_defaults -add -I");
        cmdfile.write( _path(f.trimmed()).toUtf8() );
        cmdfile.write("\n");
    }

    foreach( const QString& f, p->getLibFiles() )
    {
        cmdfile.write( "read_verilog " );
        cmdfile.write( _path( f.trimmed() ).toUtf8() );
        cmdfile.write( "\n" );
    }
    foreach( const QString& f, p->getSrcFiles() )
    {
        cmdfile.write( "read_verilog " );
        cmdfile.write( _path( f.trimmed() ).toUtf8() );
        cmdfile.write( "\n" );
    }

    const QString topmodule = p->getTopMod().trimmed();
    if( !topmodule.isEmpty() )
        cmdfile.write(QString("hierarchy -check -top %1\n").arg(topmodule).toUtf8() );

    else
        cmdfile.write("hierarchy -check\n");

    foreach( const QString& f, p->getConfig("YOSYS_CMDS") )
    {
        cmdfile.write( macroExpander()->expand(f).toUtf8() );
        cmdfile.write( "\n" );
    }

    QString args;
    args += QString("-s %1 ").arg( Utils::QtcProcess::quoteArg( cmdfile.fileName() ) );
    Utils::QtcProcess::addArgs( &args, macroExpander()->expand(d_args) );

    pp->setArguments( args );

    pp->resolveAll();

    setIgnoreReturnValue(false);

    // TODO setOutputParser(new ProjectExplorer::GccParser());
    if( outputParser() )
        outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init();
}

void YosysMakeStep::run(QFutureInterface<bool>& fi)
{
    AbstractProcessStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget* YosysMakeStep::createConfigWidget()
{
    return new YosysMakeStepWidget(this);
}

static const char* MAKE_ARGUMENTS_KEY = "VerilogCreator.Yosys.MakeStep.Args";
static const char* MAKE_COMMAND_KEY = "VerilogCreator.Yosys.MakeStep.Cmd";

QVariantMap YosysMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), d_args);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), d_cmd);
    return map;
}

bool YosysMakeStep::fromMap(const QVariantMap& map)
{
    d_args = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
    d_cmd = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();

    return BuildStep::fromMap(map);
}

QString YosysMakeStep::makeCommand(const Utils::Environment& environment) const
{
    QString command = d_cmd;
    if (command.isEmpty())
        command = environment.searchInPath("yosys").toString();
    return command;
}

YosysMakeStepWidget::YosysMakeStepWidget(YosysMakeStep* makeStep):d_step(makeStep)
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
            this, &YosysMakeStepWidget::makeLineEditTextEdited);
    connect( d_args, &QLineEdit::textEdited,
            this, &YosysMakeStepWidget::makeArgumentsLineEditTextEdited);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrrideLabel()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));

    connect(makeStep->target(), SIGNAL(kitChanged()),
            this, SLOT(updateMakeOverrrideLabel()));

    connect(makeStep->target()->project(), &ProjectExplorer::Project::environmentChanged,
            this, &YosysMakeStepWidget::updateMakeOverrrideLabel);
    connect(makeStep->target()->project(), &ProjectExplorer::Project::environmentChanged,
            this, &YosysMakeStepWidget::updateDetails);
    connect(makeStep->target()->project(), &ProjectExplorer::Project::fileListChanged,
            this, &YosysMakeStepWidget::updateDetails);
}

QString YosysMakeStepWidget::displayName() const
{
    return tr("Yosys");
}

QString YosysMakeStepWidget::summaryText() const
{
    return d_summary;
}

void YosysMakeStepWidget::makeLineEditTextEdited()
{
    d_step->d_cmd = d_cmd->text();
    updateDetails();
}

void YosysMakeStepWidget::makeArgumentsLineEditTextEdited()
{
    d_step->d_args = d_args->text();
    updateDetails();
}

void YosysMakeStepWidget::updateMakeOverrrideLabel()
{
    ProjectExplorer::BuildConfiguration *bc = d_step->buildConfiguration();
    if (!bc)
        bc = d_step->target()->activeBuildConfiguration();

    d_cmdLabel->setText(tr("Override %1:").arg(QDir::toNativeSeparators(d_step->makeCommand(bc->environment()))));
}

void YosysMakeStepWidget::updateDetails()
{
    ProjectExplorer::BuildConfiguration *bc = d_step->buildConfiguration();
    if (!bc)
        bc = d_step->target()->activeBuildConfiguration();

    Vl::Project* p = dynamic_cast<Vl::Project*>( d_step->target()->project() );
    Q_ASSERT( p != 0 );

    ProjectExplorer::ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(d_step->makeCommand(bc->environment()));
    param.setArguments( QString("-s %1 %2").arg(s_cmdFileName).arg(d_step->d_args));

    d_summary = param.summary(displayName());
    emit updateSummary();
}


YosysCleanStep::YosysCleanStep(ProjectExplorer::BuildStepList* parent):
    AbstractProcessStep(parent, ID)
{

}

bool YosysCleanStep::init()
{
    return true;
}

void YosysCleanStep::run(QFutureInterface<bool>& fi)
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
        //QFile::remove( buildDir.absoluteFilePath(s_simDir) );
    }
    AbstractProcessStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget* YosysCleanStep::createConfigWidget()
{
    return new YosysCleanStepWidget(this);
}


YosysCleanStepWidget::YosysCleanStepWidget(YosysCleanStep* makeStep)
{

}

QString YosysCleanStepWidget::displayName() const
{
    return "clean";
}

QString YosysCleanStepWidget::summaryText() const
{
    return tr("<b>remove</b> %1").arg(s_cmdFileName);
}


