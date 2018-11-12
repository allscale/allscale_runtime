/*
 * nmsimplex_bbincr.c **
 * Author : Kostas Katrinis
 * kostas.katrinis@gmail.com
 *
 * Extension of Michael F. Hutt implementation
 * of Nelder Mead Simplex search for black box,
 * incremental use (e.g. for sampled objective
 * function with complex analytical evaluation)
 *
 */

#include <allscale/components/nmsimplex_bbincr.hpp>
#define NMD_DEBUG_ 1
#define NMD_INFO_ 1

/* create the initial simplex

   vector<doubl

 */

namespace allscale { namespace components {

//NelderMead::NelderMead(double (*objfunc)(double[]),double eps){
NelderMead::NelderMead(double eps){

  EPSILON=eps;
#ifdef NMD_INFO_
  std::cout << "[NelderMead|INFO] Initial Convergence Threshold set is " << EPSILON << std::endl;
#endif
  itr=0;
  state_ = start;

  /* dynamically allocate arrays */

  /* allocate the rows of the arrays */
  v =  (double **) malloc ((n+1) * sizeof(double *));
  f =  (double *) malloc ((n+1) * sizeof(double));
  vr = (double *) malloc (n * sizeof(double));
  ve = (double *) malloc (n * sizeof(double));
  vc = (double *) malloc (n * sizeof(double));
  vm = (double *) malloc (n * sizeof(double));

  /* allocate the columns of the arrays */
  for (i=0;i<=n;i++) {
    v[i] = (double *) malloc (n * sizeof(double));
  }
}

void NelderMead::my_constraints(double x[])
{
  // round to integer and bring again with allowable margins
  // todo fix: generalize
  if (x[0] < constraint_min[0] || x[0] > constraint_max[0]){
    x[0] = (constraint_min[0] + constraint_max[0])/2;
  }

  if (x[1] < constraint_min[1] || x[1] > constraint_max[1]){
    x[1] = (constraint_min[1] + constraint_max[1])/2;
  }

  x[0]=round(x[0]);
  x[1]=round(x[1]);
}

/* FIXME: generalize */
void NelderMead::initialize_simplex(double params[][2], double values[], double constraint_min[],double constraint_max[])
{
  int i,j;

  for (i=0;i<=n;i++) {
    for (j=0;j<n;j++) {
  	  v[i][j] = params[i][j];
    }
    f[i]=values[i];
    this->constraint_min[i]=constraint_min[i];
    this->constraint_max[i]=constraint_max[i];
  }
  itr=0;
}


/* print out the initial values */
void NelderMead::print_initial_simplex()
{
  int i,j;
  std::cout << "[NelderMead DEBUG] Initial Values\n";
  for (j=0;j<=n;j++) {
    for (i=0;i<n;i++) {
      std::cout << v[j][i] << ",";
    }
    std::cout << "Objective value = " << f[j] << std::endl;
  }
}


/* print out the value at each iteration */
void NelderMead::print_iteration()
{
  int i,j;
  std::cout << "[NelderMead DEBUG] Iteration " << itr << std::endl;
  //printf("Iteration %d\n",itr);
  for (j=0;j<=n;j++) {
    std::cout << "[NelderMead DEBUG] Vertex-" << j+1 << "=(";
    for (i=0;i<n;i++) {
      //printf("%f %f\n\n",v[j][i],f[j]);
      std::cout << v[j][i];
      if (i<n-1)
        std::cout << "," ;
    }
    std::cout << ")=" << f[j] << std::endl;
  }
  std::cout << "[NelderMead DEBUG] Current Objective Minimum is at: " << f[vs] << std::endl;
  std::cout << "[NelderMead DEBUG] f[vs]= " << f[vs] << ", vs = " << vs << std::endl;
  std::cout << "[NelderMead DEBUG] f[vh]= " << f[vh] << ", vh = " << vh << std::endl;
  std::cout << "[NelderMead DEBUG] f[vg]= " << f[vg] << ", vg = " << vg << std::endl;
}


/* find the index of the largest value */
int NelderMead::vg_index()
{
  int j;
  int vg=0;

  for (j=0;j<=n;j++) {
    if (f[j] > f[vg]) {
      vg = j;
    }
  }
  return vg;
}


/* find the index of the smallest value */
int NelderMead::vs_index()
{
  int j;
  int vs=0;

  for (j=0;j<=n;j++) {
    if (f[j] < f[vs]) {
      vs = j;
    }
  }
  return vs;
}


/* find the index of the second largest value */
int NelderMead::vh_index()
{
  int j;

  for (j=0;j<=n;j++) {
    if (f[j] > f[vh] && f[j] < f[vg]) {
      vh = j;
    }
  }
  return vh;
}


/* calculate the centroid */
void NelderMead::centroid()
{
  int j,m;
  double cent;

  for (j=0;j<=n-1;j++) {
    cent=0.0;
    for (m=0;m<=n;m++) {
      if (m!=vg) {
	      cent += v[m][j];
      }
    }
    vm[j] = cent/n;
  }
}

optstepresult NelderMead::step(double param)
{
  optstepresult res;
  res.threads=0;
  res.freq_idx=-1;
  switch (state_){

    /** ITERATION START **/
    case start:
      itr++;
#ifdef NMD_DEBUG_
      std::cout << "[NelderMead DEBUG] State = Start" << std::endl;
      print_initial_simplex();
#endif
      // todo: implement here the simplex initialization, currently this is
      // done in the constructor

      /* find the index of the largest value (W) */
      vg = vg_index();

      /* find the index of the smallest value (B) */
      vs = vs_index();

      /* find the index of the second largest value (G) */
      vh = vh_index();

      /* calculate the centroid */
      centroid();

      /* reflect vg to new vertex vr */
      for (j=0;j<=n-1;j++) {
        /*vr[j] = (1+ALPHA)*vm[j] - ALPHA*v[vg][j];*/
        /*
        */
        vr[j] = vm[j]+ALPHA*(vm[j]-v[vg][j]);

        // std::cout << "vm[" << j << "]=" << vm[j] << std::endl;
        // std::cout << "v[vg" << j << "]=" << v[vg][j] << std::endl;
        // std::cout << "ALPHA=" << ALPHA << std::endl;
        // std::cout << "Vr[" << j << "]=" << vr[j] << std::endl;
      }
      my_constraints(vr);
#ifdef NMD_DEBUG_
      std::cout << "[NelderMead DEBUG] Reflection Parameter = ("
                << vr[0] << "," << vr[1] << ")"
                << std::endl;
#endif
      // enter reflection state
      state_=reflection;
      res.threads=vr[0];
      res.freq_idx=vr[1];

      break;

    /** REFLECTION **/

    /** This state is entered when we have received a sample of the objective
     ** function at the reflection vertex
     **/
    case reflection:
#ifdef NMD_DEBUG_
      std::cout << "[NelderMead DEBUG] State = Reflection" << std::endl;
#endif
      fr=param;
      //fr = objfunc(vr);

		  if (fr < f[vh]){ // f(R) < f(G) - Case (i)
        if (fr >= f[vs]) { // f(R)>f(B)
          for (j=0;j<=n-1;j++) { // replace W with R and end iteration
	         v[vg][j] = vr[j];
          }
          f[vg] = fr;
          updateObjectives();
          state_=start;
          break;
        }

        /* investigate a step further through expansion in this direction */
        else{
          for (j=0;j<=n-1;j++) {
            /*ve[j] = GAMMA*vr[j] + (1-GAMMA)*vm[j];*/
            ve[j] = vm[j]+GAMMA*(vr[j]-vm[j]);
          }
#ifdef NMD_DEBUG_
      std::cout << "[NelderMead DEBUG] Expansion Parameter = ("
                << ve[0] << "," << ve[1] << ")"
                << std::endl;
#endif
          my_constraints(ve);
          // enter the state waiting for a sampled value of the objective function
          // at the expansion vertex
          state_=expansion;
          res.threads=ve[0];
          res.freq_idx=ve[1];

          break;
        }

      }else{ // f(R) > f(G) - Case (ii)
        if (fr < f[vg]) { // f(R) < f(W)
          for (j=0;j<=n-1;j++) {  // replace W with R
           v[vg][j] = vr[j];
          }
          f[vg] = fr;
        }

        if (fr < f[vg] && fr >= f[vh]) {
	        /* perform outside contraction */
	        for (j=0;j<=n-1;j++) {
	          /*vc[j] = BETA*v[vg][j] + (1-BETA)*vm[j];*/
	          vc[j] = vm[j]+BETA*(vr[j]-vm[j]);
	        }
#ifdef NMD_DEBUG_
      std::cout << "[NelderMead DEBUG] Contraction Parameter = ("
                << vc[0] << "," << vc[1] << ")"
                << std::endl;
#endif
          my_constraints(vc);
          // enter the state waiting for a sampled value of the objective function
          // at the outside contraction vertex
          state_=contraction;
          res.threads=vc[0];
          res.freq_idx=vc[1];
          break;
        } else {
	        /* perform inside contraction */
	        for (j=0;j<=n-1;j++) {
	          /*vc[j] = BETA*v[vg][j] + (1-BETA)*vm[j];*/
	          vc[j] = vm[j]-BETA*(vm[j]-v[vg][j]);
	        }
#ifdef NMD_DEBUG_
      std::cout << "[NelderMead DEBUG] Contraction Parameter = ("
                << vc[0] << "," << vc[1] << ")"
                << std::endl;
#endif
	        my_constraints(vc);
          state_=contraction;
          res.threads=vc[0];
          res.freq_idx=vc[1];
          break;
        }


    /** EXPANSION **/

    /** This state is entered when we have received a sample of the objective
     ** function at the expansion vertex
     **/
    case expansion:
#ifdef NMD_DEBUG_
      std::cout << "[NelderMead DEBUG] State = Expansion" << std::endl;
#endif
      fe=param;
      //fe = objfunc(ve);
      if (fe < f[vs]) { // if f(E)<f(B)
  	    for (j=0;j<=n-1;j++) { // replace W with E
  	      v[vg][j] = ve[j];
  	    }
  	    f[vg] = fe;
      }
      else {
  	    for (j=0;j<=n-1;j++) { // replace W with E
  	      v[vg][j] = vr[j];
  	    }
  	    f[vg] = fr;
      }
      updateObjectives();
      state_=start;
      break;

    /** CONTRACTION **/

    /** This state is entered when we have received a sample of the objective
     ** function at the contraction vertex
     **/
    case contraction:
#ifdef NMD_DEBUG_
      std::cout << "[NelderMead|DEBUG] State = Contraction" << std::endl;
#endif
      fc=param;
      //fc = objfunc(vc);
      if (fc < f[vg]) { // f(C)<f(W)
  	    for (j=0;j<=n-1;j++) {
	        v[vg][j] = vc[j];
	      }
	      f[vg] = fc;
      } else {
        // apply shrinking
	      for (row=0;row<=n;row++) {
	        if (row != vs) {
	          for (j=0;j<=n-1;j++) {
	            v[row][j] = v[vs][j]+(v[row][j]-v[vs][j])/2.0;
              my_constraints(v[row]);
	         }
	        }
	      }
      }
      updateObjectives();
      state_=start;
      break;
    }
  }

  /* print out the value at each iteration */
#ifdef NMD_DEBUG_
  print_iteration();
#endif
  res.converged=testConvergence();
  return res;
}

bool NelderMead::testConvergence(){

  fsum = 0.0;
  for (j=0;j<=n;j++) {
    fsum += f[j];
  }
  favg = fsum/(n+1);
  s = 0.0;
  for (j=0;j<=n;j++) {
    s += pow((f[j]-favg),2.0)/(n);
  }
  s = sqrt(s);
  s = s /favg; // normalization step
#ifdef NMD_INFO_
  std::cout << "[NelderMead|INFO] Convergence Ratio is " << s << std::endl;
  std::cout << "[NelderMead|INFO] Convergence Threshold set is " << EPSILON << std::endl;
#endif
  if (s >= EPSILON && itr <= MAXITERATIONS)
    return false;
  else{
    vs = vs_index();
    min=f[vs];
    return true;
  }
}

void NelderMead::updateObjectives(){
  /* re-evaluate all the vertices */
	/*for (j=0;j<=n;j++) {
	  f[j] = objfunc(v[j]);
	}
  */

	/* find the index of the largest value */
	vg = vg_index();

	/* find the index of the smallest value */
	vs = vs_index();

	/* find the index of the second largest value */
	vh = vh_index();

  my_constraints(v[vg]);

	//f[vg] = objfunc(v[vg]);

	my_constraints(v[vh]);

  //f[vh] = objfunc(v[vh]);
}

}
}
/*

std::vector<double> NelderMead::minimum(){


  free(f);
  free(vr);
  free(ve);
  free(vc);
  free(vm);
  for (i=0;i<=n;i++) {
    free (v[i]);
  }
  free(v);
  return min;


}
*/




