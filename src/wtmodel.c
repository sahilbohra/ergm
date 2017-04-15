/*  File src/wtmodel.c in package ergm, part of the Statnet suite
 *  of packages for network analysis, http://statnet.org .
 *
 *  This software is distributed under the GPL-3 license.  It is free,
 *  open source, and has the attribution requirements (GPL Section 7) at
 *  http://statnet.org/attribution
 *
 *  Copyright 2003-2013 Statnet Commons
 */
#include <string.h>
#include "wtmodel.h"

/*****************
  void WtModelDestroy
******************/
void WtModelDestroy(WtNetwork *nwp, WtModel *m)
{  
  WtDestroyStats(nwp, m);
  
  for(unsigned int i=0; i < m->n_aux; i++)
    if(m->termarray[0].aux_storage[i]!=NULL){
      Free(m->termarray[0].aux_storage[i]);
      m->termarray[0].aux_storage[i] = NULL;
    }
  
  if(m->termarray[0].aux_storage!=NULL){
    Free(m->termarray[0].aux_storage);
  }
  
  WtEXEC_THROUGH_TERMS({
      if(mtp->aux_storage!=NULL)
	mtp->aux_storage=NULL;
    });
  
  Free(m->dstatarray);
  Free(m->termarray);
  Free(m->workspace); 
  Free(m);
}

