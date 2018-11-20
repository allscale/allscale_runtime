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
#include <cmath>

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


NelderMead::NelderMead(const NelderMead &other)
{
    EPSILON = other.EPSILON;
    state_ = other.state_;
    
    cache_.insert(other.cache_.begin(), other.cache_.end());
    warming_up_step = other.warming_up_step;
    convergence_reevaluating = other.convergence_reevaluating;

    fc = other.fc;
    fe = other.fe;
    vs = other.vs;
    vg = other.vg;
    vh = other.vh;

    for (auto i=0; i<NMD_NUM_KNOBS; ++i) {
        constraint_max[i] = other.constraint_max[i];
        constraint_min[i] = other.constraint_min[i];
        vr[i] = other.vr[i];
        ve[i] = other.ve[i];
        vm[i] = other.vm[i];
    }

    for ( auto i=0; i<NMD_NUM_OBJECTIVES; ++i ) {
        opt_weights[i] = other.opt_weights[i];
        scale[i] = other.scale[i];
    }

    for (auto i=0; i<NMD_NUM_KNOBS+1; ++i )
    {
        for ( auto j=0; j<NMD_NUM_KNOBS; ++j ) {
            v[i][j] = other.v[i][j];
            initial_configurations[i][j] = other.initial_configurations[i][j];
        }
    }

    should_update_constraints = true;
}

//NelderMead::NelderMead(double (*objfunc)(double[]),double eps){
NelderMead::NelderMead(double eps)
{

    EPSILON = eps;
#ifdef NMD_INFO_
    std::cout << "[NelderMead|INFO] Initial Convergence Threshold set is " << EPSILON << std::endl;
#endif
    itr = 0;
    state_ = warmup;
    
    warming_up_step = 0;
    convergence_reevaluating = false;

    for (auto i=0ul; i<NMD_NUM_OBJECTIVES; ++i)
        scale[i] = 1.0;
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
    auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    // VV: Restrict search-grid to a maximum block of 5x5
    int retries = 0;
    const int retries_threshold = 5*5;
    int is_same;
    do
    {
        new_set = gen(extra);
        
        auto key = std::make_pair((int)new_set[0], (int)new_set[1]);
        auto entry = cache_.find(key);
        
        is_same = 0;

        if ( entry != cache_.end() ) {
            auto dt = timestamp_now - entry->second._cache_timestamp;
            is_same = dt <= entry->second._cache_expires_dt;
        }
        
        ++ retries;
        if ( ( level < max_level +1) 
             && is_same 
             && max_combinations > (NMD_NUM_KNOBS + 1)
             && retries < retries_threshold )
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
                    << new_set[0] << " " << new_set[1] 
                    << " will try offset " << extra[0] << " " << extra[1] <<  std::endl;
            )
        } else {
            break;
        }
    } while ( 1 );

    if ( retries >= retries_threshold ) {
        extra[0] = 0;
        extra[1] = 0;

        gen(extra);
    }
}

