
/* Computation of FIRST stets */

#include "pgenheaders.h"
#include "grammar.h"
#include "token.h"

extern int Py_DebugFlag;

/* Forward */
static void calcfirstset(grammar *, dfa *);

void
addfirstsets(grammar *g)
{
    int i;
    dfa *d;

    if (Py_DebugFlag)
        printf("Adding FIRST sets ...\n");
    for (i = 0; i < g->g_ndfas; i++) {
        d = &g->g_dfa[i];
        if (d->d_first == NULL)
            calcfirstset(g, d);
    }
}

static void
calcfirstset(grammar *g, dfa *d)
{
    int i, j;
    state *s;
    arc *a;
    int nsyms;
    int *sym;
    int nbits;
    static bitset dummy;
    bitset result;
    int type;
    dfa *d1;
    label *l0;

    if (Py_DebugFlag)
        printf("Calculate FIRST set for '%s'\n", d->d_name);

    if (dummy == NULL)
        dummy = newbitset(1);
    
    /* 这种情况可能会发生 比如single_input的first集中又一个边经过test 
     * 但是test的first集中 又一个边也经过了single_input 但是底下的递归调用前已经做出了判断 必须为null才能递归调用 那么这边应该不会存在
     * d->first不是null的情况  */
    if (d->d_first == dummy) {
        fprintf(stderr, "Left-recursion for '%s'\n", d->d_name);
        return;
    }
    /* 上面的addfirstsets已经判断了当d_first不等于null的时候就 不再计算这个dfa的first集合
     * 那么这里的计算是什么意思   */
    if (d->d_first != NULL) {
        fprintf(stderr, "Re-calculating FIRST set for '%s' ???\n",
            d->d_name);
    }
    d->d_first = dummy;

    l0 = g->g_ll.ll_label;
    nbits = g->g_ll.ll_nlabels;
    result = newbitset(nbits);

    sym = (int *)PyObject_MALLOC(sizeof(int));
    if (sym == NULL)
        Py_FatalError("no mem for new sym in calcfirstset");
    nsyms = 1;
    sym[0] = findlabel(&g->g_ll, d->d_type, (char *)NULL);

    s = &d->d_state[d->d_initial];
    for (i = 0; i < s->s_narcs; i++) {
        a = &s->s_arc[i];
        for (j = 0; j < nsyms; j++) {
            if (sym[j] == a->a_lbl)
                break;
        }
        if (j >= nsyms) { /* New label */
            sym = (int *)PyObject_REALLOC(sym,
                                    sizeof(int) * (nsyms + 1));
            if (sym == NULL)
                Py_FatalError(
                    "no mem to resize sym in calcfirstset");
            sym[nsyms++] = a->a_lbl;
            type = l0[a->a_lbl].lb_type;
            if (ISNONTERMINAL(type)) {
                d1 = PyGrammar_FindDFA(g, type);
                if (d1->d_first == dummy) {
                    fprintf(stderr,
                        "Left-recursion below '%s'\n",
                        d->d_name);
                }
                else {
                    if (d1->d_first == NULL)
                        calcfirstset(g, d1);
                    mergebitset(result,
                                d1->d_first, nbits);
                }
            }
            else if (ISTERMINAL(type)) {
                addbit(result, a->a_lbl);
            }
        }
    }
    d->d_first = result;
    if (Py_DebugFlag) {
        printf("FIRST set for '%s': {", d->d_name);
        for (i = 0; i < nbits; i++) {
            if (testbit(result, i))
                printf(" %s", PyGrammar_LabelRepr(&l0[i]));
        }
        printf(" }\n");
    }

    PyObject_FREE(sym);
}
