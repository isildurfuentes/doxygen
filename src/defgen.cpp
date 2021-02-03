/******************************************************************************
 *
 *
 *
 *
 * Copyright (C) 1997-2015 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby
 * granted. No representations are made about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

#include <stdlib.h>

#include "defgen.h"
#include "doxygen.h"
#include "message.h"
#include "config.h"
#include "classlist.h"
#include "util.h"
#include "defargs.h"
#include "outputgen.h"
#include "dot.h"
#include "dotclassgraph.h"
#include "arguments.h"
#include "memberlist.h"
#include "namespacedef.h"
#include "filedef.h"
#include "filename.h"

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>

#define DEF_DB(x)

static inline void writeDEFString(FTextStream &t,const char *s)
{
  const char* p=s;
  char c;

  t << '\'';
  while ((c = *(p++)))
  {
    if (c == '\'')
      t << '\\';
    t << c;
  }
  t << '\'';
}

static void generateDEFForMember(const MemberDef *md,
    FTextStream &t,
    const Definition *def,
    const char* Prefix)
{
  QCString memPrefix;

  // + declaration
  // - reimplements
  // - reimplementedBy
  // - exceptions
  // - const/volatile specifiers
  // - examples
  // + source definition
  // - source references
  // - source referenced by
  // - include code

  if (md->memberType()==MemberType_EnumValue) return;

  QCString scopeName;
  if (md->getClassDef())
    scopeName=md->getClassDef()->name();
  else if (md->getNamespaceDef())
    scopeName=md->getNamespaceDef()->name();

  t << "    " << Prefix << "-member = {" << endl;
  memPrefix = "      ";
  memPrefix.append( Prefix );
  memPrefix.append( "-mem-" );

  QCString memType;
  bool isFunc=FALSE;
  switch (md->memberType())
  {
    case MemberType_Define:      memType="define";     break;
    case MemberType_EnumValue:   ASSERT(0);            break;
    case MemberType_Property:    memType="property";   break;
    case MemberType_Event:       memType="event";      break;
    case MemberType_Variable:    memType="variable";   break;
    case MemberType_Typedef:     memType="typedef";    break;
    case MemberType_Enumeration: memType="enum";       break;
    case MemberType_Interface:   memType="interface";  break;
    case MemberType_Service:     memType="service";    break;
    case MemberType_Sequence:    memType="sequence";   break;
    case MemberType_Dictionary:  memType="dictionary"; break;
    case MemberType_Function:    memType="function";   isFunc=TRUE; break;
    case MemberType_Signal:      memType="signal";     isFunc=TRUE; break;
    case MemberType_Friend:      memType="friend";     isFunc=TRUE; break;
    case MemberType_DCOP:        memType="dcop";       isFunc=TRUE; break;
    case MemberType_Slot:        memType="slot";       isFunc=TRUE; break;
  }

  t << memPrefix << "kind = '" << memType << "';" << endl;
  t << memPrefix << "id   = '"
    << md->getOutputFileBase() << "_1" << md->anchor()
    << "';" << endl;

  t << memPrefix << "virt = ";
  switch (md->virtualness())
  {
    case Normal:  t << "normal;"       << endl; break;
    case Virtual: t << "virtual;"      << endl; break;
    case Pure:    t << "pure-virtual;" << endl; break;
    default: ASSERT(0);
  }

  t << memPrefix << "prot = ";
  switch(md->protection())
  {
    case Public:    t << "public;"    << endl; break;
    case Protected: t << "protected;" << endl; break;
    case Private:   t << "private;"   << endl; break;
    case Package:   t << "package;"   << endl; break;
  }

  if (md->memberType()!=MemberType_Define &&
      md->memberType()!=MemberType_Enumeration
     )
  {
    QCString typeStr = replaceAnonymousScopes(md->typeString());
    t << memPrefix << "type = <<_EnD_oF_dEf_TeXt_" << endl << typeStr << endl
      << "_EnD_oF_dEf_TeXt_;" << endl;
  }

  t << memPrefix << "name = '" << md->name() << "';" << endl;

  if (isFunc) //function
  {
    const ArgumentList &defAl = md->argumentList();
    ArgumentList declAl = *stringToArgumentList(md->getLanguage(),md->argsString());
    QCString fcnPrefix = "  " + memPrefix + "param-";

    auto defIt = defAl.begin();
    for (const Argument &a : declAl)
    {
      const Argument *defArg = 0;
      if (defIt!=defAl.end())
      {
        defArg = &(*defIt);
        ++defIt;
      }
      t << memPrefix << "param = {" << endl;
      if (!a.attrib.isEmpty())
      {
        t << fcnPrefix << "attributes = ";
        writeDEFString(t,a.attrib);
        t << ';' << endl;
      }
      if (!a.type.isEmpty())
      {
        t << fcnPrefix << "type = <<_EnD_oF_dEf_TeXt_" << endl
          << a.type << endl << "_EnD_oF_dEf_TeXt_;" << endl;
      }
      if (!a.name.isEmpty())
      {
        t << fcnPrefix << "declname = ";
        writeDEFString(t,a.name);
        t << ';' << endl;
      }
      if (defArg && !defArg->name.isEmpty() && defArg->name!=a.name)
      {
        t << fcnPrefix << "defname = ";
        writeDEFString(t,defArg->name);
        t << ';' << endl;
      }
      if (!a.array.isEmpty())
      {
        t << fcnPrefix << "array = ";
        writeDEFString(t,a.array);
        t << ';' << endl;
      }
      if (!a.defval.isEmpty())
      {
        t << fcnPrefix << "defval = <<_EnD_oF_dEf_TeXt_" << endl
          << a.defval << endl << "_EnD_oF_dEf_TeXt_;" << endl;
      }
      t << "      }; /*" << fcnPrefix << "-param */" << endl;
    }
  }
  else if (  md->memberType()==MemberType_Define
      && md->argsString()!=0)
  {
    QCString defPrefix = "  " + memPrefix + "def-";
    for (const Argument &a : md->argumentList())
    {
      t << memPrefix << "param  = {" << endl;
      t << defPrefix << "name = '" << a.type << "';" << endl;
      t << "      }; /*" << defPrefix << "-param */" << endl;
    }
  }

  if (!md->initializer().isEmpty())
  {
    t << memPrefix << "initializer = <<_EnD_oF_dEf_TeXt_" << endl
      << md->initializer() << endl << "_EnD_oF_dEf_TeXt_;" << endl;
  }
  // TODO: exceptions, const volatile
  if (md->memberType()==MemberType_Enumeration) // enum
  {
    for (const auto &emd : md->enumFieldList())
    {
      t << memPrefix << "enum = { enum-name = " << emd->name() << ';';
      if (!emd->initializer().isEmpty())
      {
        t << " enum-value = ";
        writeDEFString(t,emd->initializer());
        t << ';';
      }
      t << " };" << endl;
    }
  }

  t << memPrefix << "desc-file = '" << md->getDefFileName() << "';" << endl;
  t << memPrefix << "desc-line = '" << md->getDefLine()     << "';" << endl;
  t << memPrefix << "briefdesc =    <<_EnD_oF_dEf_TeXt_"    << endl
    << md->briefDescription() << endl << "_EnD_oF_dEf_TeXt_;" << endl;
  t << memPrefix << "documentation = <<_EnD_oF_dEf_TeXt_"   << endl
    << md->documentation() << endl << "_EnD_oF_dEf_TeXt_;" << endl;

  //printf("md->getReferencesMembers()=%p\n",md->getReferencesMembers());

  QCString refPrefix = "  " + memPrefix + "ref-";
  auto refList = md->getReferencesMembers();
  for (const auto &rmd : refList)
  {
    if (rmd->getStartBodyLine()!=-1 && rmd->getBodyDef())
    {
      t << memPrefix << "referenceto = {" << endl;
      t << refPrefix << "id = '"
        << rmd->getBodyDef()->getOutputFileBase()
        << "_1"   // encoded ':' character (see util.cpp:convertNameToFile)
        << rmd->anchor() << "';" << endl;

      t << refPrefix << "line = '"
        << rmd->getStartBodyLine() << "';" << endl;

      QCString scope = rmd->getScopeString();
      QCString name = rmd->name();
      if (!scope.isEmpty() && scope!=def->name())
      {
        name.prepend(scope+"::");
      }

      t << refPrefix << "name = ";
      writeDEFString(t,name);
      t << ';' << endl << "    };" << endl;
    }
  }
  auto refByList = md->getReferencedByMembers();
  for (const auto &rmd : refByList)
  {
    if (rmd->getStartBodyLine()!=-1 && rmd->getBodyDef())
    {
      t << memPrefix << "referencedby = {" << endl;
      t << refPrefix << "id = '"
        << rmd->getBodyDef()->getOutputFileBase()
        << "_1"   // encoded ':' character (see util.cpp:convertNameToFile)
        << rmd->anchor() << "';" << endl;

      t << refPrefix << "line = '"
        << rmd->getStartBodyLine() << "';" << endl;

      QCString scope = rmd->getScopeString();
      QCString name = rmd->name();
      if (!scope.isEmpty() && scope!=def->name())
      {
        name.prepend(scope+"::");
      }

      t << refPrefix << "name = ";
      writeDEFString(t,name);
      t << ';' << endl << "    };" << endl;
    }
  }

  t << "    }; /* " << Prefix << "-member */" << endl;
}