/*****************
 int WtModelInitialize

 Allocate and initialize the WtModelTerm structures, each of which contains
 all necessary information about how to compute one term in the model.
*****************/
WtModel* WtModelInitialize (char *fnames, char *sonames, double **inputsp,
			int n_terms) {
  int i, j, k, l, offset;
  WtModelTerm *thisterm;
  char *fn,*sn;
  WtModel *m;
  double *inputs=*inputsp;
  
  m = (WtModel *) Calloc(1, WtModel);
  m->n_terms = n_terms;
  m->termarray = (WtModelTerm *) Calloc(n_terms, WtModelTerm);
  m->dstatarray = (double **) Calloc(n_terms, double *);
  m->n_stats = 0;
  m->n_aux = 0;
  for (l=0; l < n_terms; l++) {
      thisterm = m->termarray + l;
      
      /* Initialize storage and term functions to NULL. */
      thisterm->storage = NULL;
      thisterm->aux_storage = NULL;
      thisterm->d_func = NULL;
      thisterm->c_func = NULL;
      thisterm->s_func = NULL;
      thisterm->i_func = NULL;
      thisterm->u_func = NULL;
      thisterm->f_func = NULL;
      
      /* First, obtain the term name and library: fnames points to a
      single character string, consisting of the names of the selected
      options concatenated together and separated by spaces.  This is
      passed by the calling R function.  These names are matched with
      their respective C functions that calculate the appropriate
      statistics.  Similarly, sonames points to a character string
      containing the names of the shared object files associated with
      the respective functions.*/
      for (; *fnames == ' ' || *fnames == 0; fnames++);
      for (i = 0; fnames[i] != ' ' && fnames[i] != 0; i++);
      fnames[i] = 0;
      for (; *sonames == ' ' || *sonames == 0; sonames++);
      for (j = 0; sonames[j] != ' ' && sonames[j] != 0; j++);
      sonames[j] = 0;
      /* Extract the required string information from the relevant sources */
      fn=Calloc(i+3, char);
      fn[1]='_';
      for(k=0;k<i;k++)
        fn[k+2]=fnames[k];
      fn[i+2]='\0';
      /* fn is now the string 'd_[name]', where [name] is fname */
/*      Rprintf("fn: %s\n",fn); */
      sn=Calloc(j+1, char);
      sn=strncpy(sn,sonames,j);
      sn[j]='\0';


      /*  Second, process the values in
          model$option[[optionnumber]]$inputs; See comments in
          InitErgm.r for details. This needs to be done before change
          statistica are found, to determine whether a term is
          auxiliary.  */
      offset = (int) *inputs++;  /* Set offset for attr vector */
      /*      Rprintf("offsets: %f %f %f %f %f\n",inputs[0],inputs[1],inputs[2], */
      /*		         inputs[3],inputs[4],inputs[5]); */
      thisterm->nstats = (int) *inputs++; /* If >0, # of statistics returned. If ==0 an auxiliary statistic. */
      
      /*      Rprintf("l %d offset %d thisterm %d\n",l,offset,thisterm->nstats); */
      
      /*  Update the running total of statistics */
      m->n_stats += thisterm->nstats; 
      m->dstatarray[l] = (double *) Calloc(thisterm->nstats, double);
      thisterm->dstats = m->dstatarray[l];  /* This line is important for
                                               eventually freeing up allocated
					       memory, since thisterm->dstats
					       can be modified but 
					       m->dstatarray[l] cannot be.  */
      thisterm->statcache = (double *) Calloc(thisterm->nstats, double);

      thisterm->ninputparams = (int) *inputs++; /* Set # of inputs */
      /* thisterm->inputparams is a ptr to inputs */
      thisterm->inputparams = (thisterm->ninputparams ==0) ? 0 : inputs; 
      
      thisterm->attrib = inputs + offset; /* Ptr to attributes */
      inputs += thisterm->ninputparams;  /* Skip to next model option */

      /* If the term's nstats==0, it is auxiliary: it does not affect
	 acceptance probabilities or contribute any
	 statistics. Therefore, its d_ and s_ functions are never
	 called and are not initialized. It is only used for its u_
	 function. Therefore, the following code is only run when
	 thisterm->nstats>0. */
      if(thisterm->nstats){
	/*  Most important part of the WtModelTerm:  A pointer to a
	    function that will compute the change in the network statistic of 
	    interest for a particular edge toggle.  This function is obtained by
	    searching for symbols associated with the object file with prefix
	    sn, having the name fn.  Assuming that one is found, we're golden.*/ 
	fn[0]='c';
	thisterm->c_func = 
	  (void (*)(Vertex, Vertex, double, WtModelTerm*, WtNetwork*))
	  R_FindSymbol(fn,sn,NULL);

	if(thisterm->c_func==NULL){
	  fn[0]='d';
	  thisterm->d_func = 
	    (void (*)(Edge, Vertex*, Vertex*, double*, WtModelTerm*, WtNetwork*))
	    R_FindSymbol(fn,sn,NULL);
	
	  if(thisterm->d_func==NULL){
	    error("Error in WtModelInitialize: could not find function %s in "
		  "namespace for package %s. Memory has not been deallocated, so restart R sometime soon.\n",fn,sn);
	  }
	}
	
	/* Optional function to compute the statistic of interest for
	   the network given. It can be more efficient than going one
	   edge at a time. */
	fn[0]='s';
	thisterm->s_func = 
	  (void (*)(WtModelTerm*, WtNetwork*)) R_FindSymbol(fn,sn,NULL);
      }else m->n_aux++;
      
      /* Optional functions to store persistent information about the
	 network state between calls to d_ functions. */
      fn[0]='u';
      thisterm->u_func = 
	(void (*)(Vertex, Vertex, double, WtModelTerm*, WtNetwork*)) R_FindSymbol(fn,sn,NULL);

      /* If it's an auxiliary, then it needs a u_function, or
	 it's not doing anything. */
      if(thisterm->nstats==0 && thisterm->u_func==NULL){
	error("Error in WtModelInitialize: could not find updater function %s in "
	      "namespace for package %s: this term will not do anything. Memory has not been deallocated, so restart R sometime soon.\n",fn,sn);
      }
  

      /* Optional-optional functions to initialize and finalize the
	 term's storage. */
      
      fn[0]='i';
      thisterm->i_func = 
	(void (*)(WtModelTerm*, WtNetwork*)) R_FindSymbol(fn,sn,NULL);

      fn[0]='f';
      thisterm->f_func = 
	(void (*)(WtModelTerm*, WtNetwork*)) R_FindSymbol(fn,sn,NULL);

      
      /*Clean up by freeing sn and fn*/
      Free(fn);
      Free(sn);

      /*  The lines above set thisterm->inputparams to point to needed input
      parameters (or zero if none) and then increments the inputs pointer so
      that it points to the inputs for the next model option for the next pass
      through the loop. */

      fnames += i;
      sonames += j;
    }
  
  m->workspace = (double *) Calloc(m->n_stats, double);
  for(i=0; i < m->n_stats; i++)
    m->workspace[i] = 0.0;

  
  /* Allocate auxiliary storage and put a pointer to it on every model term. */
  if(m->n_aux){
    m->termarray[0].aux_storage = (void *) Calloc(m->n_aux, void *);
    for(l=1; l < n_terms; l++)
      m->termarray[l].aux_storage = m->termarray[0].aux_storage;
  }
  
  *inputsp = inputs;
  return m;
}