void NelderMead::my_constraints(double x[])
{
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

void NelderMead::invalidate_cache()
{
    should_invalidate_cache = true;
}

void NelderMead::reevaluate_scores()
{
    should_reevaluate_scores = true;
}

void NelderMead::do_invalidate_cache()
{
    cache_.clear();
    should_invalidate_cache = false;
}

void NelderMead::do_reevaluate_scores()
{
    auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    for (auto i=0ul; i<NMD_NUM_KNOBS+1; ++i )
    {
        auto key = std::make_pair( (int)v[i][0], (int)v[i][1] );
        auto entry = cache_.find(key);

        if ( entry != cache_.end() ) {
            f[i] = evaluate_score(entry->second.objectives, opt_weights);
        }
    }

    should_reevaluate_scores = false;

    sort_vertices();
    centroid();
}

void NelderMead::set_scale(const double scale[NMD_NUM_OBJECTIVES])
{
    for ( auto i=0ul; i<NMD_NUM_OBJECTIVES; ++i )
        this->scale[i] = scale[i];
    
    reevaluate_scores();
}

double NelderMead::evaluate_score(const double objectives[], const double *weights)
{
    double score;
    // VV: [time, energy/power, resources]
    
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
    score = 1.0;
    for ( auto i=0; i<NMD_NUM_OBJECTIVES; ++ i) {
        score *= std::exp(weights[i]*objectives[i]/scale[i]);
    }
    #endif
    return score;
}

void NelderMead::set_weights(const double weights[3])
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
#if 0
void NelderMead::initialize_simplex(const double weights[3],
                                    const double constraint_min[2],
                                    const double constraint_max[2])
{
    int i, j;
    auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    for (i = 0; i < NMD_NUM_KNOBS; i++)
    {
        this->constraint_min[i] = constraint_min[i];
        this->constraint_max[i] = constraint_max[i];
    }

    OUT_DEBUG(
        std::cout << "[NelderMead|Debug] Initialize contraints " << std::endl;
        std::cout << constraint_min[0] 
                    << ":" << constraint_max[0] << std::endl;
        std::cout << constraint_min[1] 
                    << ":" << constraint_max[1] << std::endl;
    )

    set_weights(weights);
    state_ = warmup;
    itr = 0;
    warming_up_step = 0;
    convergence_reevaluating = false;
    cache_.clear();

    for (i=0; i<NMD_NUM_KNOBS+1; ++i) {
        int is_ok = 1;
        do {
            
            for (j=0; j<NMD_NUM_KNOBS; ++j)
                initial_configurations[i][j] = constraint_min[j] + rand() % (int) (constraint_max[j] - constraint_min[j]+1);
            
            is_ok = 1;

            for (auto c=0; c<i && is_ok == 1; ++c)
            {
                is_ok = 0;
                for ( j=0; j<NMD_NUM_KNOBS; ++j )
                    is_ok |= (initial_configurations[c][j] != initial_configurations[i][j]);
            }

        } while (is_ok == 0);

        OUT_DEBUG(
            std::cout << "[NelderMead|DEBUG] Random initial simplex [" << i << "]: ";
            for ( j =0; j<NMD_NUM_KNOBS; ++j) 
                std::cout << initial_configurations[i][j] << " ";
            std::cout << std::endl;
        )
    }
}
#endif

void NelderMead::update_constraints(const double constraint_min[NMD_NUM_KNOBS],
							    const double constraint_max[NMD_NUM_KNOBS])
{
    for (auto i=0; i<NMD_NUM_KNOBS; ++i) {
        next_constraint_min[i] = constraint_min[i];
        next_constraint_max[i] = constraint_max[i];
    }

    should_update_constraints = true;
}

/* FIXME: generalize */
void NelderMead::initialize_simplex(const double weights[3],
                                    const double initial_simplex[][NMD_NUM_KNOBS],
                                    const double constraint_min[2],
                                    const double constraint_max[2])
{
    int i, j;
    long timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    update_constraints(constraint_min, constraint_max);

    OUT_DEBUG(
        std::cout << "[NelderMead|Debug] Initialize contraints " << std::endl;
        std::cout << constraint_min[0] 
                    << ":" << constraint_max[0] << std::endl;
        std::cout << constraint_min[1] 
                    << ":" << constraint_max[1] << std::endl;
    )

    set_weights(weights);
    state_ = warmup;
    itr = 0;
    warming_up_step = 0;
    convergence_reevaluating = false;
    cache_.clear();
    if (initial_simplex == nullptr)
    {
        #if 0
        int threads_low = round(0.25 * (constraint_max[0] - constraint_min[1]) 
                    + constraint_min[1]);
        int threads_med = round(0.5 * (constraint_max[0] - constraint_min[1])
                    + constraint_min[1]);
        int threads_high = constraint_max[0] * 0.75;

        initial_configurations[0][0] = threads_low;
        initial_configurations[0][1] = (int)constraint_min[1];

        initial_configurations[1][0] = threads_high;
        initial_configurations[1][1] = (int)constraint_min[1];

        initial_configurations[2][0] = threads_high;
        initial_configurations[2][1] = (int)constraint_max[1];
        #else
        for (i=0; i<NMD_NUM_KNOBS+1; ++i) {
            int is_ok = 1;
            do {
                
                for (j=0; j<NMD_NUM_KNOBS; ++j)
                    initial_configurations[i][j] = constraint_min[j] + rand() % (int) (constraint_max[j] - constraint_min[j]+1);
                
                is_ok = 1;

                for (auto c=0; c<i && is_ok == 1; ++c)
                {
                    is_ok = 0;
                    for ( j=0; j<NMD_NUM_KNOBS; ++j )
                        is_ok |= (initial_configurations[c][j] != initial_configurations[i][j]);
                }

            } while (is_ok == 0);

            OUT_DEBUG(
                std::cout << "[NelderMead|DEBUG] Random initial simplex [" << i << "]: ";
                for ( j =0; j<NMD_NUM_KNOBS; ++j) 
                    std::cout << initial_configurations[i][j] << " ";
                std::cout << std::endl;
            )
        }
        #endif
    } else {
        double knob_set[NMD_NUM_KNOBS];
        for (i=0; i<NMD_NUM_KNOBS+1; ++i ) {
            for (j=0; j<NMD_NUM_KNOBS; ++j ) {
                knob_set[j] = initial_simplex[i][j];
            }
            my_constraints(knob_set);
            for (j=0; j<NMD_NUM_KNOBS; ++j ) {
                initial_configurations[i][j] = (int) knob_set[j];
            }
        }
    }
}

/* print out the initial values */
void NelderMead::print_initial_simplex()
{
    int i, j;
    std::cout << "[NelderMead DEBUG] Initial Values (Order indices:" 
        << vs << ", " << vh << ", " << vg << ")" << std::endl;

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
        std::cout << std::flush;
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

    OUT_DEBUG (
        std::cout << "[NelderMead|DEBUG] New Centroid: " 
        << vm[0] << " " << vm[1] << std::endl;
    )
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
            vr[j] = vm[j] + ALPHA * (vm[j] - v[vg][j]) - extra[j];
       
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
            return do_step_reflect(entry->second.objectives,
                    entry->second.threads,
                    entry->second.freq_idx);
        }
    }

    return res;
}

