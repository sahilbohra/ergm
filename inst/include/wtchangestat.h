/*  File src/wtchangestat.h in package ergm, part of the Statnet suite
 *  of packages for network analysis, http://statnet.org .
 *
 *  This software is distributed under the GPL-3 license.  It is free,
 *  open source, and has the attribution requirements (GPL Section 7) at
 *  http://statnet.org/attribution
 *
 *  Copyright 2003-2013 Statnet Commons
 */
#ifndef WTCHANGESTAT_H
#define WTCHANGESTAT_H

#include "wtedgetree.h"

typedef struct WtModelTermstruct {
  void (*c_func)(Vertex, Vertex, double, struct WtModelTermstruct*, WtNetwork*);
  void (*d_func)(Edge, Vertex*, Vertex*, double*, struct WtModelTermstruct*, WtNetwork*);
  void (*i_func)(struct WtModelTermstruct*, WtNetwork*);
  void (*u_func)(Vertex, Vertex, double, struct WtModelTermstruct*, WtNetwork*);
  void (*f_func)(struct WtModelTermstruct*, WtNetwork*);
  void (*s_func)(struct WtModelTermstruct*, WtNetwork*);
  double *attrib; /* Ptr to vector of covariates (if necessary; generally unused) */
  int nstats;   /* Number of change statistics to be returned */
  double *dstats; /* ptr to change statistics returned */
  int ninputparams; /* Number of input parameters passed to function */
  double *inputparams; /* ptr to input parameters passed */
  double *statcache; /* vector of the same length as dstats */
  void *storage; /* optional space for persistent storage */
  void **aux_storage; /* optional space for persistent public (auxiliary) storage */
} WtModelTerm;

#include "changestat_common.inc"

/* macros that tell whether a particular edge exists */
#define WtIS_OUTEDGE(a,b) (WtEdgetreeSearch((a),(b),nwp->outedges)!=0?1:0)
#define WtIS_INEDGE(a,b) (WtEdgetreeSearch((a),(b),nwp->inedges)!=0?1:0)
#define WtIS_UNDIRECTED_EDGE(a,b) WtIS_OUTEDGE(MIN(a,b), MAX(a,b))

/* Return the Edge number of the smallest-labelled neighbor of the node 
   labelled "a".  Or, return the Edge number of the next-largest neighbor 
   starting from the pointer "e", which points to a node in an edgetree. 
   Mostly, these are utility macros used by the STEP_THROUGH_OUTEDGES 
   and STEP_THROUGH_INEDGES macros. */
#define WtMIN_OUTEDGE(a) (WtEdgetreeMinimum(nwp->outedges, (a)))
#define WtMIN_INEDGE(a) (WtEdgetreeMinimum(nwp->inedges, (a)))
#define WtNEXT_OUTEDGE(e) (WtEdgetreeSuccessor(nwp->outedges,(e)))
#define WtNEXT_INEDGE(e) (WtEdgetreeSuccessor(nwp->inedges,(e)))

/* The OUTWT and INWT macros give the weight of edge e, depending
   on whether it is an in-edge or an out-edge.  Presumably the first endnode
   of the edge is already known in this context. */
#define OUTWT(e) (nwp->outedges[(e)].weight)
#define INWT(e) (nwp->inedges[(e)].weight)

// These are "declaring" versions of the above, optimized for use in EXEC_TROUGH_*EDGES macros.
#define WtSTEP_THROUGH_OUTEDGES_DECL(a,e,v) for(Edge e=WtMIN_OUTEDGE(a);OUTVAL(e)!=0;e=WtNEXT_OUTEDGE(e))
#define WtSTEP_THROUGH_INEDGES_DECL(a,e,v) for(Edge e=WtMIN_INEDGE(a);INVAL(e)!=0;e=WtNEXT_INEDGE(e))

/* Instead of stepping through execute "subroutine" for each neighbor
   automatically adapting to undirected networks. w gets the weight of
   the edge in question. */
/* NOTE: For some reason, GCC complains when it encounters multiple
   declaration in a single statement inside the subroutine, so
   double v1, v2;
   might fail, while
   double v1;
   double v2;
   works.*/
#define WtEXEC_THROUGH_OUTEDGES(a,e,v,w,subroutine) if(DIRECTED){ WtSTEP_THROUGH_OUTEDGES_DECL(a,e,v) {Vertex v=OUTVAL(e); double w=OUTWT(e); subroutine} } else { WtEXEC_THROUGH_EDGES(a,e,v,w,subroutine) }
#define WtEXEC_THROUGH_INEDGES(a,e,v,w,subroutine) if(DIRECTED){ WtSTEP_THROUGH_INEDGES_DECL(a,e,v) {Vertex v=INVAL(e); double w=INWT(e); subroutine} } else { WtEXEC_THROUGH_EDGES(a,e,v,w,subroutine) }
#define WtEXEC_THROUGH_EDGES(a,e,v,w,subroutine) { WtSTEP_THROUGH_OUTEDGES_DECL(a,e,v) {Vertex v=OUTVAL(e); double w=OUTWT(e); subroutine}  WtSTEP_THROUGH_INEDGES_DECL(a,e,v) {Vertex v=INVAL(e); double w=INWT(e); subroutine} }

