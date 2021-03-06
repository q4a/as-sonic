/* asmif.c */
/*****************************************************************************/
/* AS-Portierung                                                             */
/*                                                                           */
/* Befehle zur bedingten Assemblierung                                       */
/*                                                                           */
/* Historie: 15. 5.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/
/* $Id: asmif.c,v 1.4 2010/08/27 14:52:41 alfred Exp $                       */
/***************************************************************************** 
 * $Log: asmif.c,v $
 * Revision 1.4  2010/08/27 14:52:41  alfred
 * - some more overlapping strcpy() cleanups
 *
 * Revision 1.3  2008/11/23 10:39:15  alfred
 * - allow strings with NUL characters
 *
 * Revision 1.2  2007/11/24 22:48:02  alfred
 * - some NetBSD changes
 *
 * Revision 1.1  2003/11/06 02:49:18  alfred
 * - recreated
 *
 * Revision 1.3  2002/07/14 18:39:57  alfred
 * - fixed TempAll-related warnings
 *
 * Revision 1.2  2002/05/01 15:56:09  alfred
 * - print start line of IF/SWITCH construct when it ends
 *
 *****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "chunks.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"

#include "asmif.h"


PIfSave FirstIfSave;
Boolean IfAsm;       /* FALSE: in einer neg. IF-Sequenz-->kein Code */

static Boolean ActiveIF;

        static LongInt GetIfVal(char *Cond)
BEGIN
   Boolean IfOK;
   LongInt Tmp;

   FirstPassUnknown=False;
   Tmp=EvalIntExpression(Cond,Int32,&IfOK);
   if ((FirstPassUnknown) OR (NOT IfOK))
    BEGIN
     Tmp=1;
     if (FirstPassUnknown) WrError(1820);
     else if (NOT IfOK) WrError(1135);
    END

   return Tmp;
END


	static void AddBoolFlag(Boolean Flag)
BEGIN
   strmaxcpy(ListLine,Flag?"=>TRUE":"=>FALSE",255);
END


	static void PushIF(LongInt IfExpr)
BEGIN
   PIfSave NewSave;
   
   NewSave=(PIfSave) malloc(sizeof(TIfSave));
   NewSave->NestLevel = SaveIFs() + 1;
   NewSave->Next = FirstIfSave;
   NewSave->SaveIfAsm = IfAsm;
   NewSave->State = IfState_IFIF;
   NewSave->CaseFound = (IfExpr != 0);
   NewSave->StartLine = CurrLine;
   FirstIfSave = NewSave;
   IfAsm=(IfAsm AND (IfExpr != 0));
END


	static void CodeIF(void)
BEGIN
   LongInt IfExpr;

   ActiveIF=IfAsm;

   if (NOT IfAsm) IfExpr=1;
   else if (ArgCnt!=1)
    BEGIN
     WrError(1110); IfExpr=1;
    END
   else IfExpr=GetIfVal(ArgStr[1]);
   if (IfAsm) AddBoolFlag(IfExpr!=0);
   PushIF(IfExpr);
END


	static void CodeIFDEF(void)
BEGIN
   LongInt IfExpr;
   Boolean Defined;

   ActiveIF=IfAsm;

   if (NOT IfAsm) IfExpr=1;
   else if (ArgCnt!=1)
    BEGIN
     WrError(1110); IfExpr=1;
    END
   else
    BEGIN
     Defined=IsSymbolDefined(ArgStr[1]);
     if (IfAsm)
      strmaxcpy(ListLine,(Defined)?"=>DEFINED":"=>UNDEFINED",255);
     if (Memo("IFDEF")) IfExpr=(Defined)?1:0;
     else IfExpr=(Defined)?0:1;
    END
   PushIF(IfExpr);
END


	static void CodeIFUSED(void)
BEGIN
   LongInt IfExpr;
   Boolean Used;

   ActiveIF=IfAsm;

   if (NOT IfAsm) IfExpr=1;
   else if (ArgCnt!=1)
    BEGIN
     WrError(1110); IfExpr=1;
    END
   else
    BEGIN
     Used=IsSymbolUsed(ArgStr[1]);
     if (IfAsm)
      strmaxcpy(ListLine,(Used)?"=>USED":"=>UNUSED",255);
     if (Memo("IFUSED")) IfExpr=(Used)?1:0;
     else IfExpr=(Used)?0:1;
    END
   PushIF(IfExpr);
END


void CodeIFEXIST(void)
{
  LongInt IfExpr;
  Boolean Found;
  String NPath;

  ActiveIF = IfAsm;

  if (!IfAsm) IfExpr = 1;
  else if (ArgCnt != 1)
  {
    WrError(1110); IfExpr = 1;
  }
  else
  {
    strmaxcpy(ArgPart, (ArgStr[1][0] == '"') ? ArgStr[1] + 1 : ArgStr[1], 255);
    if (ArgPart[strlen(ArgPart)-1] == '"')
      ArgPart[strlen(ArgPart)-1] = '\0';
    AddSuffix(ArgPart, IncSuffix);
    strmaxcpy(NPath, IncludeList, 255);
    strmaxprep(NPath, ".:", 255);
    Found = (*(FSearch(ArgPart, NPath)) != '\0');
    if (IfAsm)
      strmaxcpy(ListLine, Found ? "=>FOUND" : "=>NOT FOUND", 255);
    IfExpr = Memo("IFEXIST") ? Found : !Found;
  }
  PushIF(IfExpr);
}


	static void CodeIFB(void)	
