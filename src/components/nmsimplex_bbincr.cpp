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

#ifdef NMD_DEBUG_
#define OUT_DEBUG(X) X
#else
#define OUT_DEBUG(X) \
    {                \
    }
#endif
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

bool NelderMead::cache_update(int threads, int freq_idx,
                              const double objectives[], bool add_if_new)
{
    auto key = std::make_pair(threads, freq_idx);
    auto past = cache_.find(key);

    if (past != cache_.end())
    {
        auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        double abs_diff = 0;
        for (auto j = 0; j < NMD_NUM_OBJECTIVES; ++j)
        {
            abs_diff += past->second.objectives[j] - objectives[j];
            past->second.objectives[j] = objectives[j];
        }

        past->second._cache_timestamp = timestamp_now;
        // VV: Entries which remain relatively same should be explored less frequently
        if (abs_diff > 0.1)
            past->second._cache_expires_dt = CACHE_EXPIRE_AFTER_MS;
        else if (past->second._cache_expires_dt < CACHE_EXPIRE_AFTER_MS * 1024)
            past->second._cache_expires_dt *= 2;

        return true;
    }
    else if (add_if_new)
    {
        auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        optstepresult entry;
        entry._cache_timestamp = timestamp_now;
        entry._cache_expires_dt = CACHE_EXPIRE_AFTER_MS;
        entry.threads = threads;
        entry.freq_idx = freq_idx;

        for (auto j = 0; j < NMD_NUM_OBJECTIVES; ++j)
            entry.objectives[j] = objectives[j];

        cache_.insert(std::make_pair(key, entry));

        return true;
    }

    return false;
}

double NelderMead::evaluate_score(const double objectives[], const double *weights) const
{
    double score = 0.0f;
    // VV: [time, energy/power, resources]
    double scale[] = {1.0, 1000.0, 1.0};
    scale[2] = (double)constraint_max[0];

    if (weights == nullptr)
        weights = opt_weights;

    for (auto i = 0; i < NMD_NUM_OBJECTIVES; ++i)
    {
        double t = objectives[i] / scale[i];
        score += t * t * weights[i];
    }

    return score;
}

void NelderMead::set_weights(double weights[3])
{
    opt_weights[0] = weights[0];
    opt_weights[1] = weights[1];
    opt_weights[2] = weights[2];
    OUT_DEBUG(
        std::cout << "[NelderMead|DEBUG] Weights: " 
                << opt_weights[0] << " "
                << opt_weights[1] << " "
                << opt_weights[2] << std::endl;
    )
}

/* FIXME: generalize */
void NelderMead::initialize_simplex(double params[][2],
                                    double objectives[][3],
                                    double weights[3],
                                    double constraint_min[2],
                                    double constraint_max[2])
{
    int i, j;
    long timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    for (i = 0; i < NMD_NUM_KNOBS; i++)
    {
        this->constraint_min[i] = constraint_min[i];
        this->constraint_max[i] = constraint_max[i];
    }

    set_weights(weights);

    // VV: Need num_knobs +1
    for (i = 0; i < NMD_NUM_KNOBS + 1; i++)
    {
        f[i] = evaluate_score(objectives[i], weights);

        for (j = 0; j < n; j++)
        {
            v[i][j] = params[i][j];
        }

        my_constraints(v[i]);

        optstepresult entry;
        entry.threads = round(v[i][0]);
        entry.freq_idx = round(v[i][1]);

        // VV: Check if we can re-use a previously explored configuration
        auto key = std::make_pair(entry.threads, entry.freq_idx);

        auto past_entry = cache_.find(std::make_pair(entry.threads,
                                                     entry.freq_idx));
        if (past_entry != cache_.end())
        {
            for (j = 0; j < NMD_NUM_OBJECTIVES; ++j)
                past_entry->second.objectives[j] = objectives[i][j];

            past_entry->second._cache_timestamp = timestamp_now;
            // VV: Skip attempting to re-insert the "same" entry
            continue;
        }

        // VV: If we've reached this point we need to add the entry to the cache
        for (j = 0; j < NMD_NUM_OBJECTIVES; ++j)
            entry.objectives[j] = objectives[i][j];

        entry._cache_timestamp = timestamp_now;
        entry._cache_expires_dt = CACHE_EXPIRE_AFTER_MS;

        cache_.insert(std::make_pair(key, entry));
    }
    itr = 0;

    state_ = start;
}

