/*
 * header for nmsimplex_bbincr.h
 * Author : Kostas Katrinis
 * kostas.katrinis@gmail.com
 * 
 * Extension of Michael F. Hutt implementation
 * of Nelder Mead Simplex search for black box,
 * incremental use (e.g. for sampled objective
 * function with complex analytical evaluation)
 *
 */

#ifndef NM_SIMPLEX_INCR_H
#define NM_SIMPLEX_INCR_H

//#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>

#ifdef MACOSX
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

namespace allscale { namespace components {

#define MAX_IT      1000      /* maximum number of iterations */
#define ALPHA       1.0       /* reflection coefficient */
#define BETA        0.5       /* contraction coefficient */
#define GAMMA       2.0       /* expansion coefficient */

/* structure type of a single optimization step return status */
struct optstepresult{
      /* true if optimization has converged for the specified objective */
      bool converged;
      /* number of threads for parameters to set for sampling */
      double threads;
      /* index to frequency vector for freq parameter to set for sampling*/
      int freq_idx;
};

/* enumeration encoding state that the incremental Nelder Mead optimizer is at */
enum iterationstates {start, reflection, expansion, contraction};

class NelderMead {

  public:
    NelderMead(double);
    void initialize_simplex(double params[][2], double*,double*,double*);
    void print_initial_simplex();
    void print_iteration();
    optstepresult step(double param);
    double* getMinVertices(){
        return v[vs];
    }

    double getMinObjective(){
        return min;
    }

    unsigned long int getIterations(){return itr;}

  private:
    int vg_index();
    int vs_index();
    int vh_index();
    void my_constraints(double*);
    void centroid();
    bool testConvergence();
    void updateObjectives();

    double round2(double num, int precision)
    {
      double rnum = 0.0;
      int tnum;

      if (num == 0.0)
        return num;

      rnum = num*pow(10,precision);
      tnum = (int)(rnum < 0 ? rnum-0.5 : rnum + 0.5);
      rnum = tnum/pow(10,precision);

      return rnum;
    }

    /* vertex with smallest value */
    int vs;         

     /* vertex with next smallest value */
    int vh;        

    /* vertex with largest value */
    int vg;         
	
    int i,j,row;

    const int n=2;

    /* track the number of function evaluations */
    int k;

    /* track the number of iterations */
    int itr;	  
	
    /* holds vertices of simplex */
    double **v;

     /* value of function at each vertex */
    double *f;

    /* value of function at reflection point */
    double fr;      

    /* value of function at expansion point */
    double fe;      

     /* value of function at contraction point */
    double fc;      

    /* reflection - coordinates */
    double *vr;    

    /* expansion - coordinates */
    double *ve;     

    /* contraction - coordinates */
    double *vc;     

    /* centroid - coordinates */
    double *vm;     

    double min;
	
    double fsum,favg,s;

    double EPSILON;

    iterationstates state_;

    const int MAXITERATIONS = 15;
  
    double constraint_min[2];

    double constraint_max[2];

};

}
}
#endif
