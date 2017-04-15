/*  File src/changestat.h in package ergm, part of the Statnet suite
 *  of packages for network analysis, http://statnet.org .
 *
 *  This software is distributed under the GPL-3 license.  It is free,
 *  open source, and has the attribution requirements (GPL Section 7) at
 *  http://statnet.org/attribution
 *
 *  Copyright 2003-2013 Statnet Commons
 */
#ifndef CHANGESTAT_H
#define CHANGESTAT_H

#include "edgetree.h"
#include "edgelist.h"

typedef struct ModelTermstruct {
  void (*c_func)(Vertex, Vertex, struct ModelTermstruct*, Network*);
  void (*d_func)(Edge, Vertex*, Vertex*, struct ModelTermstruct*, Network*);
  void (*i_func)(struct ModelTermstruct*, Network*);
  void (*u_func)(Vertex, Vertex, struct ModelTermstruct*, Network*);
  void (*f_func)(struct ModelTermstruct*, Network*);
  void (*s_func)(struct ModelTermstruct*, Network*);
  double *attrib; /* Ptr to vector of covariates (if necessary; generally unused) */
  int nstats;   /* Number of change statistics to be returned */
  double *dstats; /* ptr to change statistics returned */
  int ninputparams; /* Number of input parameters passed to function */
  double *inputparams; /* ptr to input parameters passed */
  double *statcache; /* vector of the same length as dstats */
  void *storage; /* optional space for persistent storage */
  void **aux_storage; /* optional space for persistent public (auxiliary) storage */
} ModelTerm;

#include "changestat_common.inc"

/****************************************************
 Macros to make life easier when writing C code for change statistics:  */

/* return number of tail and head node in the directed node pair
   tail -> head of the selected toggle */
#define TAIL(a) (tails[(a)])
#define HEAD(a) (heads[(a)])

/* tell whether a particular edge exists */
#define IS_OUTEDGE(a,b) (EdgetreeSearch((a),(b),nwp->outedges)!=0?1:0)
#define IS_INEDGE(a,b) (EdgetreeSearch((a),(b),nwp->inedges)!=0?1:0)
#define IS_UNDIRECTED_EDGE(a,b) IS_OUTEDGE(MIN(a,b), MAX(a,b))

/* Return the Edge number of the smallest-labelled neighbor of the node 
   labelled "a".  Or, return the Edge number of the next-largest neighbor 
   starting from the pointer "e", which points to a node in an edgetree. 
   Mostly, these are utility macros used by the STEP_THROUGH_OUTEDGES 
   and STEP_THROUGH_INEDGES macros. */
#define MIN_OUTEDGE(a) (EdgetreeMinimum(nwp->outedges, (a)))
#define MIN_INEDGE(a) (EdgetreeMinimum(nwp->inedges, (a)))
#define NEXT_OUTEDGE(e) (EdgetreeSuccessor(nwp->outedges,(e)))
#define NEXT_INEDGE(e) (EdgetreeSuccessor(nwp->inedges,(e)))

/* Change the status of the (a,b) edge:  Add it if it's absent, or 
   delete it if it's present. */
#define TOGGLE(a,b) (ToggleEdge((a),(b),nwp));
#define TOGGLE_DISCORD(a,b) (ToggleEdge((a),(b),nwp+1));


/* *** don't forget tail-> head, so these functions now toggle (tails, heads), instead of (heads, tails) */

#define FOR_EACH_TOGGLE(a) for((a)=0; (a)<ntoggles; (a)++)
#define IF_MORE_TO_COME(a) if((a)+1<ntoggles)
#define TOGGLE_IF_MORE_TO_COME(a) IF_MORE_TO_COME(a){TOGGLE(tails[(a)],heads[(a)])}
#define TOGGLE_DISCORD_IF_MORE_TO_COME(a) IF_MORE_TO_COME(a){TOGGLE_DISCORD(tails[(a)],heads[(a)])}
#define UNDO_PREVIOUS(a) (a)--; while(--(a)>=0)
#define UNDO_PREVIOUS_TOGGLES(a)  UNDO_PREVIOUS(a){TOGGLE(tails[(a)],heads[(a)])}
#define UNDO_PREVIOUS_DISCORD_TOGGLES(a) UNDO_PREVIOUS(a){TOGGLE(tails[(a)],heads[(a)]); TOGGLE_DISCORD(tails[(a)],heads[(a)])}

/****************************************************/
/* changestat function prototypes */

/* *** don't forget tail -> head, so this prototype now accepts tails first, not heads first */

#define CHANGESTAT_FN(a) void (a) (Edge ntoggles, Vertex *tails, Vertex *heads, ModelTerm *mtp, Network *nwp)

/* NB:  CHANGESTAT_FN is now deprecated (replaced by D_CHANGESTAT_FN) */
#define C_CHANGESTAT_FN(a) void (a) (Vertex tail, Vertex head, ModelTerm *mtp, Network *nwp)
#define D_CHANGESTAT_FN(a) void (a) (Edge ntoggles, Vertex *tails, Vertex *heads, ModelTerm *mtp, Network *nwp)
#define I_CHANGESTAT_FN(a) void (a) (ModelTerm *mtp, Network *nwp)
#define U_CHANGESTAT_FN(a) void (a) (Vertex tail, Vertex head, ModelTerm *mtp, Network *nwp)
#define F_CHANGESTAT_FN(a) void (a) (ModelTerm *mtp, Network *nwp)
#define S_CHANGESTAT_FN(a) void (a) (ModelTerm *mtp, Network *nwp)

/* This macro wraps two calls to an s_??? function with toggles
   between them. */
#define D_FROM_S							\
  {									\
    (*(mtp->s_func))(mtp, nwp);  /* Call s_??? function */		\
    memcpy(mtp->statcache,mtp->dstats,N_CHANGE_STATS*sizeof(double));	\
    /* Note: This cannot be abstracted into EXEC_THROUGH_TOGGLES. */	\
    int j;								\
    FOR_EACH_TOGGLE(j) TOGGLE(TAIL(j),HEAD(j));				\
    (*(mtp->s_func))(mtp, nwp);  /* Call s_??? function */		\
    for(unsigned int i=0; i<N_CHANGE_STATS; i++)			\
      mtp->dstats[i] -= mtp->statcache[i];				\
    FOR_EACH_TOGGLE(j) TOGGLE(TAIL(j),HEAD(j));				\
  }

/* This macro constructs a function that wraps D_FROM_S. */
#define D_FROM_S_FN(a) D_CHANGESTAT_FN(a) D_FROM_S

#endif