/* print out the initial values */
void NelderMead::print_initial_simplex()
{
    int i, j;
    std::cout << "[NelderMead DEBUG] Initial Values\n";
    
    for (j = 0; j < NMD_NUM_KNOBS + 1; j++)
    {
        
        for (i = 0; i < NMD_NUM_KNOBS; i++)
        {
            std::cout << v[j][i] << ",";
        }
        const int threads = (int) v[j][0];
        const int freq_idx = (int) v[j][1];

        auto e = cache_.find(std::make_pair(threads, freq_idx));
        std::cout << " Objective value = " << f[j];

        if ( e == cache_.end() )
        {
            std::cout << " (not in cache)" << std::endl;
        } else {
            std::cout << " OBJs: "
                     << e->second.objectives[0] << " "
                     << e->second.objectives[1] << " "
                     << e->second.objectives[2] << " "
                     << std::endl;
        }
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
    if (vg != vs)
    {
        vh = 1 + 2 + 4 - (1 << vg) - (1 << vs);
        vh = map_to_index[vh];
    }
    else
    {
        vg = 2;
        vh = 1;
        vs = 0;
    }
}

bool NelderMead::knob_set_exists(double knobs[2], int exclude)
{
    int is_same;

    for (auto i=0; i<NMD_NUM_KNOBS+1; ++i) {
        if ( i != exclude ) {
            is_same = 1;
            for ( auto j=0; j<NMD_NUM_KNOBS; ++j ) 
                is_same &= (v[i][j] == knobs[j]);
            
            if ( is_same )
                return true;
        }
    }

    return false;
}

optstepresult NelderMead::do_step_start()
{
    optstepresult res;

    itr++;
    OUT_DEBUG(
        std::cout << "[NelderMead DEBUG] State = Start" << std::endl;
        print_initial_simplex();)

    sort_vertices();

    centroid();
    double extra[2] = {0.0, 0.0};
    int is_invalid = 0;
    int max_combinations = 0;

    max_combinations = (constraint_max[0] - constraint_min[0]+1) * (constraint_max[1] - constraint_min[1]+1);


    // VV: Try not to pick a knob_set that already exists in `v`
    do {
        for (j = 0; j < NMD_NUM_KNOBS; j++)
            vr[j] = vm[j] + ALPHA * (vm[j] - v[vg][j]) + extra[j];
        
        my_constraints(vr);
        
        is_invalid = 0;

        if ( max_combinations > NMD_NUM_KNOBS +1 ) {
            is_invalid = knob_set_exists(vr, -1);

            if ( is_invalid ) {
                extra[0] = rand() % (int)(constraint_max[0] - constraint_min[0])
                            + (int) constraint_min[0]
                            - (int)(0.5*(constraint_max[0] - constraint_min[0]));

                extra[1] = rand() % (int)(constraint_max[1] - constraint_min[1])
                            + (int) constraint_min[1]
                            - (int)(0.5*(constraint_max[1] - constraint_min[1]));
                
            }
        } 
        
    } while ( is_invalid );

#ifdef NMD_DEBUG_
    std::cout << "[NelderMead DEBUG] Reflection Parameter = ("
              << vr[0] << "," << vr[1] << ")"
              << std::endl;
#endif
    // enter reflection state
    state_ = reflection;
    res.threads = vr[0];
    res.freq_idx = vr[1];

    auto key = std::make_pair(res.threads, res.freq_idx);

    auto entry = cache_.find(key);

    if (entry != cache_.end())
    {
        auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        auto dt = timestamp_now - entry->second._cache_timestamp;

        if (dt < entry->second._cache_expires_dt)
        {
            return do_step_reflect(entry->second.objectives);
        }
    }

    return res;
}

optstepresult NelderMead::do_step_reflect(const double objectives[])
{
    optstepresult res;
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead DEBUG] State = Reflection" << std::endl;
#endif
    fr = evaluate_score(objectives, opt_weights);

    if ((f[vs] <= fr) && (fr < f[vh]))
    {
        // VV: REFLECTED point is better than the SECOND BEST
        //     but NOT better than the BEST
        //     Replace WORST point with REFLECTED
        for (j = 0; j <= n - 1; j++)
        {
            v[vg][j] = vr[j];
        }

        my_constraints(v[vg]);

        f[vg] = fr;

        const int threads = (int)(v[vg][0]);
        const int freq_idx = (int)(v[vg][1]);

        cache_update(threads, freq_idx, objectives, true);

        state_ = start;
        return do_step_start();
    }
    else if (fr < f[vs])
    {
        // VV: REFLECTED is better than BEST

        double extra[2] = {0.0, 0.0};
        int is_invalid = 0;
        int max_combinations = 0;

        max_combinations = (constraint_max[0] - constraint_min[0]+1) * (constraint_max[1] - constraint_min[1]+1);

        // VV: Try not to pick a knob_set that already exists in `v`
        do {
            for (j = 0; j < NMD_NUM_KNOBS; j++)
                ve[j] = vm[j] + GAMMA * (vr[j] - vm[j]) + extra[j];
            
            my_constraints(ve);
            
            is_invalid = 0;

            if ( max_combinations > NMD_NUM_KNOBS +1 ) {
                is_invalid = knob_set_exists(ve, -1);

                if ( is_invalid ) {
                    extra[0] = rand() % (int)(constraint_max[0] - constraint_min[0])
                                + (int) constraint_min[0]
                                - (int)(0.5*(constraint_max[0] - constraint_min[0]));

                    extra[1] = rand() % (int)(constraint_max[1] - constraint_min[1])
                                + (int) constraint_min[1]
                                - (int)(0.5*(constraint_max[1] - constraint_min[1]));
                    
                }
            } 
            
        } while ( is_invalid );

        // VV: Now evaluate EXPANDED
        res.threads = ve[0];
        res.freq_idx = ve[1];

        state_ = expansion;

        auto key = std::make_pair(res.threads, res.freq_idx);

        auto entry = cache_.find(key);

        if (entry != cache_.end())
        {
            auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
            auto dt = timestamp_now - entry->second._cache_timestamp;

            if (dt < entry->second._cache_expires_dt)
            {
                return do_step_expand(entry->second.objectives);
            }
        }

        return res;
    }
    else if ((f[vh] <= fr) && (fr < f[vg]))
    {
        // VV: REFLECTED between SECOND BEST and WORST
        double extra[2] = {0.0, 0.0};
        int is_invalid = 0;
        int max_combinations = 0;

        max_combinations = (constraint_max[0] - constraint_min[0]+1) * (constraint_max[1] - constraint_min[1]+1);

        // VV: Try not to pick a knob_set that already exists in `v`
        do {
            for (j = 0; j < NMD_NUM_KNOBS; j++)
                vc[j] = vm[j] + BETA * (vr[j] - vm[j]) + extra[j];
            
            my_constraints(vc);
            
            is_invalid = 0;

            if ( max_combinations > NMD_NUM_KNOBS +1 ) {
                is_invalid = knob_set_exists(vc, -1);

                if ( is_invalid ) {
                    extra[0] = rand() % (int)(constraint_max[0] - constraint_min[0])
                                + (int) constraint_min[0]
                                - (int)(0.5*(constraint_max[0] - constraint_min[0]));

                    extra[1] = rand() % (int)(constraint_max[1] - constraint_min[1])
                                + (int) constraint_min[1]
                                - (int)(0.5*(constraint_max[1] - constraint_min[1]));
                    
                }
            } 
            
        } while ( is_invalid );

        // VV: Now evaluate EXPANDED
        res.threads = vc[0];
        res.freq_idx = vc[1];

        state_ = contraction;

        auto key = std::make_pair(res.threads, res.freq_idx);

        auto entry = cache_.find(key);

        if (entry != cache_.end())
        {
            auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
            auto dt = timestamp_now - entry->second._cache_timestamp;

            if (dt < entry->second._cache_expires_dt)
            {
                return do_step_contract(entry->second.objectives);
            }
        }

        return res;
    }
    else
    {
        // VV: REFLECTED worse than WORST
        double extra[2] = {0.0, 0.0};
        int is_invalid = 0;
        int max_combinations = 0;

        max_combinations = (constraint_max[0] - constraint_min[0]+1) * (constraint_max[1] - constraint_min[1]+1);

        // VV: Try not to pick a knob_set that already exists in `v`
        do {
            for (j = 0; j < NMD_NUM_KNOBS; j++)
                vc[j] = vm[j] - BETA * (vr[j] - vm[j]) + extra[j];
            
            my_constraints(vc);
            
            is_invalid = 0;

            if ( max_combinations > NMD_NUM_KNOBS +1 ) {
                is_invalid = knob_set_exists(vc, -1);

                if ( is_invalid ) {
                    extra[0] = rand() % (int)(constraint_max[0] - constraint_min[0])
                                + (int) constraint_min[0]
                                - (int)(0.5*(constraint_max[0] - constraint_min[0]));

                    extra[1] = rand() % (int)(constraint_max[1] - constraint_min[1])
                                + (int) constraint_min[1]
                                - (int)(0.5*(constraint_max[1] - constraint_min[1]));
                    
                }
            } 
            
        } while ( is_invalid );

        // VV: Now evaluate EXPANDED
        res.threads = vc[0];
        res.freq_idx = vc[1];

        state_ = contraction;
        auto key = std::make_pair(res.threads, res.freq_idx);

        auto entry = cache_.find(key);

        if (entry != cache_.end())
        {
            auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
            auto dt = timestamp_now - entry->second._cache_timestamp;

            if (dt < entry->second._cache_expires_dt)
            {
                return do_step_contract(entry->second.objectives);
            }
        }

        return res;
    }
}

