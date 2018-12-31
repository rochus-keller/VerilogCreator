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

#include "VlVerilatorConfiguration.h"
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

const char* VerilatorBuildConfig::ID = "VerilogCreator.Verilator.BuildConfig";
const char* VerilatorMakeStep::ID = "VerilogCreator.Verilator.MakeStep";
const char* VerilatorCleanStep::ID = "VerilogCreator.Verilator.CleanStep";
const char* VerilatorBuildConfig::Name = "Verilator";
static const char* s_cmdFileName = "cmdfile.txt";
static const char* s_simDir = ".";
static const char* s_defaultBuildDir = "verilator_build";

VerilatorBuildConfig::VerilatorBuildConfig(ProjectExplorer::Target* parent)
    : ProjectExplorer::BuildConfiguration(parent,ID)
{

}

ProjectExplorer::NamedWidget* VerilatorBuildConfig::createConfigWidget()
{
    return new VerilatorBuildConfigWidget(this);
}

ProjectExplorer::BuildConfiguration::BuildType VerilatorBuildConfig::buildType() const
{
    return BuildConfiguration::Unknown;
}

ProjectExplorer::BuildConfiguration* VerilatorBuildConfig::create(ProjectExplorer::Target* parent, const ProjectExplorer::BuildInfo* info)
{
    VerilatorBuildConfig *bc = new VerilatorBuildConfig(parent);
    bc->setDisplayName(info->displayName);
    bc->setDefaultDisplayName(info->displayName);
    Utils::FileName name = info->buildDirectory;
    name.appendPath(s_defaultBuildDir);
    bc->setBuildDirectory(name);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    Q_ASSERT(buildSteps);
    VerilatorMakeStep *makeStep = new VerilatorMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    Q_ASSERT(cleanSteps);
    VerilatorCleanStep *cleanMakeStep = new VerilatorCleanStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);

    return bc;
}

VerilatorBuildConfig::VerilatorBuildConfig(ProjectExplorer::Target* parent, Core::Id id)
    : ProjectExplorer::BuildConfiguration(parent, id)
{

}

VerilatorBuildConfig::VerilatorBuildConfig(ProjectExplorer::Target* parent, VerilatorBuildConfig* source):
    ProjectExplorer::BuildConfiguration(parent, source)
{
    cloneSteps(source);
}


VerilatorBuildConfigWidget::VerilatorBuildConfigWidget(VerilatorBuildConfig* bc):d_conf(bc)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    d_buildPath = new Utils::PathChooser(this);
    d_buildPath->setHistoryCompleter(QLatin1String("VerilogCreator.Verilator.BuildDir.History"));
    d_buildPath->setEnabled(true);
    d_buildPath->setBaseFileName(bc->target()->project()->projectDirectory());
    d_buildPath->setEnvironment(bc->environment());
    d_buildPath->setPath(bc->rawBuildDirectory().toString());
    fl->addRow(tr("Build directory:"), d_buildPath );
#if VL_QTC_VER >= 0306
    connect(d_buildPath, SIGNAL(rawPathChanged(QString)),this,SLOT(buildDirectoryChanged()));
#else
    connect(d_buildPath, &Utils::PathChooser::changed,
            this, &VerilatorBuildConfigWidget::buildDirectoryChanged);
#endif

    connect(bc, SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()) );

    setDisplayName(tr("General"));
}

void VerilatorBuildConfigWidget::buildDirectoryChanged()
{
    d_conf->setBuildDirectory(Utils::FileName::fromString(d_buildPath->rawPath()));
}

void VerilatorBuildConfigWidget::environmentChanged()
{
    d_buildPath->setEnvironment(d_conf->environment());
}

VerilatorMakeStep::VerilatorMakeStep(ProjectExplorer::BuildStepList* parent):
    AbstractProcessStep(parent, ID)
{
    setDefaultDisplayName(tr("verilator") );
    d_args = QLatin1Literal("--cc");
}

bool VerilatorMakeStep::init()
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

    QSet<QString> dirs;
    foreach( const QString& f, p->getSrcFiles() )
    {
        dirs.insert( QFileInfo(f).path() );
    }
    foreach( const QString& f, p->getLibFiles() )
    {
        dirs.insert( QFileInfo(f).path() );
    }
    foreach( const QString& f, dirs )
    {
        cmdfile.write( "-y " );
        cmdfile.write( f.toUtf8() );
        cmdfile.write( "\n" );
    }
    const QSet<QString> undefs = QSet<QString>::fromList(p->getConfig("BUILD_UNDEFS")) +
            QSet<QString>::fromList(p->getConfig("VLTR_UNDEFS"));
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
    const QString topmodule = p->getTopMod().trimmed();
    if( !topmodule.isEmpty() )
    {
        CrossRefModel::SymRef res = ModelManager::instance()->getModelForCurrentProject()->findGlobal( topmodule.toLatin1() );
        if( res.data() )
        {
            cmdfile.write( res->tok().d_sourcePath.toUtf8() );
            cmdfile.write( "\n" );
        }
    }


    QString args = p->getConfig("VLTR_ARGS").join(QChar(' '));
    args += QString(" --Mdir %1 ").arg( s_simDir );
    args += QString("-f %1 ").arg( Utils::QtcProcess::quoteArg( cmdfile.fileName() ) );
    Utils::QtcProcess::addArgs( &args, d_args );

    pp->setArguments( args );

    pp->resolveAll();

    setIgnoreReturnValue(false);

    // TODO setOutputParser(new ProjectExplorer::GccParser());
    if( outputParser() )
        outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init();
}

