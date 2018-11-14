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
    state_ = warmup;

    /* dynamically allocate arrays */

    /* allocate the rows of the arrays */
    v = (double **)malloc((n + 1) * sizeof(double *));
    f = (double *)malloc((n + 1) * sizeof(double));
    vr = (double *)malloc(n * sizeof(double));
    ve = (double *)malloc(n * sizeof(double));
    vc = (double *)malloc(n * sizeof(double));
    vm = (double *)malloc(n * sizeof(double));
    
    warming_up_step = 0;

    /* allocate the columns of the arrays */
    for (i = 0; i <= n; i++)
    {
        v[i] = (double *)malloc(n * sizeof(double));
    }
}

std::pair<int, NelderMead::direction> NelderMead::explore_next_extra(double *extra, int level, 
                                direction dir, 
                                int level_max, int level_nested_max)
{
    /*
    const char *to_string[] = {
        "up", "up_final", "down", "left", "right", "right_final"
    };
    */
    if ( extra[0] == 0.0 && extra[1] == 0.0 ) {
        extra[1] = 1.0;

        return std::make_pair(level, dir);
    }
    switch (dir) {
        case (direction::up):
            if ( extra[1] < level ) {
                extra[1] += 1.;
            } else if( extra[0] < level_nested_max ) {
                extra[0] += 1.;
                dir = direction::right;
            } else {
                level ++;
            }
        break;

        case (direction::up_final):
            if ( extra[1] < level ) {
                extra[1] += 1.;
            } else if( extra[0] < level_nested_max ) {
                extra[0] += 1.;
                dir = direction::right_final;
            } else {
                level ++;
            }
        break;


        case (direction::down):
            if ( extra[1] > -level ) {
                extra[1] -= 1.0;
            } else if ( extra[0] > -level_nested_max ){
                extra[0] -= 1.0;
                dir = direction::left;
            }
        break;

        case (direction::left):
            if ( extra[0] > -level_nested_max ) {
                extra[0] -= 1.0;
            } else if (extra[1] < level ) {
                extra[1] += 1.0;
                dir = direction::up_final;
            }
        break;

        case (direction::right):
            if ( extra[0] < level_nested_max ) {
                extra[0] += 1.;
            } else if ( extra[1] <= level ) {
                extra[1] -= 1.;
                dir = direction::down;
            }
        break;
        
        case (direction::right_final):
        if ( extra[0] < 0. ) {
            extra[0] += 1.;
        } else {
            level ++; 
            extra[0] = 0.0;
            extra[1] = level;
            dir = direction::right;
        }
        break;
    }

    return std::make_pair(level, dir);
}

