#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * main:
 *  - Αν δοθεί όνομα αρχείου: διαβάζει εντολές από αυτό.
 *  - Αλλιώς: διαβάζει από stdin.
 *  - Κάθε γραμμή: <COMMAND> [args...]
 *    π.χ. BK 10 1 "The Hobbit"
 */
int main(int argc, char** argv)
{
    lib_init();

    FILE* fp = NULL;
    if (argc >= 2) {
        fp = fopen(argv[1], "r");
        if (!fp) {
            perror("fopen");
            lib_destroy();
            return 1;
        }
    }
    else {
        fp = stdin;
    }

    char line[8192];

    while (fgets(line, sizeof(line), fp)) {
        char* s = line;

        /* paarleipo kena  */
        while (*s && isspace((unsigned char)*s)) {
            s++;
        }

        
        if (*s == '\0' || *s == '\n' || *s == '#') {
            continue;
        }

        
        char cmd[16];
        int i = 0;
        while (*s && !isspace((unsigned char)*s) && i < (int)sizeof(cmd) - 1) {
            cmd[i++] = (char)toupper((unsigned char)*s);
            s++;
        }
        cmd[i] = '\0';

        /* oti menei einai arguments */
        char* args = s;

        /* ---- ektiposi tou "CMD args -> */
        char fullcmd[8192];

        /* katharismaargs mono gia ektiposi (den peirazoume to args pou
           θα diabazei ο parser me next_token, pera apo to kopsimo  \n) */
        char* a = args;
        while (*a && isspace((unsigned char)*a)) {
            a++;
        }
        char* end = a + strlen(a);
        while (end > a && (end[-1] == '\n' || end[-1] == '\r' ||
            isspace((unsigned char)end[-1]))) {
            end--;
        }
        *end = '\0';

        if (*a) {
            /* exoume CMD + arguments */
            snprintf(fullcmd, sizeof(fullcmd), "%s %s", cmd, a);
        }
        else {
            /* mono i entoli (p.x. D, PD, PS, AM, X, BF) */
            snprintf(fullcmd, sizeof(fullcmd), "%s", cmd);
        }

        /* ektiposi "CMD args -> ", o handler tiponeiDONE/IGNORED/... */
        printf("%s -> ", fullcmd);
        fflush(stdout);
        

        /* Phase 1 entoles */
        if (strcmp(cmd, "S") == 0)  handle_S(args);
        else if (strcmp(cmd, "G") == 0)  handle_G(args);
        else if (strcmp(cmd, "BK") == 0)  handle_BK(args);
        else if (strcmp(cmd, "M") == 0)  handle_M(args);
        else if (strcmp(cmd, "L") == 0)  handle_L(args);
        else if (strcmp(cmd, "R") == 0)  handle_R(args);
        else if (strcmp(cmd, "D") == 0)  handle_D();
        else if (strcmp(cmd, "PG") == 0)  handle_PG(args);
        else if (strcmp(cmd, "PM") == 0)  handle_PM(args);
        else if (strcmp(cmd, "PD") == 0)  handle_PD();
        else if (strcmp(cmd, "PS") == 0)  handle_PS();

        /* Phase 2 entoles */
        else if (strcmp(cmd, "F") == 0)  handle_F(args);
        else if (strcmp(cmd, "TOP") == 0)  handle_TOP(args);
        else if (strcmp(cmd, "AM") == 0)  handle_AM();
        else if (strcmp(cmd, "U") == 0)  handle_U(args);
        else if (strcmp(cmd, "X") == 0)  handle_X();
        else if (strcmp(cmd, "BF") == 0)  handle_BF();

        else {
            /* agnoisi entolis*/
            printf("IGNORED\n");
        }
    }

    if (fp != stdin) {
        fclose(fp);
    }

    lib_destroy();
    return 0;
}
