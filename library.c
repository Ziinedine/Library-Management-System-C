#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ===== Global state ===== */
int SLOTS;
library_t library;

/* System Stats  */
static long g_book_count = 0;
static long g_member_count = 0;
static long g_active_loans = 0;
static long g_total_scores = 0;
static long g_total_reviews = 0;


   /* pida whitespace */
static char* skip_ws(char* s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

/*  next_token: opos sto Phase 1  */
static char* next_token(char** p) {
    char* s = skip_ws(*p);

    if (*s == '\0' || *s == '\n') {
        *p = s;
        return NULL;
    }

    char* tok;

    if (*s == '"') {
        s++;
        tok = s;
        while (*s && *s != '"') {
            s++;
        }
        if (*s == '"') {
            *s = '\0';
            s++;
        }
    }
    else {
        tok = s;
        while (*s && !isspace((unsigned char)*s)) {
            s++;
        }
        if (*s) {
            *s = '\0';
            s++;
        }
    }

    *p = s;
    return tok;
}

/* malloc me elegxo  */
static void* xmalloc(size_t n) {
    void* p = malloc(n);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

/* ===========================
 *   AVL BookIndex helpers
 * =========================== */

static int avl_height(BookNode* n) {
    return n ? n->height : 0;
}

static int avl_max(int a, int b) {
    return (a > b) ? a : b;
}

static void avl_update_height(BookNode* n) {
    if (n)
        n->height = 1 + avl_max(avl_height(n->lc), avl_height(n->rc));
}

static BookNode* avl_rotate_right(BookNode* y) {
    BookNode* x = y->lc;
    BookNode* T2 = x->rc;

    x->rc = y;
    y->lc = T2;

    avl_update_height(y);
    avl_update_height(x);
    return x;
}

static BookNode* avl_rotate_left(BookNode* x) {
    BookNode* y = x->rc;
    BookNode* T2 = y->lc;

    y->lc = x;
    x->rc = T2;

    avl_update_height(x);
    avl_update_height(y);
    return y;
}

static int avl_balance_factor(BookNode* n) {
    if (!n) return 0;
    return avl_height(n->lc) - avl_height(n->rc);
}

static BookNode* avl_new_node(book_t* book) {
    BookNode* node = (BookNode*)xmalloc(sizeof(BookNode));
    strncpy(node->title, book->title, TITLE_MAX - 1);
    node->title[TITLE_MAX - 1] = '\0';
    node->book = book;
    node->lc = node->rc = NULL;
    node->height = 1;
    return node;
}

static BookNode* avl_insert_node(BookNode* node, book_t* book, int* inserted) {
    if (!node) {
        *inserted = 1;
        return avl_new_node(book);
    }
    int cmp = strcmp(book->title, node->title);
    if (cmp == 0) {
        *inserted = 0;   /* unique titles only */
        return node;
    }
    else if (cmp < 0) {
        node->lc = avl_insert_node(node->lc, book, inserted);
    }
    else {
        node->rc = avl_insert_node(node->rc, book, inserted);
    }

    avl_update_height(node);
    int balance = avl_balance_factor(node);

    if (balance > 1 && strcmp(book->title, node->lc->title) < 0)
        return avl_rotate_right(node);
    if (balance < -1 && strcmp(book->title, node->rc->title) > 0)
        return avl_rotate_left(node);
    if (balance > 1 && strcmp(book->title, node->lc->title) > 0) {
        node->lc = avl_rotate_left(node->lc);
        return avl_rotate_right(node);
    }
    if (balance < -1 && strcmp(book->title, node->rc->title) < 0) {
        node->rc = avl_rotate_right(node->rc);
        return avl_rotate_left(node);
    }
    return node;
}

static void avl_insert(BookNode** rootp, book_t* book) {
    int inserted = 0;
    *rootp = avl_insert_node(*rootp, book, &inserted);
}

static BookNode* avl_search(BookNode* root, const char* title) {
    while (root) {
        int cmp = strcmp(title, root->title);
        if (cmp == 0) return root;
        else if (cmp < 0) root = root->lc;
        else root = root->rc;
    }
    return NULL;
}

static BookNode* avl_min_node(BookNode* node) {
    BookNode* cur = node;
    while (cur && cur->lc) cur = cur->lc;
    return cur;
}

static BookNode* avl_delete_node(BookNode* root, const char* title) {
    if (!root) return NULL;

    int cmp = strcmp(title, root->title);
    if (cmp < 0) {
        root->lc = avl_delete_node(root->lc, title);
    }
    else if (cmp > 0) {
        root->rc = avl_delete_node(root->rc, title);
    }
    else {
        if (!root->lc || !root->rc) {
            BookNode* tmp = root->lc ? root->lc : root->rc;
            free(root);
            return tmp;
        }
        else {
            BookNode* tmp = avl_min_node(root->rc);
            strncpy(root->title, tmp->title, TITLE_MAX - 1);
            root->title[TITLE_MAX - 1] = '\0';
            root->book = tmp->book;
            root->rc = avl_delete_node(root->rc, tmp->title);
        }
    }

    avl_update_height(root);
    int balance = avl_balance_factor(root);

    if (balance > 1 && avl_balance_factor(root->lc) >= 0)
        return avl_rotate_right(root);
    if (balance > 1 && avl_balance_factor(root->lc) < 0) {
        root->lc = avl_rotate_left(root->lc);
        return avl_rotate_right(root);
    }
    if (balance < -1 && avl_balance_factor(root->rc) <= 0)
        return avl_rotate_left(root);
    if (balance < -1 && avl_balance_factor(root->rc) > 0) {
        root->rc = avl_rotate_right(root->rc);
        return avl_rotate_left(root);
    }

    return root;
}

static void avl_delete(BookNode** rootp, const char* title) {
    *rootp = avl_delete_node(*rootp, title);
}

static void avl_free(BookNode* root) {
    if (!root) return;
    avl_free(root->lc);
    avl_free(root->rc);
    free(root);
}

/* euresi bibliou apo titlo se ola ta  genres */
static book_t* find_book_by_title(const char* title) {
    for (genre_t* g = library.genres; g; g = g->next) {
        BookNode* n = avl_search(g->bookIndex, title);
        if (n) return n->book;
    }
    return NULL;
}

/* ===========================
 *  Recommendation Heap
 * =========================== */

static int book_better(const book_t* a, const book_t* b) {
    if (a->avg != b->avg) return (a->avg > b->avg);
    return (a->bid < b->bid); /* mikrotero bid kalitero */
}

static void recheap_swap(RecHeap* h, int i, int j) {
    book_t* tmp = h->heap[i];
    h->heap[i] = h->heap[j];
    h->heap[j] = tmp;

    if (h->heap[i]) h->heap[i]->heap_pos = i;
    if (h->heap[j]) h->heap[j]->heap_pos = j;
}

static void recheap_heapify_up(RecHeap* h, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (!book_better(h->heap[idx], h->heap[parent]))
            break;
        recheap_swap(h, idx, parent);
        idx = parent;
    }
}

static void recheap_heapify_down(RecHeap* h, int idx) {
    int n = h->size;
    for (;;) {
        int left = 2 * idx + 1;
        int right = 2 * idx + 2;
        int best = idx;

        if (left < n && book_better(h->heap[left], h->heap[best]))
            best = left;
        if (right < n && book_better(h->heap[right], h->heap[best]))
            best = right;
        if (best == idx) break;

        recheap_swap(h, idx, best);
        idx = best;
    }
}

static RecHeap* recheap_create(int capacity) {
    RecHeap* h = (RecHeap*)xmalloc(sizeof(RecHeap));
    h->heap = (book_t**)xmalloc(sizeof(book_t*) * capacity);
    h->size = 0;
    h->capacity = capacity;
    for (int i = 0; i < capacity; i++) h->heap[i] = NULL;
    return h;
}

static void recheap_destroy(RecHeap* h) {
    if (!h) return;
    free(h->heap);
    free(h);
}

static void recheap_remove(RecHeap* h, book_t* b) {
    if (!h || !b) return;
    int pos = b->heap_pos;
    if (pos < 0 || pos >= h->size) return;

    int last = h->size - 1;
    recheap_swap(h, pos, last);
    h->size--;
    b->heap_pos = -1;

    if (pos < h->size) {
        recheap_heapify_up(h, pos);
        recheap_heapify_down(h, pos);
    }
}

/* insert/update bibliou sto heap meta apo allagh avg*/
static void recheap_update(RecHeap* h, book_t* b) {
    if (!h || !b) return;

    /* xoris reviews or lost-> den prepei na einai sto heap*/
    if (b->n_reviews == 0 || b->lost_flag) {
        recheap_remove(h, b);
        return;
    }

    if (b->heap_pos >= 0 && b->heap_pos < h->size) {
        recheap_heapify_up(h, b->heap_pos);
        recheap_heapify_down(h, b->heap_pos);
        return;
    }

    if (h->size < h->capacity) {
        int pos = h->size++;
        h->heap[pos] = b;
        b->heap_pos = pos;
        recheap_heapify_up(h, pos);
        return;
    }

    /* capacity gemato:vres to xeirotero(grammika, capacity=64) */
    int worst_idx = 0;
    book_t* worst = h->heap[0];
    for (int i = 1; i < h->size; i++) {
        book_t* cur = h->heap[i];
        if (!cur) continue;

        /*theloume xeirotero */
        if (book_better(cur, worst)) {
            /* cur kalitero apo worst => worst menei xeirotero*/
            continue;
        }
        if (book_better(worst, cur)) {
            worst = cur;
            worst_idx = i;
        }
        else {
            /* otan isa kratao auto me to megalitero bid os xeirotero*/
            if (cur->avg == worst->avg && cur->bid > worst->bid) {
                worst = cur;
                worst_idx = i;
            }
        }
    }

    if (!book_better(b, worst))
        return; /* neo biblio den einai kalitero */

    worst->heap_pos = -1;
    h->heap[worst_idx] = b;
    b->heap_pos = worst_idx;
    recheap_heapify_up(h, worst_idx);
    recheap_heapify_down(h, worst_idx);
}

/* ===========================
 *  MemberActivity helpers
 * =========================== */

static MemberActivity* activity_new(int sid) {
    MemberActivity* a = (MemberActivity*)xmalloc(sizeof(MemberActivity));
    a->sid = sid;
    a->loans_count = 0;
    a->reviews_count = 0;
    a->score_sum = 0;
    a->next = NULL;
    return a;
}

/* Uns. singly-linked: push-front */
static void activity_add(MemberActivity* a) {
    a->next = library.activity;
    library.activity = a;
}

static void activity_free_all(void) {
    MemberActivity* cur = library.activity;
    while (cur) {
        MemberActivity* nx = cur->next;
        free(cur);
        cur = nx;
    }
    library.activity = NULL;
}

/* ===========================
   entopismos /dimiourgia kombon
   =========================== */

static genre_t* find_genre(int gid) {
    for (genre_t* g = library.genres; g; g = g->next) {
        if (g->gid == gid) return g;
    }
    return NULL;
}

static member_t* find_member(int sid) {
    for (member_t* m = library.members; m; m = m->next) {
        if (m->sid == sid) return m;
    }
    return NULL;
}

static book_t* find_book_global(int bid, genre_t** owner) {
    for (genre_t* g = library.genres; g; g = g->next) {
        for (book_t* b = g->books; b; b = b->next) {
            if (b->bid == bid) {
                if (owner) *owner = g;
                return b;
            }
        }
    }
    if (owner) *owner = NULL;
    return NULL;
}

static genre_t* genre_new(int gid, const char* name) {
    genre_t* g = (genre_t*)xmalloc(sizeof(genre_t));
    g->gid = gid;
    strncpy(g->name, name ? name : "", NAME_MAX - 1);
    g->name[NAME_MAX - 1] = '\0';

    g->books = NULL;
    g->lost_count = 0;
    g->invalid_count = 0;
    g->slots = 0;
    g->display = NULL;
    g->bookIndex = NULL;
    g->next = NULL;
    return g;
}

static member_t* member_new(int sid, const char* name) {
    member_t* m = (member_t*)xmalloc(sizeof(member_t));
    m->sid = sid;
    strncpy(m->name, name ? name : "", NAME_MAX - 1);
    m->name[NAME_MAX - 1] = '\0';

    loan_t* sent = (loan_t*)xmalloc(sizeof(loan_t));
    sent->sid = sid;
    sent->bid = -1;
    sent->next = NULL;
    m->loans = sent;

    m->activity = NULL;
    m->next = NULL;
    return m;
}

static book_t* book_new(int bid, int gid, const char* title) {
    book_t* b = (book_t*)xmalloc(sizeof(book_t));
    b->bid = bid;
    b->gid = gid;
    strncpy(b->title, title ? title : "", TITLE_MAX - 1);
    b->title[TITLE_MAX - 1] = '\0';

    b->sum_scores = 0;
    b->n_reviews = 0;
    b->avg = 0;
    b->lost_flag = 0;
    b->heap_pos = -1;
    b->prev = NULL;
    b->next = NULL;
    return b;
}

/* ===========================
   Free helpers
   =========================== */

static void free_loans(loan_t* sent) {
    if (!sent) return;
    loan_t* c = sent->next;
    while (c) {
        loan_t* nx = c->next;
        free(c);
        c = nx;
    }
    free(sent);
}

static void free_books(book_t* h) {
    while (h) {
        book_t* nx = h->next;
        free(h);
        h = nx;
    }
}

static void free_display(genre_t* g) {
    if (g->display) {
        free(g->display);
        g->display = NULL;
    }
    g->slots = 0;
}

static void clear_display_all(void) {
    for (genre_t* g = library.genres; g; g = g->next) {
        free_display(g);
    }
}

/* ===========================
   Taxinomimenes Listes
   =========================== */

   /* insert genre se lista library.genres tazinomimenes kata auxousa  */
static int insert_genre_sorted(genre_t* ng) {
    if (!library.genres || ng->gid < library.genres->gid) {
        ng->next = library.genres;
        library.genres = ng;
        return 1;
    }
    genre_t* pr = library.genres;
    genre_t* c = library.genres->next;
    while (c && c->gid < ng->gid) {
        pr = c;
        c = c->next;
    }
    if (c && c->gid == ng->gid) {
        return 0; /* iparxei idi  */
    }
    pr->next = ng;
    ng->next = c;
    return 1;
}

/* insert member se list library.members taxinomisi kata sid (auxousa) */
static int insert_member_sorted(member_t* nm) {
    if (!library.members || nm->sid < library.members->sid) {
        nm->next = library.members;
        library.members = nm;
        return 1;
    }
    member_t* pr = library.members;
    member_t* c = library.members->next;
    while (c && c->sid < nm->sid) {
        pr = c;
        c = c->next;
    }
    if (c && c->sid == nm->sid) {
        return 0;
    }
    pr->next = nm;
    nm->next = c;
    return 1;
}

/*ta biblia se kathe genre DLL
 *  - avg fthinousa
 *  - se isobathmia, bid auksousa 
 */
static void book_insert_sorted(genre_t* g, book_t* nb) {
    if (!g->books) {
        g->books = nb;
        nb->prev = nb->next = NULL;
        return;
    }

    book_t* cur = g->books;
    book_t* prev = NULL;

    while (cur) {
        if (nb->avg > cur->avg) break;
        if (nb->avg == cur->avg && nb->bid < cur->bid) break;
        prev = cur;
        cur = cur->next;
    }

    nb->prev = prev;
    nb->next = cur;

    if (prev) {
        prev->next = nb;
    }
    else {
        g->books = nb;
    }
    if (cur) {
        cur->prev = nb;
    }
}

/* relocation meta apo enimerosi agv */
static void book_relocate(genre_t* g, book_t* b) {
    /* move up */
    while (b->prev &&
        (b->prev->avg < b->avg ||
            (b->prev->avg == b->avg && b->prev->bid > b->bid))) {

        book_t* p = b->prev;
        book_t* pp = p->prev;
        book_t* n = b->next;

        if (pp) pp->next = b;
        else    g->books = b;
        b->prev = pp;

        b->next = p;
        p->prev = b;

        p->next = n;
        if (n) n->prev = p;
    }

    /* move down */
    while (b->next &&
        (b->next->avg > b->avg ||
            (b->next->avg == b->avg && b->next->bid < b->bid))) {

        book_t* n = b->next;
        book_t* nn = n->next;
        book_t* p = b->prev;

        if (p) p->next = n;
        else   g->books = n;
        n->prev = p;

        n->next = b;
        b->prev = n;

        b->next = nn;
        if (nn) nn->prev = b;
    }
}

/* ===========================
   Daneismoi
   =========================== */

static int loan_exists(member_t* m, int bid) {
    for (loan_t* c = m->loans->next; c; c = c->next) {
        if (c->bid == bid) return 1;
    }
    return 0;
}

static void loan_add(member_t* m, int bid) {
    loan_t* node = (loan_t*)xmalloc(sizeof(loan_t));
    node->sid = m->sid;
    node->bid = bid;
    node->next = m->loans->next;
    m->loans->next = node;
}

/* afairesi daneismou. epistrefei 1 an afairethike */
static int loan_remove(member_t* m, int bid) {
    loan_t* pr = m->loans;
    loan_t* c = pr->next;

    while (c) {
        if (c->bid == bid) {
            pr->next = c->next;
            free(c);
            return 1;
        }
        pr = c;
        c = c->next;
    }
    return 0;
}

/* ===========================
   Lifecycle bibliothikis
   =========================== */

void lib_init(void) {
    library.genres = NULL;
    library.members = NULL;
    library.activity = NULL;
    library.recommendations = recheap_create(REC_CAPACITY);

    SLOTS = 0;

    g_book_count = 0;
    g_member_count = 0;
    g_active_loans = 0;
    g_total_scores = 0;
    g_total_reviews = 0;
}

void lib_destroy(void) {
    clear_display_all();

    genre_t* g = library.genres;
    while (g) {
        genre_t* gn = g->next;
        free_display(g);
        free_books(g->books);
        avl_free(g->bookIndex);
        free(g);
        g = gn;
    }
    library.genres = NULL;

    member_t* m = library.members;
    while (m) {
        member_t* mn = m->next;
        free_loans(m->loans);
        free(m);
        m = mn;
    }
    library.members = NULL;

    activity_free_all();

    if (library.recommendations) {
        recheap_destroy(library.recommendations);
        library.recommendations = NULL;
    }
}

/* ===========================
   Handlers entolon 
   =========================== */

void handle_S(char* args) {
    char* p = args;
    char* t = next_token(&p);
    if (!t) {
        printf("IGNORED\n");
        return;
    }

    int v = atoi(t);
    if (v < 0) v = 0;
    SLOTS = v;

    printf("DONE\n");
}

void handle_G(char* args) {
    char* p = args;
    char* tgid = next_token(&p);
    char* tname = next_token(&p);

    if (!tgid || !tname) {
        printf("IGNORED\n");
        return;
    }

    int gid = atoi(tgid);

    if (find_genre(gid)) {
        printf("IGNORED\n");
        return;
    }

    genre_t* g = genre_new(gid, tname);
    insert_genre_sorted(g);

    printf("DONE\n");
}

void handle_BK(char* args) {
    char* p = args;
    char* tbid = next_token(&p);
    char* tgid = next_token(&p);
    char* ttitle = next_token(&p);

    if (!tbid || !tgid || !ttitle) {
        printf("IGNORED\n");
        return;
    }

    int bid = atoi(tbid);
    int gid = atoi(tgid);

    genre_t* g = find_genre(gid);
    if (!g) {
        printf("IGNORED\n");
        return;
    }

    if (find_book_global(bid, NULL)) {
        printf("IGNORED\n");
        return;
    }

    /* monadikos titlos se oli tin bibliothiki */
    if (find_book_by_title(ttitle)) {
        printf("IGNORED\n");
        return;
    }

    book_t* b = book_new(bid, gid, ttitle);
    book_insert_sorted(g, b);
    avl_insert(&g->bookIndex, b);

    g_book_count++;

    printf("DONE\n");
}

void handle_M(char* args) {
    char* p = args;
    char* tsid = next_token(&p);
    char* tname = next_token(&p);

    if (!tsid || !tname) {
        printf("IGNORED\n");
        return;
    }

    int sid = atoi(tsid);

    if (find_member(sid)) {
        printf("IGNORED\n");
        return;
    }

    member_t* m = member_new(sid, tname);
    insert_member_sorted(m);

    MemberActivity* a = activity_new(sid);
    activity_add(a);
    m->activity = a;

    g_member_count++;

    printf("DONE\n");
}

void handle_L(char* args) {
    char* p = args;
    char* tsid = next_token(&p);
    char* tbid = next_token(&p);

    if (!tsid || !tbid) {
        printf("IGNORED\n");
        return;
    }

    int sid = atoi(tsid);
    int bid = atoi(tbid);

    member_t* m = find_member(sid);
    if (!m) {
        printf("IGNORED\n");
        return;
    }

    if (!find_book_global(bid, NULL)) {
        printf("IGNORED\n");
        return;
    }

    if (loan_exists(m, bid)) {
        printf("IGNORED\n");
        return;
    }

    loan_add(m, bid);

    if (m->activity) {
        m->activity->loans_count += 1;
    }
    g_active_loans++;

    printf("DONE\n");
}

/* handle_R Phase 2 */
void handle_R(char* args) {
    char* p = args;
    char* tsid = next_token(&p);
    char* tbid = next_token(&p);
    char* tscore = next_token(&p);
    char* tstatus = next_token(&p);

    if (!tsid || !tbid || !tscore || !tstatus) {
        printf("IGNORED\n");
        return;
    }

    int sid = atoi(tsid);
    int bid = atoi(tbid);

    member_t* m = find_member(sid);
    if (!m) {
        printf("IGNORED\n");
        return;
    }

    genre_t* g_owner = NULL;
    book_t* bk = find_book_global(bid, &g_owner);
    if (!bk) {
        printf("IGNORED\n");
        return;
    }

    if (!loan_exists(m, bid)) {
        printf("IGNORED\n");
        return;
    }

    /* lost */
    if (strcmp(tstatus, "lost") == 0) {
        loan_remove(m, bid);
        g_active_loans--;

        bk->lost_flag = 1;
        if (g_owner) {
            g_owner->lost_count++;
            avl_delete(&g_owner->bookIndex, bk->title);
        }

        recheap_remove(library.recommendations, bk);

        printf("DONE\n");
        return;
    }

    /* status must be ok */
    if (strcmp(tstatus, "ok") != 0) {
        printf("IGNORED\n");
        return;
    }

    /* ok + NA */
    if (strcmp(tscore, "NA") == 0) {
        loan_remove(m, bid);
        g_active_loans--;
        printf("DONE\n");
        return;
    }

    /* numeric score */
    char* end = NULL;
    long sc = strtol(tscore, &end, 10);

    if (end && *end != '\0') {
        if (g_owner) g_owner->invalid_count++;
        printf("IGNORED\n");
        return;
    }

    if (sc < 0 || sc > 10) {
        if (g_owner) g_owner->invalid_count++;
        printf("IGNORED\n");
        return;
    }

    loan_remove(m, bid);
    g_active_loans--;

    bk->sum_scores += (int)sc;
    bk->n_reviews += 1;
    bk->avg = bk->sum_scores / bk->n_reviews;

    if (g_owner)
        book_relocate(g_owner, bk);

    if (m->activity) {
        m->activity->reviews_count += 1;
        m->activity->score_sum += sc;
    }

    g_total_scores += sc;
    g_total_reviews++;

    recheap_update(library.recommendations, bk);

    printf("DONE\n");
}

/* ===========================
   katanomi bitrinon (D)  
   =========================== */

typedef struct {
    long long points;
    long long rem;
    int       seats;
    genre_t* g;
} remrec_t;

/* Megalitera ipoloipa    - rem fthinousa    - tie break:mikrotero gid */
static int cmp_remdesc_gidasc(const void* a, const void* b) {
    const remrec_t* ra = (const remrec_t*)a;
    const remrec_t* rb = (const remrec_t*)b;

    if (ra->rem < rb->rem) return  1;
    if (ra->rem > rb->rem) return -1;
    return (ra->g->gid - rb->g->gid);
}

void handle_D(void) {
    clear_display_all();

    int G = 0;
    for (genre_t* g = library.genres; g; g = g->next) {
        G++;
    }

    if (G == 0) {
        printf("DONE\n");
        return;
    }

    remrec_t* A = (remrec_t*)xmalloc(sizeof(remrec_t) * G);

    long long VALID = 0;
    int idx = 0;
    for (genre_t* g = library.genres; g; g = g->next) {
        long long pts = 0;
        for (book_t* b = g->books; b; b = b->next) {
            if (!b->lost_flag && b->n_reviews > 0) {
                pts += b->sum_scores;
            }
        }
        A[idx].points = pts;
        A[idx].rem = 0;
        A[idx].seats = 0;
        A[idx].g = g;
        VALID += pts;
        idx++;
    }

    if (SLOTS <= 0) {
        free(A);
        printf("DONE\n");
        return;
    }

    long long quota = 0;
    if (VALID > 0) {
        quota = VALID / (long long)SLOTS;
    }
    else {
        quota = 0;
    }

    int allocated = 0;
    if (quota > 0) {
        for (int k = 0; k < G; k++) {
            A[k].seats = (int)(A[k].points / quota);
            allocated += A[k].seats;
            A[k].rem = A[k].points - ((long long)A[k].seats * quota);
        }
    }
    else {
        for (int k = 0; k < G; k++) {
            A[k].seats = 0;
            A[k].rem = A[k].points;
        }
    }

    int remaining = SLOTS - allocated;
    if (remaining < 0) remaining = 0;

    if (remaining > 0) {
        qsort(A, G, sizeof(remrec_t), cmp_remdesc_gidasc);
        for (int k = 0; k < G && remaining > 0; k++) {
            A[k].seats++;
            remaining--;
        }
    }

    for (int k = 0; k < G; k++) {
        genre_t* g = A[k].g;
        int need = A[k].seats;

        g->slots = need;
        g->display = NULL;

        if (need <= 0) {
            continue;
        }

        g->display = (book_t**)xmalloc(sizeof(book_t*) * need);

        int taken = 0;
        for (book_t* b = g->books; b && taken < need; b = b->next) {
            if (!b->lost_flag) {
                g->display[taken++] = b;
            }
        }

        g->slots = taken;
        if (taken == 0) {
            free(g->display);
            g->display = NULL;
        }
    }

    free(A);
    printf("DONE\n");
}

/* ===========================
   ektiposi katastasis
   =========================== */

void handle_PG(char* args) {
    char* p = args;
    char* tgid = next_token(&p);

    if (!tgid) {
        printf("IGNORED\n");
        return;
    }

    int gid = atoi(tgid);
    genre_t* g = find_genre(gid);
    if (!g) {
        printf("IGNORED\n");
        return;
    }

    for (book_t* b = g->books; b; b = b->next) {
        printf("%d, %d\n", b->bid, b->avg);
    }
}

void handle_PM(char* args) {
    char* p = args;
    char* tsid = next_token(&p);

    printf("Loans:\n");

    if (!tsid) {
        return;
    }

    int sid = atoi(tsid);
    member_t* m = find_member(sid);
    if (!m) {
        return;
    }

    for (loan_t* c = m->loans->next; c; c = c->next) {
        printf("%d\n", c->bid);
    }
}

void handle_PD(void) {
    int any = 0;
    for (genre_t* g = library.genres; g; g = g->next) {
        if (g->slots > 0 && g->display) {
            any = 1;
            break;
        }
    }

    printf("Display:\n");

    if (!any) {
        printf("(empty)\n");
        return;
    }

    for (genre_t* g = library.genres; g; g = g->next) {
        printf("%d:\n", g->gid);
        if (g->slots > 0 && g->display) {
            for (int i = 0; i < g->slots; i++) {
                book_t* b = g->display[i];
                if (b) {
                    printf("%d, %d\n", b->bid, b->avg);
                }
            }
        }
    }
}

void handle_PS(void) {
    printf("SLOTS=%d\n", SLOTS);

    for (genre_t* g = library.genres; g; g = g->next) {
        long long pts = 0;
        for (book_t* b = g->books; b; b = b->next) {
            if (!b->lost_flag && b->n_reviews > 0) {
                pts += b->sum_scores;
            }
        }
        printf("%d: points=%lld\n", g->gid, pts);
    }
}

   /* F <title> */
void handle_F(char* args) {
    char* p = args;
    char* ttitle = next_token(&p);

    if (!ttitle) {
        printf("NOT FOUND\n");
        return;
    }

    book_t* b = find_book_by_title(ttitle);
    if (!b) {
        printf("NOT FOUND\n");
        return;
    }

    printf("Book %d \"%s\" avg=%d\n", b->bid, b->title, b->avg);
}

/* TOP <k> */
void handle_TOP(char* args) {
    char* p = args;
    char* tk = next_token(&p);

    printf("Top Books:\n");

    if (!tk || !library.recommendations || library.recommendations->size == 0) {
        printf("(empty)\n");
        return;
    }

    int k = atoi(tk);
    RecHeap* h = library.recommendations;
    int n = h->size;

    if (k <= 0 || n == 0) {
        printf("(empty)\n");
        return;
    }
    if (k > n) k = n;

    int used[REC_CAPACITY] = { 0 };

    for (int printed = 0; printed < k; printed++) {
        int best_idx = -1;
        book_t* best = NULL;

        for (int i = 0; i < n; i++) {
            if (used[i]) continue;
            book_t* cur = h->heap[i];
            if (!cur) continue;
            if (!best || book_better(cur, best)) {
                best = cur;
                best_idx = i;
            }
        }

        if (best_idx == -1 || !best) break;
        used[best_idx] = 1;

        printf("%d \"%s\" avg=%d\n", best->bid, best->title, best->avg);
    }
}

/* AM*/
void handle_AM(void) {
    printf("Active Members:\n");

    if (!library.activity) {
        printf("NO ACTIVE MEMBERS\n");
        return;
    }

    for (MemberActivity* a = library.activity; a; a = a->next) {
        member_t* m = find_member(a->sid);
        const char* name = m ? m->name : "";
        printf("%d %s loans=%ld reviews=%ld\n",
            a->sid, name, a->loans_count, a->reviews_count);
    }
}

/* U <bid> "<new_title>" */
void handle_U(char* args) {
    char* p = args;
    char* tbid = next_token(&p);
    char* tnew = next_token(&p);

    if (!tbid || !tnew) {
        printf("IGNORED\n");
        return;
    }

    int bid = atoi(tbid);
    genre_t* g_owner = NULL;
    book_t* bk = find_book_global(bid, &g_owner);
    if (!bk || !g_owner) {
        printf("IGNORED\n");
        return;
    }

    /*an iparxei ki allo biblio me idio titlo -> IGNORED */
    book_t* other = find_book_by_title(tnew);
    if (other && other != bk) {
        printf("IGNORED\n");
        return;
    }

    /* remove apo BookIndex me ton palio titlo */
    avl_delete(&g_owner->bookIndex, bk->title);

    /* enimerosi titlou */
    strncpy(bk->title, tnew, TITLE_MAX - 1);
    bk->title[TITLE_MAX - 1] = '\0';

    /* re-insert BookIndex */
    avl_insert(&g_owner->bookIndex, bk);

    printf("DONE\n");
}

/* X*/
void handle_X(void) {
    printf("Stats:\n");
    printf("books=%ld\n", g_book_count);
    printf("members=%ld\n", g_member_count);
    printf("active_loans=%ld\n", g_active_loans);

    double avg_all = 0.0;
    if (g_total_reviews > 0)
        avg_all = (double)g_total_scores / (double)g_total_reviews;

    printf("all_score_avg=%.2f\n", avg_all);
}

/* BF */
void handle_BF(void) {
    lib_destroy();
    lib_init();
    printf("DONE\n");
}
