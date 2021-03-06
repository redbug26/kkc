/*--------------------------------------------------------------------*\
|- procedure d'effacement                                             -|
\*--------------------------------------------------------------------*/
#ifndef LINUX
    #include <io.h>
    #include <dos.h>
    #include <direct.h>
#endif

#include <stdio.h>
#include <string.h>

#include "kk.h"
#include "gestion.h"

int Erase(char *path, struct file *F);
int Deltree(char *path);

/*--------------------------------------------------------------------*\
|-  Delete File                                                       -|
\*--------------------------------------------------------------------*/

int Del(char *s)
{
    static char buffer[256];

    strcpy(buffer, s);
    buffer[58] = 0;
    PrintTo(1, 0, "Delete%58s", buffer);

// _dos_setfileattr(s,RB_NORMAL);

    IOerr = 0;

    return remove(s);
}


/*--------------------------------------------------------------------*\
|-  Deltree                                                           -|
\*--------------------------------------------------------------------*/

int Deltree(char *path)
{
#ifdef __WC32__

    DIR *dirp;
    struct dirent *ff;

    char error;

    Path2Abs(path, "*.*");
    dirp = opendir(path);
    Path2Abs(path, "..");

    if (dirp != NULL)
        while (1) {
            ff = readdir(dirp);
            if (ff == NULL) break;

            error = ff->d_attr;

            if (ff->d_name[0] != '.') {
                Path2Abs(path, ff->d_name);
                if ((error & 0x10) == 0x10)
                    Deltree(path);
                else
                    Del(path);
                Path2Abs(path, "..");
            }
        }

    closedir(dirp);

    IOerr = 0;

    return rmdir(path);
#else // ifdef __WC32__
    return 1;
#endif // ifdef __WC32__
} // Deltree



/*--------------------------------------------------------------------*\
|-  Efface convenablement                                             -|
\*--------------------------------------------------------------------*/
int Erase(char *path, struct file *F)
{
    int i;

    if ((F->attrib & RB_SUBDIR) == RB_SUBDIR)
        i = Deltree(path);
    else
        i = Del(path);

    return i;
}


/*--------------------------------------------------------------------*\
|-  retourne 0 si pas copie                                           -|
\*--------------------------------------------------------------------*/
int FenDelete(char *dest)
{
    static char CadreLength = 70;
    static char Dir[256];

    struct Tmt T[6] = {
        {5,  5, 5, "   Delete    ", NULL},                 // Delete
        {30, 5, 5, " Delete All  ", NULL},                 // Delete All
        {55, 5, 3, NULL,            NULL},
        {5,  3, 0, Dir,             NULL},
        {5,  2, 0, "Delete file",   NULL},
        {1,  1, 4, &CadreLength,    NULL}
    };

    struct TmtWin F = { -1, 10, 74, 17, "Delete" };

    int n;

    memcpy(Dir, dest, 255);

    NumHelp(13);
    n = WinTraite(T, 6, &F, 0);

    return n;

} // FenDelete

int DelErr(char *dest)
{
    static char CadreLength = 70;
    static char Dir[256];

    struct Tmt T[5] = {
        {15, 5, 2, NULL,                   NULL},              // Delete
        {45, 5, 3, NULL,                   NULL},
        {5,  3, 0, Dir,                    NULL},
        {5,  2, 0, "Couldn't delete file", NULL},
        {1,  1, 4, &CadreLength,           NULL}
    };

    struct TmtWin F = { -1, 10, 74, 17, "Error!" };

    int n;

    memcpy(Dir, dest, 255);

    n = WinTraite(T, 5, &F, 0);

    return n;

} // DelErr


int Delete(FENETRE *F1)  // --- Delete Multiple -------------------------
{
    int i;
    int x1;

    char fin;
    char test;
    signed char car;

    char inpath[128];

    struct file *F;

    if (F1->FenTyp != 0)
        return -1;

    SaveScreen();

    x1 = (Cfg->TailleX - 66) / 2;

    Cadre(x1, 10, x1 + 66, 12, 0, Cfg->col[55], Cfg->col[56]);
    Window(x1 + 1, 11, x1 + 65, 11, Cfg->col[16]);


    if ( (F1->nbrsel == 0) & (F1->F[F1->pcur]->name[0] != '.') ) {
        F1->F[F1->pcur]->select = 1;
        F1->nbrsel++;
        F1->taillesel += F1->F[F1->pcur]->size;
        if (F1->pcur < F1->nbrfic - 1)
            F1->pcur++;
        else
        if (F1->pcur > 0)
            F1->pcur--;
    }

    fin = 0;

    if ((KKCfg->noprompt & 1) == 1)
        test = 0;
    else
        test = 1;

    for (i = 0; i < F1->nbrfic; i++) {
        F = F1->F[i];

        if ((F->select) == 1) {
            strcpy(inpath, F1->path);
            Path2Abs(inpath, F->name);

            fin = 0;

            if (test == 1)
                car = (char)FenDelete(inpath);
            else
                car = 0;

            switch (car) {
                case -1:                                // Touche ESCape
                case 3:
                    fin = 1;
                    break;
                case 1:                                 // Delete ALL
                    test = 0;
                case 0:                                 // Delete
                    switch (F1->system) {
                        case 0:
                            if (Erase(inpath, (F1->F[i])) == 0) {
                                F1->F[i]->select = 0;
                                F1->nbrsel--;
                                (F1->taillesel) -= F1->F[i]->size;
                                car = 7;
                            } else
                            if (DelErr(inpath) != 0)
                                fin = 1;
                            break;
                        default:
                            YouMad("Delete");
                            break;
                    }
                    break;
            } // switch
        }
        if (fin == 1) break;
    }

    LoadScreen();

    if (fin == 1) return 1;

    if (test == 0) return 4;

    if (car == 7) return 3;

    return 2;

} // Delete

// Retour:
// 1: Annuler tout
// 2: Le dernier n'a pas �t� �ffac�
// 3: Le dernier � �t� �ffac�
// 4: Efface tout