static void generateDEFClassSection(const ClassDef *cd,
    FTextStream &t,
    const MemberList *ml,
    const char *kind)
{
  if (cd && ml && !ml->empty())
  {
    t << "  cp-section = {" << endl;
    t << "    sec-kind = '" << kind << "';" << endl;

    for (const auto &md : *ml)
    {
      generateDEFForMember(md,t,cd,"sec");
    }
    t << "  }; /* cp-section */" << endl;
  }
}

static void generateDEFForClass(const ClassDef *cd,FTextStream &t)
{
  // + brief description
  // + detailed description
  // - template arguments
  // - include files
  // + inheritance diagram
  // + list of direct super classes
  // + list of direct sub classes
  // + collaboration diagram
  // - list of all members
  // + user defined member sections
  // + standard member sections
  // + detailed member documentation
  // - examples

  if (cd->isReference()) return; // skip external references.
  if (cd->name().find('@')!=-1) return; // skip anonymous compounds.
  if (cd->templateMaster()!=0) return; // skip generated template instances.

  t << cd->compoundTypeString() << " = {" << endl;
  t << "  cp-id     = '" << cd->getOutputFileBase() << "';" << endl;
  t << "  cp-name   = '" << cd->name() << "';" << endl;

  for (const auto &bcd : cd->baseClasses())
  {
    t << "  cp-ref     = {" << endl << "    ref-type = base;" << endl;
    t << "    ref-id   = '"
      << bcd.classDef->getOutputFileBase() << "';" << endl;
    t << "    ref-prot = ";
    switch (bcd.prot)
    {
      case Public:    t << "public;"    << endl; break;
      case Package: // package scope is not possible
      case Protected: t << "protected;" << endl; break;
      case Private:   t << "private;"   << endl; break;
    }
    t << "    ref-virt = ";
    switch(bcd.virt)
    {
      case Normal:  t << "non-virtual;";  break;
      case Virtual: t << "virtual;";      break;
      case Pure:    t << "pure-virtual;"; break;
    }
    t << endl << "  };" << endl;
  }

  for (const auto &bcd : cd->subClasses())
  {
    t << "  cp-ref     = {" << endl << "    ref-type = derived;" << endl;
    t << "    ref-id   = '"
      << bcd.classDef->getOutputFileBase() << "';" << endl;
    t << "    ref-prot = ";
    switch (bcd.prot)
    {
      case Public:    t << "public;"    << endl; break;
      case Package: // packet scope is not possible!
      case Protected: t << "protected;" << endl; break;
      case Private:   t << "private;"   << endl; break;
    }
    t << "    ref-virt = ";
    switch (bcd.virt)
    {
      case Normal:  t << "non-virtual;";  break;
      case Virtual: t << "virtual;";      break;
      case Pure:    t << "pure-virtual;"; break;
    }
    t << endl << "  };" << endl;
  }

  size_t numMembers = 0;
  for (const auto &ml : cd->getMemberLists())
  {
    if ((ml->listType()&MemberListType_detailedLists)==0)
    {
      numMembers+=ml->size();
    }
  }
  if (numMembers>0)
  {
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_pubTypes),"public-type");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_interfaces),"interfaces");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_services),"services");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_pubMethods),"public-func");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_pubAttribs),"public-attrib");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_pubSlots),"public-slot");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_signals),"signal");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_dcopMethods),"dcop-func");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_properties),"property");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_pubStaticMethods),"public-static-func");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_pubStaticAttribs),"public-static-attrib");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_proTypes),"protected-type");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_proMethods),"protected-func");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_proAttribs),"protected-attrib");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_proSlots),"protected-slot");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_proStaticMethods),"protected-static-func");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_proStaticAttribs),"protected-static-attrib");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_priTypes),"private-type");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_priMethods),"private-func");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_priAttribs),"private-attrib");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_priSlots),"private-slot");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_priStaticMethods),"private-static-func");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_priStaticAttribs),"private-static-attrib");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_friends),"signal");
    generateDEFClassSection(cd,t,cd->getMemberList(MemberListType_related),"related");
  }

  t << "  cp-filename  = '" << cd->getDefFileName() << "';" << endl;
  t << "  cp-fileline  = '" << cd->getDefLine()     << "';" << endl;
  t << "  cp-briefdesc = <<_EnD_oF_dEf_TeXt_" << endl
    << cd->briefDescription() << endl << "_EnD_oF_dEf_TeXt_;" << endl;

  t << "  cp-documentation = <<_EnD_oF_dEf_TeXt_" << endl
    << cd->documentation() << endl << "_EnD_oF_dEf_TeXt_;" << endl;

  DotClassGraph inheritanceGraph(cd,Inheritance);
  if (!inheritanceGraph.isTrivial())
  {
    t << "  cp-inheritancegraph = <<_EnD_oF_dEf_TeXt_" << endl;
    inheritanceGraph.writeDEF(t);
    t << endl << "_EnD_oF_dEf_TeXt_;" << endl;
  }
  DotClassGraph collaborationGraph(cd,Collaboration);
  if (!collaborationGraph.isTrivial())
  {
    t << "  cp-collaborationgraph = <<_EnD_oF_dEf_TeXt_" << endl;
    collaborationGraph.writeDEF(t);
    t << endl << "_EnD_oF_dEf_TeXt_;" << endl;
  }
  t << "}; /* " <<  cd->compoundTypeString() << " */" << endl;
}