BEGIN
   Boolean Blank=True;
   LongInt IfExpr;
   int z;
   
   ActiveIF=IfAsm;

   if (NOT IfAsm) IfExpr=1;
   else
    BEGIN
     for (z=1; z<=ArgCnt; z++) if (strlen(ArgStr[z++])>0) Blank=False;
     if (IfAsm)
      strmaxcpy(ListLine,(Blank)?"=>BLANK":"=>NOT BLANK",255);
     if (Memo("IFB")) IfExpr=(Blank)?1:0;
     else IfExpr=(Blank)?0:1;
    END
   PushIF(IfExpr); 
END


	static void CodeELSEIF(void)
BEGIN
   LongInt IfExpr;

   if (FirstIfSave==Nil) WrError(1840);
   else if (ArgCnt==0)
    BEGIN
     if (FirstIfSave->State!=IfState_IFIF) WrError(1480);
     else if (FirstIfSave->SaveIfAsm) AddBoolFlag(IfAsm=(NOT FirstIfSave->CaseFound));
     FirstIfSave->State=IfState_IFELSE;
    END
   else if (ArgCnt==1)
    BEGIN
     if (FirstIfSave->State!=IfState_IFIF) WrError(1480);
     else
      BEGIN
       if (NOT FirstIfSave->SaveIfAsm) IfExpr=1;
       else if (FirstIfSave->CaseFound) IfExpr=0;
       else IfExpr=GetIfVal(ArgStr[1]);
       IfAsm=((FirstIfSave->SaveIfAsm) AND (IfExpr!=0) AND (NOT FirstIfSave->CaseFound));
       if (FirstIfSave->SaveIfAsm) AddBoolFlag(IfExpr!=0);
       if (IfExpr!=0) FirstIfSave->CaseFound=True;
      END
    END
   else WrError(1110);

   ActiveIF=(FirstIfSave==Nil) OR (FirstIfSave->SaveIfAsm);
END


	static void CodeENDIF(void)
BEGIN
   PIfSave NewSave;

   if (ArgCnt!=0) WrError(1110);
   if (FirstIfSave==Nil) WrError(1840);
   else
    BEGIN
     if ((FirstIfSave->State!=IfState_IFIF) AND (FirstIfSave->State!=IfState_IFELSE)) WrError(1480);
     else
      BEGIN
       IfAsm=FirstIfSave->SaveIfAsm;
       NewSave=FirstIfSave; FirstIfSave=NewSave->Next;
       sprintf(ListLine, "[%u]", (unsigned)NewSave->StartLine);
       free(NewSave);
      END
    END

   ActiveIF=IfAsm;
END


	static void EvalIfExpression(char *Cond, TempResult *erg)
BEGIN
   FirstPassUnknown=False;
   EvalExpression(Cond,erg);
   if ((erg->Typ==TempNone) OR (FirstPassUnknown))
    BEGIN
     erg->Typ=TempInt; erg->Contents.Int=1;
     if (FirstPassUnknown) WrError(1820);
    END
END


	static void CodeSWITCH(void)
BEGIN
   PIfSave NewSave;

   ActiveIF=IfAsm;

   NewSave=(PIfSave) malloc(sizeof(TIfSave));
   NewSave->NestLevel=SaveIFs()+1;
   NewSave->Next=FirstIfSave; NewSave->SaveIfAsm=IfAsm;
   NewSave->CaseFound=False; NewSave->State=IfState_CASESWITCH;
   NewSave->StartLine = CurrLine;
   if (ArgCnt!=1)
    BEGIN
     NewSave->SaveExpr.Typ=TempInt; 
     NewSave->SaveExpr.Contents.Int=1; 
     if (IfAsm) WrError(1110);
    END
   else
    BEGIN
     EvalIfExpression(ArgStr[1],&(NewSave->SaveExpr));
     SetListLineVal(&(NewSave->SaveExpr));
    END
   FirstIfSave=NewSave;
END