optstepresult NelderMead::do_step_reflect(const double objectives[], 
            double knob1, double knob2)
{
    optstepresult res;
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead DEBUG] State = Reflection" << std::endl;
#endif
    // VV: Make sure that we actually profiled what we meant to
    double profiled[] = {knob1, knob2};
    my_constraints(profiled);

    if ( vr[0] != profiled[0] || vr[1] != profiled[1] ) {
        std::cout << "[NelderMead|WARN] Meant to profile " << vr[0] << " knob1 "
                     "but ended up using " << profiled[0] << std::endl;
        std::cout << "[NelderMead|WARN] Meant to profile " << vr[1] << " knob2 "
                     "but ended up using " << profiled[1] << std::endl;
        
        auto key = std::make_pair((int)vr[0], (int)vr[1]);
        auto iter = cache_.find(key);
        if ( iter != cache_.end() ) {
            iter->second.threads = profiled[0];
            iter->second.freq_idx = profiled[1];
        }

        vr[0] = profiled[0];
        vr[1] = profiled[1];

        cache_update((int)vr[0], (int)vr[1], objectives, true);
    }

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
                ve[j] = vm[j] + GAMMA * (vr[j] - vm[j]) - extra[j];
                
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
                return do_step_expand(entry->second.objectives,
                    entry->second.threads,
                    entry->second.freq_idx);
            }
        }

        return res;
    }
    else if ((f[vh] <= fr) && (fr < f[vg]))
    {
        // VV: REFLECTED between SECOND BEST and WORST
        auto gen_new = [this](double *extra) mutable -> double* {
            for (j = 0; j < NMD_NUM_KNOBS; j++)
                vc[j] = vm[j] + BETA * (vr[j] - vm[j]) - extra[j];
                
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
                return do_step_contract(entry->second.objectives,
                    entry->second.threads,
                    entry->second.freq_idx);
            }
        }

        return res;
    }
    else
    {
        // VV: REFLECTED worse than WORST
        auto gen_new = [this](double *extra) mutable -> double* {
            for (j = 0; j < NMD_NUM_KNOBS; j++)
                vc[j] = vm[j] - BETA * (vr[j] - vm[j]) - extra[j];
                
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
                return do_step_contract(entry->second.objectives,
                    entry->second.threads,
                    entry->second.freq_idx);
            }
        }

        return res;
    }
}

optstepresult NelderMead::do_step_expand(const double objectives[],
    double knob1, double knob2)
{
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead DEBUG] State = Expansion" << std::endl;
#endif
    fe = evaluate_score(objectives, nullptr);

    double profiled[] = {knob1, knob2};
    my_constraints(profiled);

    if ( ve[0] != profiled[0] || ve[1] != profiled[1] ) {
        std::cout << "[NelderMead|WARN] Meant to profile expand " << ve[0] << " knob1 "
                     "but ended up using " << profiled[0] << std::endl;
        std::cout << "[NelderMead|WARN] Meant to profile expand " << ve[1] << " knob2 "
                     "but ended up using " << profiled[1] << std::endl;
        
        auto key = std::make_pair((int)ve[0], (int)ve[1]);
        auto iter = cache_.find(key);
        if ( iter != cache_.end() ) {
            iter->second.threads = profiled[0];
            iter->second.freq_idx = profiled[1];
        }

        ve[0] = profiled[0];
        ve[1] = profiled[1];

        cache_update((int)ve[0], (int)ve[1], objectives, true);
    }

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