/* Non-adaptive versions of the above. (I.e. ForceOUT/INEDGES.) */
#define WtEXEC_THROUGH_FOUTEDGES(a,e,v,w,subroutine) WtSTEP_THROUGH_OUTEDGES_DECL(a,e,v) {Vertex v=OUTVAL(e); double w=OUTWT(e); subroutine}
#define WtEXEC_THROUGH_FINEDGES(a,e,v,w,subroutine) WtSTEP_THROUGH_INEDGES_DECL(a,e,v) {Vertex v=INVAL(e); double w=INWT(e); subroutine}

/* Exectute through all edges (nonzero values) in the network. */
#define WtEXEC_THROUGH_NET_EDGES(a,b,e,w,subroutine) for(Vertex a=1; a <= N_NODES; a++)  WtEXEC_THROUGH_FOUTEDGES(a, e, b, w, {subroutine});

/* Get and set the weight of the (a,b) edge. */
#define WtGETWT(a,b) (WtGetEdge(a,b,nwp))
#define WtSETWT(a,b,w) (WtSetEdge(a,b,w,nwp))

/* Cycle through all toggles proposed for the current step, then
   make the current toggle in case of more than one proposed toggle, then
   undo all of the toggles to reset the original network state.  */
#define WtFOR_EACH_TOGGLE for(unsigned int WtTOGGLEIND=0; WtTOGGLEIND<ntoggles; WtTOGGLEIND++)
/* The idea here is to essentially swap the contents of the proposed
   weights with the current weights, and then swap them back when
   done. */
#define WtTAIL (tail_var)
#define WtHEAD (head_var)
#define WtNEWWT (newwt_var)
#define WtOLDWT (oldwt_var)

#define WtGETOLDTOGGLEINFO() Vertex WtTAIL=tails[WtTOGGLEIND], WtHEAD=heads[WtTOGGLEIND]; double WtOLDWT=WtGETWT(WtTAIL,WtHEAD);
#define WtGETTOGGLEINFO() WtGETOLDTOGGLEINFO(); double WtNEWWT=weights[WtTOGGLEIND];
#define WtGETNEWTOGGLEINFO() Vertex WtTAIL=tails[WtTOGGLEIND], WtHEAD=heads[WtTOGGLEIND]; double WtNEWWT=weights[WtTOGGLEIND];

/* SETWT_WITH_BACKUP(a) must be called _after_ GETTOGGLEINFO! */
#define WtSETWT_WITH_BACKUP() {WtSETWT(WtTAIL,WtHEAD,WtNEWWT); weights[WtTOGGLEIND]=WtOLDWT;}
#define WtUNDO_SETWT() {WtGETOLDTOGGLEINFO(); WtSETWT(WtTAIL,WtHEAD,weights[WtTOGGLEIND]); weights[WtTOGGLEIND]=WtOLDWT;}
#define WtIF_MORE_TO_COME if(WtTOGGLEIND+1<ntoggles)
#define WtSETWT_IF_MORE_TO_COME() {WtIF_MORE_TO_COME{WtSETWT_WITH_BACKUP();}}
#define WtUNDO_PREVIOUS for(int TOGGLEIND=ntoggles-2; WtTOGGLEIND>=0; WtTOGGLEIND--)
#define WtUNDO_PREVIOUS_SETWTS() {WtUNDO_PREVIOUS{WtUNDO_SETWT();}}
/* Brings together the above operations:
   For each toggle:
      Get the current edge weight.
      Calculate the change.
      Back up the current edge weight by swapping weight[i] with current edge weight.
   For each toggle:
      Undo the changes by swapping them back. */
#define WtEXEC_THROUGH_TOGGLES(subroutine){WtFOR_EACH_TOGGLE{ WtGETTOGGLEINFO(); {subroutine}; WtSETWT_IF_MORE_TO_COME();}; WtUNDO_PREVIOUS_SETWTS();}

#define SAMEDYAD(a1,b1,a2,b2) (DIRECTED? a1==a2 && b1==b2 : MIN(a1,b1)==MIN(a2,b2) && MAX(a1,b1)==MAX(a2,b2))

#define WtGETOLDWT(a,b) (SAMEDYAD(WtTAIL,WtHEAD,a,b)?WtOLDWT:WtGETWT(a,b))
#define WtGETNEWWT(a,b) (SAMEDYAD(WtTAIL,WtHEAD,a,b)?WtNEWWT:WtGETWT(a,b))
#define WtGETNEWWTOLD(a,b,old) (SAMEDYAD(WtTAIL,WtHEAD,a,b)?WtNEWWT:(old))