static void generateDEFSection(const Definition *d,
    FTextStream &t,
    const MemberList *ml,
    const char *kind)
{
  if (ml && !ml->empty())
  {
    t << "    " << kind << " = {" << endl;
    for (const auto &md : *ml)
    {
      generateDEFForMember(md,t,d,kind);
    }
    t << "    };" << endl;
  }
}

static void generateDEFForNamespace(const NamespaceDef *nd,FTextStream &t)
{
  if (nd->isReference()) return; // skip external references
  t << "  namespace = {" << endl;
  t << "    ns-id   = '" << nd->getOutputFileBase() << "';" << endl;
  t << "    ns-name = ";
  writeDEFString(t,nd->name());
  t << ';' << endl;

  generateDEFSection(nd,t,nd->getMemberList(MemberListType_decDefineMembers),"define");
  generateDEFSection(nd,t,nd->getMemberList(MemberListType_decProtoMembers),"prototype");
  generateDEFSection(nd,t,nd->getMemberList(MemberListType_decTypedefMembers),"typedef");
  generateDEFSection(nd,t,nd->getMemberList(MemberListType_decSequenceMembers),"sequence");
  generateDEFSection(nd,t,nd->getMemberList(MemberListType_decDictionaryMembers),"dictionary");
  generateDEFSection(nd,t,nd->getMemberList(MemberListType_decEnumMembers),"enum");
  generateDEFSection(nd,t,nd->getMemberList(MemberListType_decFuncMembers),"func");
  generateDEFSection(nd,t,nd->getMemberList(MemberListType_decVarMembers),"var");

  t << "  ns-filename  = '" << nd->getDefFileName() << "';" << endl;
  t << "  ns-fileline  = '" << nd->getDefLine()     << "';" << endl;
  t << "  ns-briefdesc = <<_EnD_oF_dEf_TeXt_" << endl
    << nd->briefDescription() << endl << "_EnD_oF_dEf_TeXt_;" << endl;

  t << "  ns-documentation = <<_EnD_oF_dEf_TeXt_" << endl
    << nd->documentation() << endl << "_EnD_oF_dEf_TeXt_;" << endl;
  t << "  };" << endl;
}