template <typename F>
void NelderMead::generate_new(F &gen)
{
    double extra[] = {0, 0};
    double *new_set;
    int i = 0;
    int max_combinations = (constraint_max[0] - constraint_min[0]+1) 
                            * (constraint_max[1] - constraint_min[1]+1);
    int level = 1;
    int max_nested_level = constraint_max[1] - constraint_min[1] +1;
    int max_level = constraint_max[0] - constraint_min[0] +1;
    direction dir = direction::right;

    // VV: Search for a twice as big space to take into account that
    //     new_set is not *actually* at 0, 0

    max_level *= 2;
    max_nested_level *=2;

    int is_same;
    do
    {
        new_set = gen(extra);
        
        auto key = std::make_pair((int)new_set[0], (int)new_set[1]);
        auto entry = cache_.find(key);
        is_same = (entry != cache_.end());

        if ( ( level < max_level +1) 
             && is_same 
             && max_combinations > (NMD_NUM_KNOBS + 1))
        {
            # if 0
            extra[0] = rand() % (int)(constraint_max[0] - constraint_min[0]) 
                            + (int)constraint_min[0] 
                            - (int)(0.5 * (constraint_max[0] - constraint_min[0]));

            extra[1] = rand() % (int)(constraint_max[1] - constraint_min[1]) 
                                + (int)constraint_min[1] 
                                - (int)(0.5 * (constraint_max[1] - constraint_min[1]));
            #else
            auto logistics = explore_next_extra(extra, level, dir, 
                                                max_level, max_nested_level);
            level = logistics.first;
            dir = logistics.second;

            #endif
            OUT_DEBUG(
                std::cout << "[NelderMead|Debug] Rejecting " 
                    << new_set[0] << " " << new_set[1] <<  std::endl;
            )
        } else {
            break;
        }
    } while ( 1 );
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
    double score;
    // VV: [time, energy/power, resources]
    double scale[] = {1.0, 1000.0, 1.0};
    scale[2] = (double)constraint_max[0];

    if (weights == nullptr)
        weights = opt_weights;

    #if 0
    score = 0.0;
    for (auto i = 0; i < NMD_NUM_OBJECTIVES; ++i)
    {
        double t = objectives[i] / scale[i];
        score += t * t * weights[i];
    }
    #else 
    score = 0.0;
    for ( auto i=0; i<NMD_NUM_OBJECTIVES; ++ i) {
        score += exp(weights[i]*objectives[i]/scale[i]);
    }
    #endif
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
void NelderMead::initialize_simplex(double weights[3],
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
    state_ = warmup;
    itr = 0;
    warming_up_step = 0;
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

optstepresult NelderMead::do_step_start()
{
    optstepresult res;

    OUT_DEBUG(
        std::cout << "[NelderMead DEBUG] State = Start" << std::endl;
        print_initial_simplex();
    )

    sort_vertices();

    centroid();   

    // VV: Try not to pick a knob_set that already exists in `v`
    auto gen_new = [this](double *extra) mutable -> double* {
        
        for (j = 0; j < NMD_NUM_KNOBS; j++)
            vr[j] = vm[j] + ALPHA * (vm[j] - v[vg][j]) + extra[j];
       
        my_constraints(vr);

        return vr;
    };
    
    generate_new(gen_new);

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
        auto gen_new = [this](double *extra) mutable -> double* {
            for (j = 0; j < NMD_NUM_KNOBS; j++)
                ve[j] = vm[j] + GAMMA * (vr[j] - vm[j]) + extra[j];
                
            my_constraints(ve);

            return ve;
        };
    
        generate_new(gen_new);

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
        auto gen_new = [this](double *extra) mutable -> double* {
            for (j = 0; j < NMD_NUM_KNOBS; j++)
                vc[j] = vm[j] + BETA * (vr[j] - vm[j]) + extra[j];
                
            my_constraints(vc);

            return vc;
        };

        generate_new(gen_new);  
        
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
        auto gen_new = [this](double *extra) mutable -> double* {
            for (j = 0; j < NMD_NUM_KNOBS; j++)
                vc[j] = vm[j] - BETA * (vr[j] - vm[j]) + extra[j];
                
            my_constraints(vc);

            return vc;
        };

        generate_new(gen_new);

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
        
        auto gen_new = [this, &new_vh](double *extra) mutable -> double* {
            for (auto j = 0; j < NMD_NUM_KNOBS; j++)
                new_vh[j] = v[vs][j] + DELTA * (v[vh][j] - v[vs][j]) + extra[j];
                
            my_constraints(new_vh);

            return new_vh;
        };

        generate_new(gen_new);

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
    OUT_DEBUG(
        std::cout << "[NelderMead|DEBUG] Starting step with "
            << objectives[0] << " " 
            << objectives[1] << " " 
            << objectives[2] << std::endl;
    )
    
    std::size_t tested_combinations = cache_.size();

    switch (state_)
    {
    case warmup:
    {
        #ifdef NMD_DEBUG_
            std::cout << "[NelderMead|DEBUG] State = Warmup " 
                      << warming_up_step << std::endl;
        #endif
        if ( warming_up_step > 0 ) {
            // VV: Record results of last warming up step
            f[warming_up_step-1] = evaluate_score(objectives, nullptr);
            cache_update(v[warming_up_step-1][0], v[warming_up_step-1][1], 
                         objectives, true);
        }

        if ( warming_up_step == NMD_NUM_KNOBS + 1) {
            // VV: We need not explore the knob_set space anymore
            state_ = start;
            return step(objectives);
        }

        // VV: Start at 25% threads with lowest CPU Freq, then 75% threads with max freq
        //     and 100% threads with max freq

        int threads_low = round(0.25 * (constraint_max[0] - constraint_min[1]) 
                    + constraint_min[1]);
        int threads_med = round(0.75 * (constraint_max[0] - constraint_min[1])
                    + constraint_min[1]);
        int threads_max = constraint_max[0];

        const int initial_configurations[NMD_NUM_KNOBS+1][NMD_NUM_KNOBS] = {
            {threads_low, (int)constraint_min[1]},
            {threads_med, (int)constraint_max[1]},
            {threads_max, (int)constraint_max[1]},
        };

        optstepresult res;
        res.objectives[0] = -1;
        res.objectives[1] = -1;
        res.objectives[2] = -1;
        res.converged = false;
        res.score = -1;
        res.threads = initial_configurations[warming_up_step][0];
        res.freq_idx = initial_configurations[warming_up_step][1];
        
        v[warming_up_step][0] = res.threads;
        v[warming_up_step][1] = res.freq_idx;
        warming_up_step++;

        return res;
    }
    break;
    case start:
        itr++;
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

    res.converged = testConvergence(tested_combinations);

    if (res.converged == true)
    {
        res.threads = v[vs][0];
        res.freq_idx = v[vs][1];
        OUT_DEBUG(
            std::cout << "[NelderMead|DEBUG] Converged to " << res.threads << " " << res.freq_idx << std::endl;
        )
    }

    std::cout << "Stop step with "
                << objectives[0] << " " 
                << objectives[1] << " " 
                << objectives[2] << std::endl;

    return res;
}

bool NelderMead::testConvergence(std::size_t tested_combinations)
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
    int max_combinations = (constraint_max[0] - constraint_min[0]+1) 
                            * (constraint_max[1] - constraint_min[1]+1);

    if ( (s >= EPSILON)
        && (itr <= MAXITERATIONS)
        && (max_combinations != tested_combinations) )
        return false;
    else
    {
        sort_vertices();
        min = f[vs];

        OUT_DEBUG(
            std::cout << "[NelderMead|Debug] Cache_ Max: " << max_combinations 
                        << " explored " << tested_combinations << std::endl;
            for (const auto &entry: cache_ ) {
                std::cout << "[NelderMead|Debug] Cache_ " 
                    << entry.second.threads << " " 
                    << entry.second.freq_idx << " :: "
                    << entry.second.objectives[0] << " "
                    << entry.second.objectives[1] << " "
                    << entry.second.objectives[2] << " :: "
                    << evaluate_score(entry.second.objectives, nullptr) << std::endl;
            }
        )

        return true;
    }
}

} // namespace components
} // namespace allscale
