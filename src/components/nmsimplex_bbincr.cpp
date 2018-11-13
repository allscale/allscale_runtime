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

namespace allscale
{
namespace components
{

//NelderMead::NelderMead(double (*objfunc)(double[]),double eps){
NelderMead::NelderMead(double eps)
{

    EPSILON = eps;
#ifdef NMD_INFO_
    std::cout << "[NelderMead|INFO] Initial Convergence Threshold set is " << EPSILON << std::endl;
#endif
    itr = 0;
    state_ = start;

    /* dynamically allocate arrays */

    /* allocate the rows of the arrays */
    v = (double **)malloc((n + 1) * sizeof(double *));
    f = (double *)malloc((n + 1) * sizeof(double));
    vr = (double *)malloc(n * sizeof(double));
    ve = (double *)malloc(n * sizeof(double));
    vc = (double *)malloc(n * sizeof(double));
    vm = (double *)malloc(n * sizeof(double));

    /* allocate the columns of the arrays */
    for (i = 0; i <= n; i++)
    {
        v[i] = (double *)malloc(n * sizeof(double));
    }
}

void NelderMead::my_constraints(double x[])
{
    // round to integer and bring again with allowable margins
    // todo fix: generalize

    // if (x[0] < constraint_min[0] || x[0] > constraint_max[0]){
    //   x[0] = (constraint_min[0] + constraint_max[0])/2;
    // }

    // if (x[1] < constraint_min[1] || x[1] > constraint_max[1]){
    //   x[1] = (constraint_min[1] + constraint_max[1])/2;
    // }

    for (auto i = 0u; i < 2u; ++i)
    {
        if (x[i] < constraint_min[i])
            x[i] = constraint_min[i];
        else if (x[i] > constraint_max[i])
            x[i] = constraint_max[i];
    }

    x[0] = round(x[0]);
    x[1] = round(x[1]);
}

/* FIXME: generalize */
void NelderMead::initialize_simplex(double params[][2], double values[], double constraint_min[], double constraint_max[])
{
    int i, j;

    for (i = 0; i <= n; i++)
    {
        for (j = 0; j < n; j++)
        {
            v[i][j] = params[i][j];
        }
        f[i] = values[i];
        this->constraint_min[i] = constraint_min[i];
        this->constraint_max[i] = constraint_max[i];
    }
    itr = 0;

    state_ = start;
}

/* print out the initial values */
void NelderMead::print_initial_simplex()
{
    int i, j;
    std::cout << "[NelderMead DEBUG] Initial Values\n";
    for (j = 0; j <= n; j++)
    {
        for (i = 0; i < n; i++)
        {
            std::cout << v[j][i] << ",";
        }
        std::cout << " Objective value = " << f[j] << std::endl;
    }
}

/* print out the value at each iteration */
void NelderMead::print_iteration()
{
    int i, j;
    std::cout << "[NelderMead DEBUG] Iteration " << itr << std::endl;
    //printf("Iteration %d\n",itr);
    for (j = 0; j <= n; j++)
    {
        std::cout << "[NelderMead DEBUG] Vertex-" << j + 1 << "=(";
        for (i = 0; i < n; i++)
        {
            //printf("%f %f\n\n",v[j][i],f[j]);
            std::cout << v[j][i];
            if (i < n - 1)
                std::cout << ",";
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
    int vg = 0;

    for (j = 0; j <= n; j++)
    {
        if (f[j] > f[vg])
        {
            vg = j;
        }
    }
    return vg;
}

/* find the index of the smallest value */
int NelderMead::vs_index()
{
    int j;
    int vs = 0;

    for (j = 0; j <= n; j++)
    {
        if (f[j] < f[vs])
        {
            vs = j;
        }
    }
    return vs;
}

/* find the index of the second largest value */
int NelderMead::vh_index()
{
    int j;

    for (j = 0; j <= n; j++)
    {
        if (f[j] > f[vh] && f[j] < f[vg])
        {
            vh = j;
        }
    }
    return vh;
}

/* calculate the centroid */
void NelderMead::centroid()
{
    int j, m;
    double cent;

    for (j = 0; j <= n - 1; j++)
    {
        cent = 0.0;
        for (m = 0; m <= n; m++)
        {
            if (m != vg)
            {
                cent += v[m][j];
            }
        }
        vm[j] = cent / n;
    }
}

void NelderMead::sort_vertices()
{
    // VV: -1 is used for padding because the index to this map will never evaluate to 0
    int map_to_index[] = {
        -1, 0, 1, 0, 2, 0, 0, 0};

    vg = vs = vh = 0;

    // VV: Compute greatest, smallest, and half-point
    for (i = 0; i <= n; ++i)
    {
        vg = f[i] > f[vg] ? i : vg;
        vs = f[i] < f[vs] ? i : vs;
    }

    // VV: Find out what's the half-point by using a bitmap,
    //     when vg==vs that means that all points are equal
    vh = 1 + 2 + 4 - (1 << vg) - (1 << vs);
    vh = map_to_index[vh];
}

optstepresult NelderMead::do_step_start(double param)
{
    optstepresult res;

    itr++;
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead DEBUG] State = Start" << std::endl;
    print_initial_simplex();
#endif
    sort_vertices();

    centroid();

    for (j = 0; j <= n - 1; j++)
    {
        vr[j] = vm[j] + ALPHA * (vm[j] - v[vg][j]);
    }
    my_constraints(vr);
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead DEBUG] Reflection Parameter = ("
              << vr[0] << "," << vr[1] << ")"
              << std::endl;
#endif
    // enter reflection state
    state_ = reflection;
    res.threads = vr[0];
    res.freq_idx = vr[1];

    return res;
}

optstepresult NelderMead::do_step_reflect(double param)
{
    optstepresult res;
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead DEBUG] State = Reflection" << std::endl;
#endif
    fr = param;

    std::cout << "fr:" << fr << " f[vh]:" << f[vh]
              << " f[vs]:" << f[vs] << std::endl;

    if ( (f[vs] <= fr) && (fr < f[vh]) ) {
        // VV: REFLECTED point is better than the SECOND BEST
        //     but NOT better than the BEST
        //     Replace WORST point with REFLECTED
        for (j = 0; j <= n - 1; j++)
        {
            v[vg][j] = vr[j];
        }
        f[vg] = fr;
        state_ = start;
        return do_step_start(param);
    } else if ( fr < f[vs] ) {
        // VV: REFLECTED is better than BEST
        
        for ( j=0; j<=n-1; ++j)
            ve[j] = vm[j] + GAMMA * (vr[j] - vm[j]);
        
        my_constraints(ve);
        // VV: Now evaluate EXPANDED
        res.threads = ve[0];
        res.freq_idx = ve[1];

        state_ = expansion;

        return res;
    } else if ( (f[vh] <= fr) && (fr < f[vg])) {
        // VV: REFLECTED between SECOND BEST and WORST
        
        for ( j=0; j<=n-1; ++j)
            vc[j] = vm[j] + BETA * (vr[j] - vm[j]);
        
        my_constraints(vc);

        // VV: Now evaluate EXPANDED
        res.threads = vc[0];
        res.freq_idx = vc[1];

        state_ = contraction;

        return res;
    } else {
        // VV: REFLECTED worse than WORST
        for ( j=0; j<=n-1; ++j)
            vc[j] = vm[j] - BETA * (vr[j] - vm[j]);
        
        my_constraints(vc);

        // VV: Now evaluate EXPANDED
        res.threads = vc[0];
        res.freq_idx = vc[1];

        state_ = contraction;

        return res;
    }
}

optstepresult NelderMead::do_step_expand(double param)
{
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead DEBUG] State = Expansion" << std::endl;
#endif
    fe = param;