/****************************************************/
/* changestat function prototypes, 
   plus a few supporting function prototypes */
#define WtC_CHANGESTAT_FN(a) void (a) (Vertex tail, Vertex head, double weight, WtModelTerm *mtp, WtNetwork *nwp)
#define WtD_CHANGESTAT_FN(a) void (a) (Edge ntoggles, Vertex *tails, Vertex *heads, double *weights, WtModelTerm *mtp, WtNetwork *nwp)
#define WtI_CHANGESTAT_FN(a) void (a) (WtModelTerm *mtp, WtNetwork *nwp)
#define WtU_CHANGESTAT_FN(a) void (a) (Vertex tail, Vertex head, double weight, WtModelTerm *mtp, WtNetwork *nwp)
#define WtF_CHANGESTAT_FN(a) void (a) (WtModelTerm *mtp, WtNetwork *nwp)
#define WtS_CHANGESTAT_FN(a) void (a) (WtModelTerm *mtp, WtNetwork *nwp)

/* This macro wraps two calls to an s_??? function with toggles
   between them. */
#define WtD_FROM_S							\
  {									\
    (*(mtp->s_func))(mtp, nwp);  /* Call s_??? function */		\
    memcpy(mtp->statcache,mtp->dstats,N_CHANGE_STATS*sizeof(double));	\
    /* Note: This cannot be abstracted into EXEC_THROUGH_TOGGLES. */	\
    WtFOR_EACH_TOGGLE{ WtGETTOGGLEINFO(); WtSETWT_WITH_BACKUP(); }		\
    (*(mtp->s_func))(mtp, nwp);  /* Call s_??? function */		\
    for(unsigned int i=0; i<N_CHANGE_STATS; i++)			\
      mtp->dstats[i] -= mtp->statcache[i];				\
    WtFOR_EACH_TOGGLE{ WtUNDO_SETWT(); }					\
  }

/* This macro constructs a function that wraps D_FROM_S. */
#define WtD_FROM_S_FN(a) WtD_CHANGESTAT_FN(a) WtD_FROM_S


/* If STRICT_Wt_HEADERS is not set, give the terms more generic names. */
#ifndef STRICT_Wt_HEADERS

#define IS_OUTEDGE WtIS_OUTEDGE
#define IS_INEDGE WtIS_INEDGE
#define IS_UNDIRECTED_EDGE WtIS_UNDIRECTED_EDGE
#define MIN_OUTEDGE WtMIN_OUTEDGE
#define MIN_INEDGE WtMIN_INEDGE
#define NEXT_OUTEDGE WtNEXT_OUTEDGE
#define NEXT_INEDGE WtNEXT_INEDGE
#define STEP_THROUGH_OUTEDGES_DECL WtSTEP_THROUGH_OUTEDGES_DECL
#define STEP_THROUGH_INEDGES_DECL WtSTEP_THROUGH_INEDGES_DECL
#define EXEC_THROUGH_OUTEDGES WtEXEC_THROUGH_OUTEDGES
#define EXEC_THROUGH_INEDGES WtEXEC_THROUGH_INEDGES
#define EXEC_THROUGH_EDGES WtEXEC_THROUGH_EDGES
#define EXEC_THROUGH_FOUTEDGES WtEXEC_THROUGH_FOUTEDGES
#define EXEC_THROUGH_FINEDGES WtEXEC_THROUGH_FINEDGES
#define EXEC_THROUGH_NET_EDGES WtEXEC_THROUGH_NET_EDGES

#define GETWT WtGETWT
#define SETWT WtSETWT
#define FOR_EACH_TOGGLE WtFOR_EACH_TOGGLE
#define TAIL WtTAIL
#define HEAD WtHEAD
#define NEWWT WtNEWWT
#define OLDWT WtOLDWT

#define TOGGLEIND WtTOGGLEIND

#define GETOLDTOGGLEINFO WtGETOLDTOGGLEINFO
#define GETTOGGLEINFO WtGETTOGGLEINFO
#define GETNEWTOGGLEINFO WtGETNEWTOGGLEINFO
#define SETWT_WITH_BACKUP WtSETWT_WITH_BACKUP
#define UNDO_SETWT WtUNDO_SETWT
#define IF_MORE_TO_COME WtIF_MORE_TO_COME
#define SETWT_IF_MORE_TO_COME WtSETWT_IF_MORE_TO_COME
#define UNDO_PREVIOUS WtUNDO_PREVIOUS
#define UNDO_PREVIOUS_SETWTS WtUNDO_PREVIOUS_SETWTS
#define EXEC_THROUGH_TOGGLES WtEXEC_THROUGH_TOGGLES
#define GETOLDWT WtGETOLDWT
#define GETNEWWT WtGETNEWWT
#define GETNEWWTOLD WtGETNEWWTOLD

#define D_FROM_S WtD_FROM_S

#endif // STRICT_Wt_HEADERS

#endif