static void generateDEFForFile(const FileDef *fd,FTextStream &t)
{
  if (fd->isReference()) return; // skip external references

  t << "file = {" << endl;
  t << "  file-id   = '" << fd->getOutputFileBase() << "';" << endl;
  t << "  file-name = ";
  writeDEFString(t,fd->name());
  t << ';' << endl;

  generateDEFSection(fd,t,fd->getMemberList(MemberListType_decDefineMembers),"define");
  generateDEFSection(fd,t,fd->getMemberList(MemberListType_decProtoMembers),"prototype");
  generateDEFSection(fd,t,fd->getMemberList(MemberListType_decTypedefMembers),"typedef");
  generateDEFSection(fd,t,fd->getMemberList(MemberListType_decSequenceMembers),"sequence");
  generateDEFSection(fd,t,fd->getMemberList(MemberListType_decDictionaryMembers),"dictionary");
  generateDEFSection(fd,t,fd->getMemberList(MemberListType_decEnumMembers),"enum");
  generateDEFSection(fd,t,fd->getMemberList(MemberListType_decFuncMembers),"func");
  generateDEFSection(fd,t,fd->getMemberList(MemberListType_decVarMembers),"var");

  t << "  file-full-name  = '" << fd->getDefFileName() << "';" << endl;
  t << "  file-first-line = '" << fd->getDefLine()     << "';" << endl;

  t << "  file-briefdesc  = <<_EnD_oF_dEf_TeXt_" << endl
    << fd->briefDescription() << endl << "_EnD_oF_dEf_TeXt_;" << endl;

  t << "  file-documentation = <<_EnD_oF_dEf_TeXt_" << endl
    << fd->documentation() << endl << "_EnD_oF_dEf_TeXt_;" << endl;

  t << "}; /* file */" << endl;
}