optstepresult NelderMead::do_step_expand(const double objectives[])
{
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead DEBUG] State = Expansion" << std::endl;
#endif
    fe = evaluate_score(objectives, nullptr);

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
    const int threads = (int)(v[vg][0]);
    const int freq_idx = (int)(v[vg][1]);

    cache_update(threads, freq_idx, objectives, true);
    return do_step_start();
}

optstepresult NelderMead::do_step_contract(const double objectives[])
{
    int j;
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead|DEBUG] State = Contraction" << std::endl;
#endif
    fc = evaluate_score(objectives, nullptr);

    if (fc <= fr)
    {
        // VV: CONTRACTED_O is better than REFLECTED
        //     Replace WORST with CONTRACTED_O
        for (j = 0; j <= n - 1; j++)
        {
            v[vg][j] = vc[j];
        }
        f[vg] = fc;

        const int threads = (int)(v[vg][0]);
        const int freq_idx = (int)(v[vg][1]);

        cache_update(threads, freq_idx, objectives, true);
        return do_step_start();
    }
    else
    {
        // VV: Replace SECOND BEST
        double new_vh[NMD_NUM_KNOBS];
        double extra[NMD_NUM_KNOBS] = {0.0, 0.0};
        int is_invalid = 0;
        int max_combinations = 0;

        max_combinations = (constraint_max[0] - constraint_min[0]+1) * (constraint_max[1] - constraint_min[1]+1);

        // VV: Try not to pick a knob_set that already exists in `v`
        do {
            for (j = 0; j < NMD_NUM_KNOBS; j++)
                new_vh[j] = v[vs][j] + DELTA * (v[vh][j] - v[vs][j]) + extra[j];
            
            my_constraints(new_vh);
            
            is_invalid = 0;

            if ( max_combinations > NMD_NUM_KNOBS +1 ) {
                is_invalid = knob_set_exists(new_vh, -1);

                if ( is_invalid ) {
                    extra[0] = rand() % (int)(constraint_max[0] - constraint_min[0])
                                + (int) constraint_min[0]
                                - (int)(0.5*(constraint_max[0] - constraint_min[0]));

                    extra[1] = rand() % (int)(constraint_max[1] - constraint_min[1])
                                + (int) constraint_min[1]
                                - (int)(0.5*(constraint_max[1] - constraint_min[1]));
                    
                }
            } 
            
        } while ( is_invalid );

        for (j = 0; j < NMD_NUM_KNOBS; j++)
            v[vh][j] = new_vh[j];

        // VV: Now evaluate SHRINK

        optstepresult res;
        res.threads = v[vh][0];
        res.freq_idx = v[vh][1];
        state_ = shrink;

        auto key = std::make_pair(res.threads, res.freq_idx);

        auto entry = cache_.find(key);

        if (entry != cache_.end())
        {
            auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
            auto dt = timestamp_now - entry->second._cache_timestamp;

            if (dt < entry->second._cache_expires_dt)
            {
                return do_step_shrink(entry->second.objectives);
            }
        }

        return res;
    }
}