optstepresult NelderMead::do_step_contract(const double objectives[],
    double knob1, double knob2)
{
    int j;
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead|DEBUG] State = Contraction" << std::endl;
#endif
    fc = evaluate_score(objectives, nullptr);

    double profiled[] = {knob1, knob2};
    my_constraints(profiled);

    if ( vc[0] != profiled[0] || vc[1] != profiled[1] ) {
        std::cout << "[NelderMead|WARN] Meant to profile contract " << vc[0] << " knob1 "
                     "but ended up using " << profiled[0] << std::endl;
        std::cout << "[NelderMead|WARN] Meant to profile contract " << vc[1] << " knob2 "
                     "but ended up using " << profiled[1] << std::endl;
        
        auto key = std::make_pair((int)vc[0], (int)vc[1]);
        auto iter = cache_.find(key);
        if ( iter != cache_.end() ) {
            iter->second.threads = profiled[0];
            iter->second.freq_idx = profiled[1];
        }

        vc[0] = profiled[0];
        vc[1] = profiled[1];

        cache_update((int)vc[0], (int)vc[1], objectives, true);
    }

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
                new_vh[j] = v[vs][j] + DELTA * (v[vh][j] - v[vs][j]) - extra[j];
                
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
                return do_step_shrink(entry->second.objectives, 
                                        entry->second.threads, 
                                        entry->second.freq_idx);
            }
        }

        return res;
    }
}

optstepresult NelderMead::do_step_shrink(const double objectives[], 
            double knob1, double knob2)
{
#ifdef NMD_DEBUG_
    std::cout << "[NelderMead|DEBUG] State = Shrink" << std::endl;
#endif
    f[vh] = evaluate_score(objectives, nullptr);

    double profiled[] = {knob1, knob2};
    my_constraints(profiled);

    if ( v[vh][0] != profiled[0] || v[vh][1] != profiled[1] ) {
        std::cout << "[NelderMead|WARN] Meant to profile shrink " << v[vh][0] << " knob1 "
                     "but ended up using " << profiled[0] << std::endl;
        std::cout << "[NelderMead|WARN] Meant to profile shrink " << v[vh][1] << " knob2 "
                     "but ended up using " << profiled[1] << std::endl;
        
        auto key = std::make_pair((int)v[vh][0], (int)v[vh][1]);
        auto iter = cache_.find(key);
        if ( iter != cache_.end() ) {
            iter->second.threads = profiled[0];
            iter->second.freq_idx = profiled[1];
        }

        v[vh][0] = profiled[0];
        v[vh][1] = profiled[1];

        cache_update((int)v[vh][0], (int)v[vh][1], objectives, true);
    }

    const int threads = (int)(v[vh][0]);
    const int freq_idx = (int)(v[vh][1]);

    cache_update(threads, freq_idx, objectives, true);

    return do_step_start();
}

