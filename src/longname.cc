/*--------------------------------------------------------------------*\
|- Gestion des noms longs                                             -|
\*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <dos.h>
#include <direct.h>
#include <ctype.h>
#include <bios.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "kk.h"

struct RBREGS
    {
    long edi,esi,ebp;
    long reserved;
    long ebx,edx,ecx,eax;
    short flag,es,ds,fs,gs,ip,cs,sp,ss;
    };

static unsigned short DOSbuf1,DOSsel1;
static unsigned short DOSbuf2,DOSsel2;

static char buf1[256],buf2[256];

static struct RBREGS R;


/*--------------------------------------------------------------------*\
|- Gestion des interruptions en mode reel                             -|
\*--------------------------------------------------------------------*/
void DOS_Int(short i,struct RBREGS *RR)
{
union REGS R;
struct SREGS S;

segread(&S);
R.w.ax=0x300;
R.h.bl=i;
R.h.bh=0;
R.w.cx=0;     // Number of word to copy from the protected mode stack to
                                                  // the real mode stack
R.x.edi=(unsigned long)RR;

int386x(0x31,&R,&R,&S);
}

/*--------------------------------------------------------------------*\
|- Allocation des buffer DOSbuf1 et DOSbuf2                           -|
\*--------------------------------------------------------------------*/
void DOS_Alloc(void)
{
union REGS R;

R.w.ax=0x100;
R.w.bx=40;                            // Nombre de paragraphes � allouer

int386(0x31,&R,&R);

//if (carry flag!=0)

DOSbuf1=R.w.ax;
DOSsel1=R.w.dx;

R.w.ax=0x100;
R.w.bx=40;                            // Nombre de paragraphes � allouer

int386(0x31,&R,&R);

// if (carry flag!=0)

DOSbuf2=R.w.ax;
DOSsel2=R.w.dx;
}

/*--------------------------------------------------------------------*\
|- Liberation des buffers DOSbuf1 et DOSbuf2                          -|
\*--------------------------------------------------------------------*/
void DOS_Free(void)
{
union REGS R;

R.w.ax=0x101;
R.w.dx=DOSsel1;

int386(0x31,&R,&R);

R.w.ax=0x101;
R.w.dx=DOSsel2;

int386(0x31,&R,&R);
}


/*--------------------------------------------------------------------*\
|- Donne le nom long du fichier courant                               -|
\*--------------------------------------------------------------------*/
void InfoLongFile(FENETRE *Fen,char *chaine)
{
char *old,*nouv;
long l;

DOS_Alloc();

l=DOSbuf1*16;
old=(char*)l;

l=DOSbuf2*16;
nouv=(char*)l;

strcpy(old,Fen->path);
Path2Abs(old,Fen->F[Fen->pcur]->name);

strcpy(nouv,"");

R.eax=0x7160;
R.ecx=2;
R.ds=DOSbuf1;
R.esi=0;
R.es=DOSbuf2;
R.edi=0;

DOS_Int(0x21,&R);

if (R.eax!=0x7100)
    {
    FileinPath(nouv,chaine);
    }
    else
    {
    strcpy(chaine,Fen->F[Fen->pcur]->name);
    }

DOS_Free();
}


/*--------------------------------------------------------------------*\
|- Prend le vrai nom de from et le donne � to                         -|
|- ! inpath est toujours un nom court                                 -|
\*--------------------------------------------------------------------*/
void UpdateLongName(char *from,char *to)
{
char *old,*nouv;
long l;

FileinPath(from,buf1);
FileinPath(to,buf2);

DOS_Alloc();

l=DOSbuf1*16;
old=(char*)l;

l=DOSbuf2*16;
nouv=(char*)l;

Path2Abs(old,from);
strcpy(nouv,"");

R.eax=0x7160;
R.ecx=2;
R.ds=DOSbuf1;
R.esi=0;
R.es=DOSbuf2;
R.edi=0;

DOS_Int(0x21,&R);

if (R.eax!=0x7100)
    {
    if (WildCmp(buf1,buf2)==0)
        {
        strcpy(old,to);

        FileinPath(nouv,buf1);
        strcpy(buf2,to);
        Path2Abs(buf2,"..");
        Path2Abs(buf2,buf1);

        strcpy(nouv,buf2);

        R.eax=0x7156;
        R.ds=DOSbuf1;
        R.edx=0;
        R.es=DOSbuf2;
        R.edi=0;

        DOS_Int(0x21,&R);

        strcpy(old,buf2);
        strcpy(nouv,"");

        R.eax=0x7160;
        R.ecx=1;
        R.ds=DOSbuf1;
        R.esi=0;
        R.es=DOSbuf2;
        R.edi=0;

        DOS_Int(0x21,&R);

        FileinPath(nouv,buf1);
        Path2Abs(to,"..");
        Path2Abs(to,buf1);
        }
    else
        {
        FileinPath(to,buf1);
        buf1[8]=0;

        strcpy(old,to);
        Path2Abs(old,"..");
        Path2Abs(old,buf1);

        strcpy(nouv,to);

//        PrintAt(0,0,"(%s,%s)",old,nouv);
//        Wait(0,0,0);

        R.eax=0x7156;
        R.ds=DOSbuf1;
        R.edx=0;
        R.es=DOSbuf2;
        R.edi=0;

        DOS_Int(0x21,&R);
        }
    }

DOS_Free();
}