optstepresult NelderMead::do_step_shrink(const double objectives[])
{
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead|DEBUG] State = Shrink" << std::endl;
#endif
    f[vh] = evaluate_score(objectives, nullptr);

    const int threads = (int)(v[vh][0]);
    const int freq_idx = (int)(v[vh][1]);

    cache_update(threads, freq_idx, objectives, true);

    return do_step_start();
}

optstepresult NelderMead::step(const double objectives[])
{
    int i, j;

    optstepresult res;
    res.threads = 0;
    res.freq_idx = -1;
    std::cout << "Starting step with "
                << objectives[0] << " " 
                << objectives[1] << " " 
                << objectives[2] << std::endl;
    
    switch (state_)
    {

    case start:
        res = do_step_start();
        break;
    case reflection:
        res = do_step_reflect(objectives);
        break;
    case expansion:
        res = do_step_expand(objectives);
        break;
    case contraction:
        res = do_step_contract(objectives);
        break;
    case shrink:
        res = do_step_shrink(objectives);
        break;
    default:
        std::cout << "Unknown NelderMead state (" << state_ << ")" << std::endl;
        res.converged = false;
        return res;
    }

    res.converged = testConvergence();

    if (res.converged == true)
    {
        res.threads = v[vs][0];
        res.freq_idx = v[vs][1];
        std::cout << "Converged to " << res.threads << " " << res.freq_idx << std::endl;
    }
    std::cout << "Stop step with "
                << objectives[0] << " " 
                << objectives[1] << " " 
                << objectives[2] << std::endl;

    return res;
}

bool NelderMead::testConvergence()
{
    double temp;
    #if 0
    int all_same = 1;

    for (auto i = 0; i <= n; ++i)
    {
        for (auto k = i + 1; j <= n; ++k)
            for (auto j = 0; j < n; ++j)
                all_same &= (v[i][j] == v[k][j]);
    }

    if (all_same)
    {
        min = f[vs];
        return true;
    }
    #endif

    fsum = 0.0;
    for (auto j = 0; j <= n; j++)
    {
        fsum += f[j];
    }
    favg = fsum / (n + 1);
    s = 0.0;
    for (auto j = 0; j <= n; j++)
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
        sort_vertices();
        min = f[vs];
        return true;
    }
}

} // namespace components
} // namespace allscale