optstepresult NelderMead::step(const double objectives[], 
            double knob1, double knob2)
{
    int i, j;

    optstepresult res;
    res.threads = 0;
    res.freq_idx = -1;

    OUT_DEBUG(
        auto score = evaluate_score(objectives, nullptr);

        std::cout << "[NelderMead|DEBUG] Starting step with "
            << objectives[0] << " " 
            << objectives[1] << " " 
            << objectives[2] << " score " << score << std::endl;
    )
    
    if ( should_update_constraints ) {
        for (i=0; i<NMD_NUM_KNOBS; ++i )
        {
           constraint_min[i] = next_constraint_min[i];
           constraint_max[i] = next_constraint_max[i];
        }
        should_update_constraints = false;
    }

    std::size_t tested_combinations = cache_.size();
    
    #if 0
    evaluate_score(objectives, nullptr);

    for (i=0; i<NMD_NUM_KNOBS+1; ++i) {
        auto key = std::make_pair((int)v[i][0], (int)v[i][1]);
        auto entry = cache_.find(key);

        if ( entry != cache_.end() ) {
            f[i] = evaluate_score(entry->second.objectives, nullptr);
        }
    }
    #endif

    if ( should_invalidate_cache )
        do_invalidate_cache();
    
    if ( should_reevaluate_scores )
        do_reevaluate_scores();
    
    switch (state_)
    {
    case warmup:
    {
        #ifdef NMD_DEBUG_
            std::cout << "[NelderMead|DEBUG] State = Warmup " 
                      << warming_up_step << std::endl;
        #endif

        OUT_DEBUG(
            if ( warming_up_step == 0 ) {
                std::cout << "[NelderMead|DEBUG] Initial exploration" << std::endl;

                for ( auto i =0; i<NMD_NUM_KNOBS+1; ++i ) {
                    std::cout << "Simplex[" << i <<"]:";
                    for ( auto j=0; j<NMD_NUM_KNOBS; ++j )
                        std::cout << " " << initial_configurations[i][j];
                    std::cout << std::endl;
                }
            }
        )

        // VV: Make sure that we actually profiled what we meant to
        if ( warming_up_step > 0 && warming_up_step <= NMD_NUM_KNOBS + 1) {
            double profiled[] = {knob1, knob2};
            my_constraints(profiled);

            if ( v[warming_up_step-1][0] != profiled[0] || v[warming_up_step-1][1] != profiled[1] ) {
                std::cout << "[NelderMead|WARN] Meant to profile expand " << v[warming_up_step-1][0] << " knob1 "
                            "but ended up using " << profiled[0] << std::endl;
                std::cout << "[NelderMead|WARN] Meant to profile expand " << v[warming_up_step-1][1] << " knob2 "
                            "but ended up using " << profiled[1] << std::endl;
                
                auto key = std::make_pair((int)v[warming_up_step-1][0], (int)v[warming_up_step-1][1]);
                auto iter = cache_.find(key);
                if ( iter != cache_.end() ) {
                    iter->second.threads = profiled[0];
                    iter->second.freq_idx = profiled[1];
                }

                v[warming_up_step-1][0] = profiled[0];
                v[warming_up_step-1][1] = profiled[1];
            }
            
            // VV: Record results of last warming up step
            f[warming_up_step-1] = evaluate_score(objectives, nullptr);
            cache_update(v[warming_up_step-1][0], v[warming_up_step-1][1], 
                         objectives, true);
        } 

        if ( warming_up_step == NMD_NUM_KNOBS + 1) {
            // VV: We need not explore the knob_set space anymore
            state_ = start;
            return step(objectives, knob1, knob2);
        } else if (warming_up_step > NMD_NUM_KNOBS + 1) {
            std::cout << "[NelderMead|Warn] Unknown warmup step " << warming_up_step << std::endl;
        }

        optstepresult res;
        res.objectives[0] = -1;
        res.objectives[1] = -1;
        res.objectives[2] = -1;
        res.converged = false;

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
        res = do_step_reflect(objectives, knob1, knob2);
        break;
    case expansion:
        res = do_step_expand(objectives, knob1, knob2);
        break;
    case contraction:
        res = do_step_contract(objectives, knob1, knob2);
        break;
    case shrink:
        res = do_step_shrink(objectives, knob1, knob2);
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
    bool ret = false;

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
        ret = false;
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

        ret = true;
    }

    if ( ret == true && convergence_reevaluating == true ) {
        // VV: Now find the best result from cache
        sort_vertices();

        double best_knobs[NMD_NUM_KNOBS] = { v[vs][0], v[vs][1]};
        double best_score = f[vs];

        for ( const auto & entry: cache_ ) {
            auto cur_score = evaluate_score(entry.second.objectives, nullptr);
            if ( cur_score < best_score) {
                best_knobs[0] = entry.second.threads;
                best_knobs[1] = entry.second.freq_idx;

                best_score = cur_score;
            }
        }

        v[vs][0] = best_knobs[0];
        v[vs][1] = best_knobs[1];
        f[vs] = best_score;
        return true;
    } else if ( ret == true ) {
        // VV: Do another final run to make sure that the objective scores still hold up
        OUT_DEBUG (
            std::cout << "[NelderMead|Debug] Doing another final search" << std::endl;
        )
        state_ = warmup;
        warming_up_step = 0;
        itr --;
        convergence_reevaluating = true;
        cache_.clear();

        for (auto i=0; i<NMD_NUM_KNOBS+1; ++i ) {
            for (auto j=0; j<NMD_NUM_KNOBS; ++j) {
                initial_configurations[i][j] = v[i][j];
            }
        }

        OUT_DEBUG (
            print_initial_simplex();
        )

        return false;
    } else {
        return false;
    }
}

} // namespace components
} // namespace allscale