void VerilatorMakeStep::run(QFutureInterface<bool>& fi)
{
    AbstractProcessStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget* VerilatorMakeStep::createConfigWidget()
{
    return new VerilatorMakeStepWidget(this);
}

static const char* MAKE_ARGUMENTS_KEY = "VerilogCreator.Verilator.MakeStep.Args";
static const char* MAKE_COMMAND_KEY = "VerilogCreator.Verilator.MakeStep.Cmd";

QVariantMap VerilatorMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), d_args);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), d_cmd);
    return map;
}

bool VerilatorMakeStep::fromMap(const QVariantMap& map)
{
    d_args = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
    d_cmd = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();

    return BuildStep::fromMap(map);
}

QString VerilatorMakeStep::makeCommand(const Utils::Environment& environment) const
{
    QString command = d_cmd;
    if (command.isEmpty())
        command = environment.searchInPath("verilator_bin").toString();
    return command;
}

VerilatorMakeStepWidget::VerilatorMakeStepWidget(VerilatorMakeStep* makeStep):d_step(makeStep)
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
            this, &VerilatorMakeStepWidget::makeLineEditTextEdited);
    connect( d_args, &QLineEdit::textEdited,
            this, &VerilatorMakeStepWidget::makeArgumentsLineEditTextEdited);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrrideLabel()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));

    connect(makeStep->target(), SIGNAL(kitChanged()),
            this, SLOT(updateMakeOverrrideLabel()));

    connect(makeStep->target()->project(), &ProjectExplorer::Project::environmentChanged,
            this, &VerilatorMakeStepWidget::updateMakeOverrrideLabel);
    connect(makeStep->target()->project(), &ProjectExplorer::Project::environmentChanged,
            this, &VerilatorMakeStepWidget::updateDetails);
    connect(makeStep->target()->project(), &ProjectExplorer::Project::fileListChanged,
            this, &VerilatorMakeStepWidget::updateDetails);
}

QString VerilatorMakeStepWidget::displayName() const
{
    return tr("verilator");
}

QString VerilatorMakeStepWidget::summaryText() const
{
    return d_summary;
}

void VerilatorMakeStepWidget::makeLineEditTextEdited()
{
    d_step->d_cmd = d_cmd->text();
    updateDetails();
}

void VerilatorMakeStepWidget::makeArgumentsLineEditTextEdited()
{
    d_step->d_args = d_args->text();
    updateDetails();
}

void VerilatorMakeStepWidget::updateMakeOverrrideLabel()
{
    ProjectExplorer::BuildConfiguration *bc = d_step->buildConfiguration();
    if (!bc)
        bc = d_step->target()->activeBuildConfiguration();

    d_cmdLabel->setText(tr("Override %1:").arg(QDir::toNativeSeparators(d_step->makeCommand(bc->environment()))));
}

void VerilatorMakeStepWidget::updateDetails()
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
    param.setArguments( QString("%4 --Mdir %2 -f %1 %3").arg(s_cmdFileName)
                        .arg(s_simDir).arg(d_step->d_args)
                        .arg( p->getConfig("VLTR_ARGS").join(QChar(' ')) ));

    d_summary = param.summary(displayName());
    emit updateSummary();
}


VerilatorCleanStep::VerilatorCleanStep(ProjectExplorer::BuildStepList* parent):
    AbstractProcessStep(parent, ID)
{

}

bool VerilatorCleanStep::init()
{
    return true;
}

void VerilatorCleanStep::run(QFutureInterface<bool>& fi)
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

ProjectExplorer::BuildStepConfigWidget* VerilatorCleanStep::createConfigWidget()
{
    return new VerilatorCleanStepWidget(this);
}


VerilatorCleanStepWidget::VerilatorCleanStepWidget(VerilatorCleanStep* makeStep)
{

}

QString VerilatorCleanStepWidget::displayName() const
{
    return "clean";
}

QString VerilatorCleanStepWidget::summaryText() const
{
    return tr("<b>remove</b> %1").arg(s_cmdFileName);
}