static void CodeCASE(void)
{
  Boolean eq;
  int z;
  TempResult t;

  if (!FirstIfSave) WrError(1840);
  else if (!ArgCnt) WrError(1110);
  else 
  {
    if ((FirstIfSave->State != IfState_CASESWITCH) && (FirstIfSave->State != IfState_CASECASE)) WrError(1480);
    else
    {
      if (!FirstIfSave->SaveIfAsm) eq = True;
      else if (FirstIfSave->CaseFound) eq = False;
      else
      {
        eq = False; z = 1;
        do
        {
          EvalIfExpression(ArgStr[z], &t);
          eq = (FirstIfSave->SaveExpr.Typ == t.Typ);
          if (eq)
           switch (t.Typ)
           {
             case TempInt:
               eq = (t.Contents.Int == FirstIfSave->SaveExpr.Contents.Int);
               break;
             case TempFloat:
               eq = (t.Contents.Float == FirstIfSave->SaveExpr.Contents.Float);
               break;
             case TempString:
               eq = (DynStringCmp(&t.Contents.Ascii, &FirstIfSave->SaveExpr.Contents.Ascii) == 0);
               break;
             default:
               eq = False;
               break;
            END
          z++;
        }
        while ((!eq) && (z <= ArgCnt));
      }
      IfAsm = ((FirstIfSave->SaveIfAsm) && (eq) & (!FirstIfSave->CaseFound));
      if (FirstIfSave->SaveIfAsm)
        AddBoolFlag(eq && (!FirstIfSave->CaseFound));
      if (eq)
        FirstIfSave->CaseFound = True;
      FirstIfSave->State=IfState_CASECASE;
    }
  }

  ActiveIF = (FirstIfSave == Nil) || (FirstIfSave->SaveIfAsm);
}


	static void CodeELSECASE(void)
BEGIN
   if (ArgCnt!=0) WrError(1110);
   else
    BEGIN
     if ((FirstIfSave->State!=IfState_CASESWITCH) AND (FirstIfSave->State!=IfState_CASECASE)) WrError(1480);
     else IfAsm=(FirstIfSave->SaveIfAsm AND (NOT FirstIfSave->CaseFound));
     if (FirstIfSave->SaveIfAsm) AddBoolFlag(NOT FirstIfSave->CaseFound);
     FirstIfSave->CaseFound=True;
     FirstIfSave->State=IfState_CASEELSE;
    END

   ActiveIF=(FirstIfSave==Nil) OR (FirstIfSave->SaveIfAsm);
END


	static void CodeENDCASE(void)
BEGIN
   PIfSave NewSave;

   if (ArgCnt!=0) WrError(1110);
   if (FirstIfSave==Nil) WrError(1840);
   else 
    BEGIN
     if ((FirstIfSave->State!=IfState_CASESWITCH) 
     AND (FirstIfSave->State!=IfState_CASECASE) 
     AND (FirstIfSave->State!=IfState_CASEELSE)) WrError(1480);
     else
      BEGIN
       IfAsm=FirstIfSave->SaveIfAsm;
       if (NOT FirstIfSave->CaseFound) WrError(100);
       NewSave=FirstIfSave; FirstIfSave=NewSave->Next;
       sprintf(ListLine, "[%u]", (unsigned)NewSave->StartLine);
       free(NewSave);
      END
    END

   ActiveIF=IfAsm;
END


	Boolean CodeIFs(void)
BEGIN
   Boolean Result=True;

   ActiveIF=False;

   switch (mytoupper(*OpPart))
    BEGIN
     case 'I':
      if (Memo("IF")) CodeIF();
      else if ((Memo("IFDEF")) OR (Memo("IFNDEF"))) CodeIFDEF();
      else if ((Memo("IFUSED")) OR (Memo("IFNUSED"))) CodeIFUSED();
      else if ((Memo("IFEXIST")) OR (Memo("IFNEXIST"))) CodeIFEXIST();
      else if ((Memo("IFB")) OR (Memo("IFNB"))) CodeIFB();
      else Result=False;
      break;
     case 'E':
      if ((Memo("ELSE")) OR (Memo("ELSEIF"))) CodeELSEIF();
      else if (Memo("ENDIF")) CodeENDIF();
      else if (Memo("ELSECASE")) CodeELSECASE();
      else if (Memo("ENDCASE")) CodeENDCASE();
      else Result=False;
      break;
     case 'S':
      if (Memo("SWITCH")) CodeSWITCH();
      else Result=False;
      break;
     case 'C':
      if (Memo("CASE")) CodeCASE();
      else Result=False;
      break;
     default:
      Result=False;
    END

   return Result;
END

	Integer SaveIFs(void)
BEGIN
   return (FirstIfSave==Nil) ? 0 : FirstIfSave->NestLevel;
END

	void RestoreIFs(Integer Level)
BEGIN
   PIfSave OldSave;

   while ((FirstIfSave!=Nil) AND (FirstIfSave->NestLevel!=Level))
    BEGIN
     OldSave=FirstIfSave; FirstIfSave=OldSave->Next;
     IfAsm=OldSave->SaveIfAsm;
     free(OldSave);
    END
END


	Boolean IFListMask(void)
BEGIN
   switch (ListOn)
    BEGIN
     case 0: return True;
     case 1: return False;
     case 2: return ((NOT ActiveIF) AND (NOT IfAsm));
     case 3: return (ActiveIF OR (NOT IfAsm));
    END
   return True;
END


	void AsmIFInit(void)
BEGIN
   IfAsm=True;
END


	void asmif_init(void)
BEGIN
END
