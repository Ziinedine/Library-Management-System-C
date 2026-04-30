#ifndef LIBRARY_H
#define LIBRARY_H

#include <stddef.h>

#define TITLE_MAX 128
#define NAME_MAX  64
#define REC_CAPACITY 64   /* capacity Recommendation Heap */

/*  Pagkosmia thesi provolon (orizetai apo tin entoli S <slots>)
    Frontiste na to kanete define sto antistoixo .c arxeio, ws
    int SLOTS;
*/
extern int SLOTS;

/* Forward declarations */
typedef struct loan  loan_t;
typedef struct book  book_t;
typedef struct member member_t;
typedef struct genre genre_t;

/* -----------------------------------------
   MemberActivity List (Phase 2)
   Singly-linked, unsorted
   ----------------------------------------- */
typedef struct member_activity {
    int  sid;             /* member id */
    long loans_count;     /* #loans from this member */
    long reviews_count;   /* #reviews from this member */
    long score_sum;       /* S(score) apo ola ta reviews */
    struct member_activity* next;
} MemberActivity;

/* -----------------------------------------
   Recommendation Heap (Phase 2)
   Max heap of book pointers
   ----------------------------------------- */
typedef struct rec_heap {
    book_t** heap;   /* pinakas deiktwn se books */
    int size;        /* # stoixeiwn */
    int capacity;    /* megisti xoritikotita */
} RecHeap;

/* -----------------------------------------
   BookIndex AVL node (Phase 2)
   - AVL tree taksinomimeno leksikografika kata title
   ----------------------------------------- */
typedef struct book_node {
    char  title[TITLE_MAX];
    book_t* book;           /* pointer sto antistoixo book */
    struct book_node* lc;   /* left child */
    struct book_node* rc;   /* right child */
    int height;             /* ypsos AVL */
} BookNode;

/* -----------------------------------------
   LOAN: energos daneismos (unsorted, O(1) insert/remove)
   Lista ana Member me xrhsh sentinel node.
   ----------------------------------------- */
struct loan {
    int sid;            /* member id (idioktitis tis listas) */
    int bid;            /* book id pou exei daneistei */
    struct loan* next;  /* epomenos daneismos tou melous */
};

/* -----------------------------------------
   BOOK: vivlio
   - Anikei se akrivos ena Genre (gid)
   - Symmetexei sti dipla syndedemeni lista tou Genre,
     taksinomimeni fthinonta kata avg.
   ----------------------------------------- */
struct book {
    int  bid;                         /* book id (monadiko) */
    int  gid;                         /* genre id (idioktisia listas) */
    char title[TITLE_MAX];

    /* Statistik a dimotikotitas */
    int sum_scores;                   /* athroisma egkyrwn vathmologiwn */
    int n_reviews;                    /* plithos egkyrwn vathmologiwn */
    int avg;                          /* cache: floor(sum_scores / n_reviews); 0 an n_reviews=0 */
    int lost_flag;                    /* 1 an dilomeno lost, alliws 0 */

    /* Thesi sto Recommendation Heap (Phase 2).
       -1 an den anikei sto heap. */
    int heap_pos;

    /* Dipla syndedemeni lista tou genre, taksinomimeni kata avg (desc). */
    struct book* prev;
    struct book* next;
};

/* -----------------------------------------
   MEMBER: melos vivliothikis
   - Krata unsorted lista energwn daneismwn (loan_t) me xrhsh sentinel node
   ----------------------------------------- */
struct member {
    int  sid;                         /* member id (monadiko) */
    char name[NAME_MAX];

    /* Lista energwn daneismwn:
       Uns. singly-linked me sentinel node:
       - Eisagwgi: O(1) push-front
       - Diagrafi gnwstou bid: O(1) an kratate prev pointer sti sarwsi */
    loan_t* loans;

    /* Drastiriotita melous (Phase 2) */
    MemberActivity* activity;

    /* Monosyndedemeni lista olwn twn melwn taksinomimeni kata sid */
    struct member* next;
};

/* -----------------------------------------
   GENRE: katigoria vivliwn
   - Krata DIPLA syndedemeni lista VIVLIWN taksinomimeni kata avg (desc)
   - Krata kai to apotelesma tis teleutaias D (display) gia ektipwsi PD
   - Krata BookIndex AVL gia anazitisi titlwn (Phase 2)
   ----------------------------------------- */
struct genre {
    int  gid;                         /* genre id (monadiko) */
    char name[NAME_MAX];

    /* dipla syndedemeni lista vivliwn taksinomimeni kata avg fthinousa. */
    book_t* books;
    int lost_count;
    int invalid_count;

    /* BookIndex AVL (riza) */
    BookNode* bookIndex;

    /* Apotelesma teleutaias katanomis D: epilegmena vivlia gia provoli.
       Apothikeuoume aplws pointers sta book_t (den antigrafoume dedomena). */
    int slots;            /* posa epilexthikan gia provoli se auto to genre */
    book_t** display;     /* dinamikos pinakas me pointers sta epilegmena vivlia gia provoli */

    /* Monosyndedemeni lista olwn twn genres taksinomimeni kata gid (gia eukoli sarwsi). */
    struct genre* next;
};

/* -----------------------------------------
   LIBRARY: kentrikos "rizas"
   Phase 2: prostithetai recommendations, activity
   ----------------------------------------- */
typedef struct library {
    RecHeap* recommendations;  /* Recommendation Heap (Max heap) */
    MemberActivity* activity;  /* MemberActivity list head */
    genre_t* genres;           /* kefali listas genres (sorted by gid) */
    member_t* members;         /* lista melwn (sorted by sid) */
} library_t;

/* Global library instance (orizetai sto Library.c) */
extern library_t library;

/* Phase 1 API */
void lib_init(void);
void lib_destroy(void);

void handle_S(char* args);
void handle_G(char* args);
void handle_BK(char* args);
void handle_M(char* args);
void handle_L(char* args);
void handle_R(char* args);
void handle_D(void);
void handle_PG(char* args);
void handle_PM(char* args);
void handle_PD(void);
void handle_PS(void);

/* Phase 2  */
void handle_F(char* args);
void handle_TOP(char* args);
void handle_AM(void);
void handle_U(char* args);
void handle_X(void);
void handle_BF(void);

    

#endif