char Verif95(void)
{
char a;
char *old,*nouv;
long l;

DOS_Alloc();

l=DOSbuf1*16;
old=(char*)l;

l=DOSbuf2*16;
nouv=(char*)l;

strcpy(old,Fics->path);

strcpy(nouv,"");

R.eax=0x7160;
R.ecx=2;
R.ds=DOSbuf1;
R.esi=0;
R.es=DOSbuf2;
R.edi=0;

DOS_Int(0x21,&R);

if (R.eax!=0x7100)
    a=1;
    else
    a=0;

DOS_Free();

return a;
}


/*--------------------------------------------------------------------*\
|- Ejection du CD                                                     -|
\*--------------------------------------------------------------------*/
void EjectCD(FENETRE *Fen)
{
EjectDrive(toupper(Fen->path[0])-'A');

IOver=1;
CommandLine("#CD .");
}


/*--------------------------------------------------------------------*\
|- Set Name                                                           -|
\*--------------------------------------------------------------------*/
void SetWindowsTitle(void)
{
char *buf;
long l;

DOS_Alloc();

l=DOSbuf1*16;
buf=(char*)l;

strcpy(buf,"KKC");

R.eax=0X168E;
R.edx=1;
R.es=DOSbuf1;
R.edi=0;

DOS_Int(0x2F,&R);

strcpy(buf,"RedBug for King");

R.eax=0X168E;
R.edx=0;
R.es=DOSbuf1;
R.edi=0;

DOS_Int(0x2F,&R);

DOS_Free();
}


/*--------------------------------------------------------------------*\
|- Ejection DRIVE  (lect: 0 -> 'a')                                   -|
\*--------------------------------------------------------------------*/
void EjectDrive(char lect)
{
char *buf;
int i;
long l;
union REGS RR;

RR.w.ax=0x150B;
RR.w.cx=lect;
int386(0x2F,&RR,&RR);
if ( (RR.w.bx!=0xADAD) | (RR.w.ax==0) )
    return;

DOS_Alloc();

l=DOSbuf1*16;
buf=(char*)l;

buf[0]=13;
buf[1]=lect;
buf[2]=0;
for(i=3;i<32;i++)
    buf[i]=0;

R.eax=0X1510;
R.ebx=0;
R.ecx=lect;
R.es=DOSbuf1;


buf[0]=13;
buf[1]=lect;
buf[2]=12;
for(i=3;i<32;i++)
    buf[i]=0;

buf[14]=26;     // BYTE 26
buf[15]=0;
buf[16]=DOSbuf1&255;
buf[17]=DOSbuf1/256;

buf[18]=0;      // Taille du bloc
buf[19]=1;      // Taille du bloc

buf[26]=0;   // Open CD ------------------------------------------------

R.eax=0X1510;
R.ebx=0;
R.ecx=lect;
R.es=DOSbuf1;

DOS_Int(0x2F,&R);

WinMesg("Information Message","Put a CD-ROM in the tray",0);

buf[0]=13;
buf[1]=lect;
buf[2]=12;
for(i=3;i<32;i++)
    buf[i]=0;

buf[14]=26;     // BYTE 26
buf[15]=0;
buf[16]=DOSbuf1&255;
buf[17]=DOSbuf1/256;

buf[18]=1;  // Taille du bloc
buf[19]=0;  // Taille du bloc

buf[26]=5;  // Close CD ------------------------------------------------

R.eax=0X1510;
R.ebx=0;
R.ecx=lect;
R.es=DOSbuf1;

DOS_Int(0x2F,&R);

DOS_Free();

Delay(100);
}