    if (fe < fr)
    {
        // VV: EXPANDED point is better than REFLECTIVE
        //     Replace WORST with EXPANDED
        for (j = 0; j <= n - 1; j++)
        {
            v[vg][j] = ve[j];
        }
        f[vg] = fe;
    }
    else
    {
        // VV: Replace WORST with REFLECTED
        for (j = 0; j <= n - 1; j++)
        {
            v[vg][j] = vr[j];
        }
        f[vg] = fr;
    }

    state_ = start;
    
    return do_step_start(param);
}

optstepresult NelderMead::do_step_contract(double param)
{
    int j;
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead|DEBUG] State = Contraction" << std::endl;
#endif
    fc = param;

    if ( fc <= fr ) {
        // VV: CONTRACTED_O is better than REFLECTED
        //     Replace WORST with CONTRACTED_O
        for (j = 0; j <= n - 1; j++)
        {
            v[vg][j] = vc[j];
        }
        f[vg] = fc;

        return do_step_start(param);
    } else {
        // VV: Replace SECOND BEST
        for (j = 0; j <= n - 1; j++)
            v[vh][j] = v[vs][j] + DELTA * (v[vh][j] - v[vs][j]);
        
        my_constraints(v[vh]);
        // VV: Now evaluate SHRINK

        optstepresult res;
        res.threads = v[vh][0];
        res.freq_idx = v[vh][1];
        state_ = shrink;
        return res;
    }
}

optstepresult NelderMead::do_step_shrink(double param)
{
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead|DEBUG] State = Shrink" << std::endl;
#endif
    f[vh] = param;
    return do_step_start(param);
}

optstepresult NelderMead::step(double param)
{
    int i, j;

    optstepresult res;
    res.threads = 0;
    res.freq_idx = -1;

    switch (state_)
    {

    case start:
        res = do_step_start(param);
    break;
    case reflection:
        res = do_step_reflect(param);
    break;
    case expansion:
        res = do_step_expand(param);
    break;
    case contraction:
        res = do_step_contract(param);
    break;
    case shrink:
        res = do_step_shrink(param);
    break;
    default:
        std::cout << "Unknown NelderMead state (" << state_ << ")" << std::endl;
        res.converged = false;
        return res;
    }

    res.converged = testConvergence();

    if ( res.converged == true ) {
        res.threads = v[vs][0];
        res.freq_idx = v[vs][1];
        std::cout << "Converged to " << res.threads << " " << res.freq_idx << std::endl;
    }

    return res;
}

bool NelderMead::testConvergence()
{
    double temp;

    fsum = 0.0;
    for (j = 0; j <= n; j++)
    {
        fsum += f[j];
    }
    favg = fsum / (n + 1);
    s = 0.0;
    for (j = 0; j <= n; j++)
    {
        temp = (f[j] - favg);
        s += temp * temp / (n);
    }
    s = sqrt(s);
    s = s / favg; // normalization step
#ifdef NMD_INFO_
    std::cout << "[NelderMead|INFO] Convergence Ratio is " << s << std::endl;
    std::cout << "[NelderMead|INFO] Convergence Threshold set is " << EPSILON << std::endl;
#endif
    if (s >= EPSILON && itr <= MAXITERATIONS)
        return false;
    else
    {
        vs = vs_index();
        min = f[vs];
        return true;
    }
}

void NelderMead::updateObjectives()
{
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

} // namespace components
} // namespace allscale
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