/*
  WtChangeStats
  A helper's helper function to compute change statistics.
  The vector of changes is written to m->workspace.
*/
void WtChangeStats(unsigned int ntoggles, Vertex *tails, Vertex *heads, double *weights,
				 WtNetwork *nwp, WtModel *m){
  memset(m->workspace, 0, m->n_stats*sizeof(double)); /* Zero all change stats. */ 

  /* Make a pass through terms with d_functions. */
  WtEXEC_THROUGH_TERMS_INTO(m->workspace, {
      mtp->dstats = dstats; /* Stuck the change statistic here.*/
      if(mtp->c_func==NULL && mtp->d_func)
	(*(mtp->d_func))(ntoggles, tails, heads, weights,
			 mtp, nwp);  /* Call d_??? function */
    });
  /* Notice that mtp->dstats now points to the appropriate location in
     m->workspace. */
  
  /* Put the original destination dstats back unless there's only one
     toggle. */
  if(ntoggles!=1){
    unsigned int i = 0;
    WtEXEC_THROUGH_TERMS({
	mtp->dstats = m->dstatarray[i];
	i++;
      });
  }

  /* Make a pass through terms with c_functions. */
  FOR_EACH_TOGGLE{
    GETTOGGLEINFO();
    
    WtEXEC_THROUGH_TERMS_INTO(m->workspace, {
	if(mtp->c_func){
	  if(ntoggles!=1) ZERO_ALL_CHANGESTATS();
	  (*(mtp->c_func))(TAIL, HEAD, NEWWT,
			   mtp, nwp);  /* Call d_??? function */
	  
	  if(ntoggles!=1){
	    for(unsigned int k=0; k<N_CHANGE_STATS; k++){
	      dstats[k] += mtp->dstats[k];
	    }
	  }
	}
      });

    /* Update storage and network */    
    IF_MORE_TO_COME{
      WtUPDATE_STORAGE_COND(TAIL, HEAD, NEWWT, nwp, m, NULL, mtp->d_func==NULL);
      SETWT_WITH_BACKUP();
    }
  }
  /* Undo previous storage updates and toggles */
  UNDO_PREVIOUS{
    GETOLDTOGGLEINFO();
    WtUPDATE_STORAGE_COND(TAIL,HEAD,weights[TOGGLEIND], nwp, m, NULL, mtp->d_func==NULL);
    SETWT(TAIL,HEAD,weights[TOGGLEIND]);
    weights[TOGGLEIND]=OLDWT;
  }
}
      
/*
  WtInitStats
  A helper's helper function to initialize storage for functions that use it.
*/
void WtInitStats(WtNetwork *nwp, WtModel *m){
  // Iterate in reverse, so that auxliary terms get initialized first.  
  WtEXEC_THROUGH_TERMS_INREVERSE({
      double *dstats = mtp->dstats;
      mtp->dstats = NULL; // Trigger segfault if i_func tries to write to change statistics.
      if(mtp->i_func)
	(*(mtp->i_func))(mtp, nwp);  /* Call i_??? function */
      else if(mtp->u_func) /* No initializer but an updater -> uses a 1-function implementation. */
	(*(mtp->u_func))(0, 0, 0, mtp, nwp);  /* Call u_??? function */
      mtp->dstats = dstats;
    });
}

/*
  WtDestroyStats
  A helper's helper function to finalize storage for functions that use it.
*/
void WtDestroyStats(WtNetwork *nwp, WtModel *m){
  unsigned int i=0;
  WtEXEC_THROUGH_TERMS({
      if(mtp->f_func)
	(*(mtp->f_func))(mtp, nwp);  /* Call f_??? function */
      Free(m->dstatarray[i]);
      Free(mtp->statcache);
      if(mtp->storage){
	Free(mtp->storage);
	mtp->storage = NULL;
      }
      i++;
    });
}
