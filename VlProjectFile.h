/****************************************************************************
**
** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (C) 2018 Rochus Keller me@rochus-keller.ch
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License versions 2.0 or 3.0 as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file.  Please review the following information
** to ensure GNU General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.  In addition, as a special
** exception, Nokia gives you certain additional rights. These rights
** are described in the Nokia Qt GPL Exception version 1.3, included in
** the file GPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VLPROJECTFILE_H
#define VLPROJECTFILE_H

#include <qstringlist.h>
#include <qtextstream.h>
#include <qstring.h>
#include <qstack.h>
#include <qmap.h>
#include <qmetatype.h>

namespace Vl
{
struct ParsableBlock;
struct IteratorBlock;
struct FunctionBlock;

class ProjectFile
{
    typedef QMap<QString, QStringList> Place;
    struct ScopeBlock
    {
        enum TestStatus { TestNone, TestFound, TestSeek };
        ScopeBlock() : iterate(0), ignore(false), else_status(TestNone) { }
        ScopeBlock(bool i) : iterate(0), ignore(i), else_status(TestNone) { }
        ~ScopeBlock();
        IteratorBlock *iterate;
        uint ignore : 1, else_status : 2;
    };
    friend struct ParsableBlock;
    friend struct IteratorBlock;
    friend struct FunctionBlock;

    QStack<ScopeBlock> scope_blocks;
    QStack<FunctionBlock *> function_blocks;
    IteratorBlock *iterator;
    FunctionBlock *function;
    QMap<QString, FunctionBlock*> testFunctions, replaceFunctions;

    QString pfile;
    void reset();
    Place vars;
    bool parse(const QString &text, Place &place, int line_count=1);

    enum IncludeStatus {
        IncludeSuccess,
        IncludeFeatureAlreadyLoaded,
        IncludeFailure,
        IncludeNoExist,
        IncludeParseFailure
    };
    enum IncludeFlags {
        IncludeFlagNone = 0x00,
        IncludeFlagFeature = 0x01,
        IncludeFlagNewParser = 0x02,
        IncludeFlagNewProject = 0x04
    };
    IncludeStatus doProjectInclude(QString file, uchar flags, Place &place);

    bool doProjectCheckReqs(const QStringList &deps, Place &place);
    bool doVariableReplace(QString &str, Place &place);
    QStringList doVariableReplaceExpand(const QString &str, Place &place, bool *ok=0);
    void init(const Place *);
    static QStringList &values(const QString &v, Place &place);

public:
    ProjectFile() { init(0); }
    ProjectFile(ProjectFile *p, const Place *nvars=0);
    ProjectFile(const Place &nvars) { init(&nvars); }
    ~ProjectFile();

    inline bool parse(const QString &text) { return parse(text, vars); }
    inline bool read(const QString &file) { pfile = file; return read(file, vars); }

    QStringList userExpandFunctions() { return replaceFunctions.keys(); }
    QStringList userTestFunctions() { return testFunctions.keys(); }

    QString projectFile();

    bool doProjectTest(QString str, Place &place);
    bool doProjectTest(QString func, const QString &params, Place &place);
    bool doProjectTest(QString func, QStringList args, Place &place);
    bool doProjectTest(QString func, QList<QStringList> args, Place &place);
    QStringList doProjectExpand(QString func, const QString &params, Place &place);
    QStringList doProjectExpand(QString func, QStringList args, Place &place);
    QStringList doProjectExpand(QString func, QList<QStringList> args, Place &place);

    QStringList expand(const QString &v);
    QStringList expand(const QString &func, const QList<QStringList> &args);
    bool test(const QString &v);
    bool test(const QString &func, const QList<QStringList> &args);
    bool isActiveConfig(const QString &x, bool regex=false, Place *place=NULL);

    bool isSet(const QString &v);
    bool isEmpty(const QString &v);
    QStringList &values(const QString &v);
    QString first(const QString &v);
    Place &variables();

protected:
    bool read(const QString &file, Place &place);
    bool read(QTextStream &file, Place &place);

};

inline QString ProjectFile::projectFile()
{
    if (pfile == "-")
        return QString("(stdin)");
    return pfile;
}

inline QStringList &ProjectFile::values(const QString &v)
{ return values(v, vars); }

inline bool ProjectFile::isEmpty(const QString &v)
{ return !isSet(v) || values(v).isEmpty(); }

inline bool ProjectFile::isSet(const QString &v)
{ return vars.contains(v); }

inline QString ProjectFile::first(const QString &v)
{
    const QStringList vals = values(v);
    if(vals.isEmpty())
        return QString("");
    return vals.first();
}

inline ProjectFile::Place &ProjectFile::variables()
{ return vars; }

}
#endif // VLPROJECTFILE_H