void generateDEF()
{
  QCString outputDirectory = Config_getString(OUTPUT_DIRECTORY);
  if (outputDirectory.isEmpty())
  {
    outputDirectory=QDir::currentDirPath().utf8();
  }
  else
  {
    QDir dir(outputDirectory);
    if (!dir.exists())
    {
      dir.setPath(QDir::currentDirPath());
      if (!dir.mkdir(outputDirectory))
      {
        term("tag OUTPUT_DIRECTORY: Output directory '%s' does not "
            "exist and cannot be created\n",outputDirectory.data());
      }
      else
      {
        msg("Notice: Output directory '%s' does not exist. "
            "I have created it for you.\n", outputDirectory.data());
      }
      dir.cd(outputDirectory);
    }
    outputDirectory=dir.absPath().utf8();
  }

  QDir dir(outputDirectory);
  if (!dir.exists())
  {
    dir.setPath(QDir::currentDirPath());
    if (!dir.mkdir(outputDirectory))
    {
      err("Cannot create directory %s\n",outputDirectory.data());
      return;
    }
  }
  QDir defDir(outputDirectory+"/def");
  if (!defDir.exists() && !defDir.mkdir(outputDirectory+"/def"))
  {
    err("Could not create def directory in %s\n",outputDirectory.data());
    return;
  }

  QCString fileName=outputDirectory+"/def/doxygen.def";
  QFile f(fileName);
  if (!f.open(IO_WriteOnly))
  {
    err("Cannot open file %s for writing!\n",fileName.data());
    return;
  }
  FTextStream t(&f);
  t << "AutoGen Definitions dummy;" << endl;

  if (Doxygen::classLinkedMap->size()+
      Doxygen::inputNameLinkedMap->size()+
      Doxygen::namespaceLinkedMap->size()>0)
  {
    for (const auto &cd : *Doxygen::classLinkedMap)
    {
      generateDEFForClass(cd.get(),t);
    }
    for (const auto &fn : *Doxygen::inputNameLinkedMap)
    {
      for (const auto &fd : *fn)
      {
        generateDEFForFile(fd.get(),t);
      }
    }
    for (const auto &nd : *Doxygen::namespaceLinkedMap)
    {
      generateDEFForNamespace(nd.get(),t);
    }
  }
  else
  {
    t << "dummy_value = true;" << endl;
  }
}
