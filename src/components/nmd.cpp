#include <iostream>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <allscale/components/nmd.hpp>


#define NMD_DEBUG_
#define NMD_INFO_

#ifdef NMD_DEBUG_
#define OUT_DEBUG(X) X
#ifndef NMD_INFO_
    #define NMD_INFO_
#endif
#else
#define OUT_DEBUG(X) {}
#endif

#if defined(NMD_INFO_)
#define OUT_INFO(X) X
#else
#define OUT_INFO(X) {}
#endif


using namespace allscale::components;

NmdGeneric::NmdGeneric()
:
current_state(warmup), warmup_step(0), 
conv_threshold(0), num_knobs(0), num_objectives(0), 
scores(nullptr), simplex(nullptr), initial_config(nullptr),
constraint_max(nullptr), constraint_min(nullptr),
point_reflect(nullptr), point_contract(nullptr), weights(nullptr)
{}

NmdGeneric::NmdGeneric(std::size_t num_knobs, 
                        std::size_t num_objectives, 
                        double conv_threshold,
                        int64_t cache_expire_dt_ms,
                        std::size_t max_iters)
: conv_threshold(conv_threshold), num_knobs(num_knobs), 
num_objectives(num_objectives), 
cache_expire_dt_ms(cache_expire_dt_ms),
final_explore(false),
max_iters(max_iters)
{
    scores = new double [num_knobs+1];
    centroid = new std::size_t [num_knobs];
    simplex = new std::size_t* [num_knobs+1];
    initial_config = new std::size_t* [num_knobs+1];

    for (auto i=0ul; i<num_knobs+1; ++i) {
        simplex[i] = new std::size_t [num_knobs];
        initial_config[i] = new std::size_t [num_knobs];
    }

    constraint_max = new std::size_t [num_knobs];
    constraint_min = new std::size_t [num_knobs];

    point_reflect = new std::size_t [num_knobs];
    point_contract = new std::size_t [num_knobs];
    point_expand = new std::size_t [num_knobs];

    weights = new double [num_objectives];
}

double NmdGeneric::score(const double measurements[]) const
{
    return (*score_function)(measurements, weights);
}

void NmdGeneric::initialize(const std::size_t constraint_min[], 
                            const std::size_t constraint_max[],
                            const std::size_t *initial_config[], 
                            const double weights[], double (*score_function)(const double[], const double []))
{
    for (auto i=0ul; i<num_objectives; ++i)
        this->weights[i] = weights[i];
    
    this->score_function = score_function;

    set_constraints_now(constraint_min, constraint_max);

    iteration = 0;
    if ( initial_config == nullptr ) {
        std::set<std::vector<std::size_t> > fake;
        
        OUT_INFO(
            std::cout << "[NMD|Info] Generating initial config for " << num_knobs << std::endl;
        )

        for (auto i=0ul; i<num_knobs+1; ++i ) {
            for ( auto j=0ul; j<num_knobs; ++j ) {
                auto width = constraint_max[j] - constraint_min[j] + 1;
                this->initial_config[i][j] = std::rand() % width + constraint_min[j];
            }

            generate_unique(this->initial_config[i], false, &fake);
            auto new_key = std::vector<std::size_t>();
            new_key.assign(this->initial_config[i], this->initial_config[i]+num_knobs);
            fake.insert(new_key);
        }
    } else {
        for (auto i=0ul; i<num_knobs+1; ++i )
            for (auto j=0ul; j<num_knobs; ++j )
                this->initial_config[i][j] = initial_config[i][j];
    }

    current_state = warmup;
    warmup_step = 0;

    OUT_INFO(
        for (auto i=0ul; i<num_knobs+1; ++i ) {
            std::cout << "[NMD|Info] Initial config " << i << " : ";
            for (auto j=0ul; j<num_knobs; ++j )
                std::cout << this->initial_config[i][j] << " ";
            std::cout << std::endl;
        }
    )   

    final_explore = false;
    times_reentered_start = 0;
}

void NmdGeneric::set_constraints_now(const std::size_t constraint_min[],
                                    const std::size_t constraint_max[])
{
    for (auto i=0ul; i<num_knobs; ++i ){
        this->constraint_max[i] = constraint_max[i];
        this->constraint_min[i] = constraint_min[i];
    }
}

void NmdGeneric::generate_unique(std::size_t initial[], bool accept_stale=false, 
                                const std::set<std::vector<std::size_t> > *extra=nullptr) const
{
    const auto ts_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    auto explored = (std::size_t) std::count_if(cache.begin(), cache.end(), [ts_now, accept_stale](const auto &entry) {
        auto dt = ts_now - entry.second.cache_ts;
        return accept_stale || dt < entry.second.cache_dt;
    });

    auto max_comb = compute_max_combinations();

    if ( max_comb > explored && max_comb - explored > 1 ) {
        // VV: TODO Optimize check_novel(). Currently, large "max_distance" values 
        //     may result in extreme overheads
        const auto max_distance = 3ul;
        int64_t temp[num_knobs];
        std::set< std::vector<std::size_t> > candidates;

        auto check_novel = [this, &ts_now, &candidates, &accept_stale, &extra](int64_t knobs[]) mutable -> void {
            apply_constraint(knobs);

            auto key = std::vector<std::size_t>();

            key.assign(knobs, knobs+num_knobs);
            auto entry = cache.find(key);
            if ( extra == nullptr || extra->find(key) == extra->end()) {
                if ( entry == cache.end() ) {
                    candidates.insert(key);
                } else {
                    std::cout << "Found ";
                    for (auto i=0ul; i<num_knobs; ++i ) {
                        std::cout << key[i] << " ";
                    }
                    std::cout << std::endl;
                    auto dt = ts_now - entry->second.cache_ts;
                    if (accept_stale==false || 
                        (dt >= entry->second.cache_dt && cache_expire_dt_ms > 0) ) {
                        candidates.insert(key);
                    }
                }
            }
        };

        auto counters = std::vector<std::size_t>(num_knobs, 0ul);

        bool done = false;

        while ( done == false ) {
            // VV: Generate all possible permutations
            auto ops = std::string(num_knobs, '0');
            do{
                for ( auto j=0ul; j<num_knobs; ++j ) {
                    temp[j] = ops[j] == '0' ? initial[j] + counters[j] :
                                        initial[j] - counters[j];
                }
                check_novel(temp);
            } while (next_binary(ops.begin(), ops.end()));

            // VV: Increase inner-most loop and see if the whole process is terminated or not
            counters[0] += 1;

            for ( auto i=0ul; i<num_knobs-1; ++i ) {
                if ( (counters[i] > constraint_max[i] - constraint_min[i] +1) ||
                    (counters[i] > max_distance) ) {
                    counters[i] = 0;
                    counters[i+1] += 1;
                }
            }

            if ( (counters[num_knobs-1] > 
                    constraint_max[num_knobs-1] - constraint_min[num_knobs-1] +1)
                || (counters[num_knobs-1] > max_distance))
                done = true;
        }

        // std::cout << "Step " << candidates.size() << std::endl;

        std::vector< std::vector<std::size_t> > sorted;

        sorted.assign(candidates.begin(), candidates.end());
        candidates.clear();

        std::sort(sorted.begin(), sorted.end(), 
            [initial](const auto &e1, const auto &e2) mutable -> int {
                int64_t t;
                std::size_t d1=0ul, d2=0ul;

                for (auto i=0ul; i<e1.size(); ++i) {
                    t = (int64_t)e1[i] - (int64_t)initial[i];
                    d1 += t*t;

                    t = (int64_t)e2[i] - (int64_t)initial[i];
                    d2 += t*t;
                }

                return d1 < d2;
            });
        for (auto i=0ul; i<num_knobs; ++i)
            initial[i] = sorted[0][i];
    }
}

std::size_t NmdGeneric::compute_max_combinations() const
{
    if ( constraint_max == nullptr || constraint_min == nullptr ) {
        return 0ul;
    }
    std::size_t combinations = 1;

    for ( auto i=0ul; i<num_knobs; ++i )
        combinations += constraint_max[i] - constraint_min[i] +1;
    
    return combinations;
}

void NmdGeneric::ensure_profile_consistency(std::size_t expected[], 
    const std::size_t observed[]) const
{
    bool same = true;

    for (auto i=0ul; i<num_knobs; ++i)
        if (expected[i] != observed[i])
            same = false;
    if ( same == false ) {
        OUT_INFO(
            std::cout << "[NMD|Info] Profile does not match last suggestion, will correct: ";
        )
        
        for (auto i=0ul; i<num_knobs; ++i) 
            std::cout << expected[i] << " ";
        
        std::cout << " -- ";

        for (auto i=0ul; i<num_knobs; ++i) 
            std::cout << observed[i] << " ";
        std::cout << std::endl;

        for (auto i=0ul; i<num_knobs; ++i) 
            expected[i] = observed[i];
    }
}

void NmdGeneric::compute_centroid()
{
    double c[num_knobs];
    
    for (auto i=0ul; i<num_knobs; ++i )
    {
        c[i] = 0.0;

        for (auto j=0ul; j<num_knobs; ++j)
            c[i] += simplex[i][j];
        
        c[i] = round(c[i]/(double) num_knobs);
    }
    apply_constraint(c);

    for (auto i=0ul; i<num_knobs; ++i)
        centroid[i] = (std::size_t) c[i];
}

void NmdGeneric::sort_simplex(bool consult_cache)
{
    auto key = std::vector<std::size_t>();
    const auto ts_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    for ( auto i = 0ul; i<num_knobs+1; ++i) {
        key.assign(simplex[i], simplex[i]+num_knobs);
        auto entry = cache.find(key);
        logistics p = entry->second;
        p.cache_ts = ts_now;
        p.cache_dt = cache_expire_dt_ms;
        entry->second = p;
    }

    OUT_DEBUG(
        std::cout << "CACHE ENTRIES: "<<cache.size() << std::endl;
        for (const auto &e:cache) {
            for (auto i=0ul; i<num_knobs; ++i)
                std::cout << e.second.knobs[i] << " ";
            std::cout << ": ";
            for (auto i=0ul; i<num_objectives; ++i)
                std::cout << e.second.objectives[i] << " ";
            std::cout << " = " << score(e.second.objectives.data());
            std::cout << std::endl;
        }
    )
    
    std::vector<logistics> fresh;
    if ( consult_cache ) {
        for (const auto &e:cache )
            if (ts_now - e.second.cache_ts < e.second.cache_dt)
                fresh.push_back(e.second);
    
        std::sort(fresh.begin(), fresh.end(), 
            [this](const auto &e1, const auto &e2) mutable ->int {
                return this->score(e1.objectives.data()) < this->score(e2.objectives.data());
            });
    }

    if ( fresh.size() >= num_knobs+1) {
        for (auto i=0ul; i<num_knobs+1; ++i) {
            memcpy(simplex[i], fresh[i].knobs.data(), sizeof(std::size_t)*num_knobs);
            scores[i] = score(fresh[i].objectives.data());
        }
    } else {
        std::vector< std::pair<double, std::vector<std::size_t> > > plain;
        for ( auto i=0ul; i<num_knobs+1; ++i ) {
            key.assign(simplex[i], simplex[i] + num_knobs);
            plain.push_back( std::make_pair(scores[i], key));
        }

        std::sort(plain.begin(), plain.end(), 
        [](const auto &e1, const auto &e2) mutable ->int {
            return e1.first < e2.first;
        });

        for (auto i=0ul; i<num_knobs+1; ++i) {
            memcpy(simplex[i], plain[i].second.data(), sizeof(std::size_t)*num_knobs);
            scores[i] = plain[i].first;
        }
    }
}

std::vector<std::size_t> NmdGeneric::do_start(bool consult_cache=true)
{
    OUT_DEBUG(
        std::cout << "[NMD|Dbg]  INNER start" << std::endl;
    )
    iteration ++;
    sort_simplex(false);
    compute_centroid();
    double temp[num_knobs];

    OUT_INFO(
        std::cout << "[NMD|Info] Initial simplex" << std::endl;
        for ( auto i=0ul; i<num_knobs+1; ++i) {
            std::cout << "[NMD|Info] Score " << scores[i];
            for ( auto j=0ul; j<num_knobs; ++j)
                std::cout << " " << simplex[i][j];
            std::cout << std::endl;
        }

        std::cout << "[NMD|Info] Centroid: ";
        for (auto i=0ul; i<num_knobs; ++i)
            std::cout << centroid[i] << " ";
        std::cout << std::endl;
    )
    for (auto i=0ul; i<num_knobs; ++i)
        temp[i] = centroid[i] + ALPHA * (centroid[i] - (double)simplex[num_knobs][i]);
    
    apply_constraint(temp);

    for ( auto i=0ul; i<num_knobs; ++i)
        point_reflect[i] = temp[i];
    
    generate_unique(point_reflect, false);

    auto key = std::vector<std::size_t>();
    key.assign(point_reflect, point_reflect + num_knobs);

    auto entry = cache.find(key);

    current_state = reflect;

    if ( entry != cache.end() 
        && times_reentered_start++ < 5
        && iteration < max_iters ) {
        auto ts_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

        if ( ts_now - entry->second.cache_ts < entry->second.cache_dt ) {
            return do_reflect(entry->second.objectives.data(), entry->second.knobs.data());
        }
    }
    
    return key;
}

std::vector<std::size_t> NmdGeneric::do_shrink()
{
    OUT_DEBUG(
        std::cout << "[NMD|Dbg]  INNER shrink" << std::endl;
    )

    std::set<std::vector<std::size_t> > fake;
    std::vector<std::size_t> key;
    
    for ( auto i=0ul; i<num_knobs+1; ++i ) {
        double temp[num_knobs];

        for ( auto j=0ul; j<num_knobs; ++j ) {
            temp[j] = centroid[j] + DELTA * ((double)simplex[i][j] - (double)centroid[j]);
        }

        apply_constraint(temp);

        for ( auto j=0ul; j<num_knobs; ++j)
            initial_config[i][j] = temp[j];
        
        generate_unique(initial_config[i], false, &fake);

        key.assign(initial_config[i], initial_config[i]+num_knobs);
        fake.insert(key);
    }
    
    current_state = warmup;
    warmup_step = 0;

    OUT_INFO(
        for (auto i=0ul; i<num_knobs+1; ++i ) {
            std::cout << "[NMD|Info] Shrank simplex " << i << " : ";
            for (auto j=0ul; j<num_knobs; ++j )
                std::cout << this->initial_config[i][j] << " ";                
            std::cout << std::endl;
        }
    )

    return do_warmup({}, {});
}

std::vector<std::size_t> NmdGeneric::do_contract_out(const double measurements[], 
                            const std::size_t observed_knobs[])
{
    ensure_profile_consistency(point_contract, observed_knobs);
    score_contract = score(measurements);
    OUT_DEBUG(
        std::cout << "[NMD|Dbg]  INNER ContractOUT: ";
        for (auto i=0ul; i<num_knobs; ++i)
            std::cout << point_contract[i] << " ";
        std::cout << ":" << score_contract << std::endl;
    )

    auto ts_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    logistics entry;
    entry.knobs.assign(observed_knobs, observed_knobs+num_knobs);
    entry.objectives.assign(measurements, measurements+num_objectives);
    entry.cache_dt = cache_expire_dt_ms;
    entry.cache_ts = ts_now;

    cache[entry.knobs] = entry;

    if ( score_contract <= score_reflect ){
        // VV: foc <= fr then replace v[n] with voc

        for (auto i=0ul; i<num_knobs; ++i)
            simplex[num_knobs][i] = point_contract[i];
        
        scores[num_knobs] = score_contract;
        current_state = start;
        return do_start(true);
    } else {
        current_state = shrink;
        return do_shrink();
    }
}

std::vector<std::size_t> NmdGeneric::do_contract_in(const double measurements[], 
                            const std::size_t observed_knobs[])
{
    ensure_profile_consistency(point_contract, observed_knobs);
    score_contract = score(measurements);
    
    OUT_DEBUG(
        std::cout << "[NMD|Dbg]  INNER ContractIN: ";
        for (auto i=0ul; i<num_knobs; ++i)
            std::cout << point_contract[i] << " ";
        std::cout << ":" << score_contract << std::endl;
    )

    auto ts_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    logistics entry;
    entry.knobs.assign(observed_knobs, observed_knobs+num_knobs);
    entry.objectives.assign(measurements, measurements+num_objectives);
    entry.cache_dt = cache_expire_dt_ms;
    entry.cache_ts = ts_now;

    cache[entry.knobs] = entry;

    if ( score_contract < scores[num_knobs] ){
        // VV: fic < f[n] then replace v[n] with vic

        for (auto i=0ul; i<num_knobs; ++i)
            simplex[num_knobs][i] = point_contract[i];
        scores[num_knobs] = score_contract;
        current_state = start;
        return do_start(true);
    } else {
        current_state = shrink;
        return do_shrink();
    }
}


std::vector<std::size_t> NmdGeneric::do_expand(const double measurements[], 
                            const std::size_t observed_knobs[])
{
    ensure_profile_consistency(point_expand, observed_knobs);
    score_expand = score(measurements);

    OUT_DEBUG(
        std::cout << "[NMD|Dbg]  INNER Expand: ";
        for (auto i=0ul; i<num_knobs; ++i)
            std::cout << point_expand[i] << " ";
        std::cout << ":" << score_expand << std::endl;
    )

    auto ts_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    logistics entry;
    entry.knobs.assign(observed_knobs, observed_knobs+num_knobs);
    entry.objectives.assign(measurements, measurements+num_objectives);
    entry.cache_dt = cache_expire_dt_ms;
    entry.cache_ts = ts_now;

    cache[entry.knobs] = entry;

    if ( score_expand < score_reflect ){
        // VV: fe < fr then replace v[n] with ve
        for (auto i=0ul; i<num_knobs; ++i)
            simplex[num_knobs][i] = point_expand[i];
        scores[num_knobs] = score_expand;
    } else {
        // VV: fr <= fe then replace v[n] with vr
        for (auto i=0ul; i<num_knobs; ++i)
            simplex[num_knobs][i] = point_reflect[i];
        scores[num_knobs] = score_reflect;
    }

    current_state = start;
    return do_start(false);
}

std::vector<std::size_t> NmdGeneric::do_reflect(const double measurements[], 
                            const std::size_t observed_knobs[])
{
    ensure_profile_consistency(point_reflect, observed_knobs);
    score_reflect = score(measurements);

    OUT_DEBUG(
        std::cout << "[NMD|Dbg]  INNER Reflect: ";
        for (auto i=0ul; i<num_knobs; ++i)
            std::cout << point_reflect[i] << " ";
        std::cout << ":" << score_reflect << std::endl;
    )

    auto ts_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    logistics entry;
    entry.knobs.assign(observed_knobs, observed_knobs+num_knobs);
    entry.objectives.assign(measurements, measurements+num_objectives);
    entry.cache_dt = cache_expire_dt_ms;
    entry.cache_ts = ts_now;

    cache[entry.knobs] = entry;

    if ( score_reflect >= scores[0] && score_reflect < scores[num_knobs-1]) {
        // VV: fo <= fr < f[n-1] then replace v[n] with vr and start over
        for ( auto i=0ul; i<num_knobs; ++i)
            simplex[num_knobs][i] = point_reflect[i];
        scores[num_knobs] = score_reflect;
        current_state = start;
        return do_start(true);
    } else if (score_reflect < scores[0]) {
        double temp[num_knobs];
        current_state = expand;
        for (auto i=0ul; i<num_knobs; ++i)
            temp[i] = centroid[i] + BETA * (point_reflect[i] - (double)centroid[i]);
        
        apply_constraint(temp);

        for ( auto i=0ul; i<num_knobs; ++i)
            point_expand[i] = temp[i];
        generate_unique(point_expand, false);

        auto key = std::vector<std::size_t>();
        key.assign(point_expand, point_expand+num_knobs);
        auto e = cache.find(key);

        if ( e != cache.end() ) {
            if ( ts_now - e->second.cache_ts < e->second.cache_dt ) {
                return do_expand(e->second.objectives.data(),
                                e->second.knobs.data());
            }
        }

        return key;
    } else if (scores[num_knobs-1] <= score_reflect 
            &&  score_reflect < scores[num_knobs]) {
        // VV: Reflect lies between f[n-1] and f[n] then contract (outside)
        current_state = contract_out;
        double temp[num_knobs];

        for (auto i=0ul; i<num_knobs; ++i)
            temp[i] = centroid[i] + GAMMA * (point_reflect[i] - (double)centroid[i]);
        
        apply_constraint(temp);

        for ( auto i=0ul; i<num_knobs; ++i)
            point_contract[i] = temp[i];
        generate_unique(point_contract, false);

        auto key = std::vector<std::size_t>();
        key.assign(point_contract, point_contract+num_knobs);
        auto e = cache.find(key);

        if ( e != cache.end() ) {
            if ( ts_now - e->second.cache_ts < e->second.cache_dt ) {
                return do_contract_out(e->second.objectives.data(),
                                e->second.knobs.data());
            }
        }

        return key;
    } else if (score_reflect >= scores[num_knobs]) {
        // VV: Reflect > f[n] then contract (inside)
        current_state = contract_in;
        double temp[num_knobs];

        for (auto i=0ul; i<num_knobs; ++i)
            temp[i] = (double) centroid[i] - GAMMA * ((double)point_reflect[i] - (double)centroid[i]);
        
        apply_constraint(temp);

        for ( auto i=0ul; i<num_knobs; ++i)
            point_contract[i] = temp[i];
        generate_unique(point_contract, false);

        auto key = std::vector<std::size_t>();
        key.assign(point_contract, point_contract+num_knobs);
        auto e = cache.find(key);

        if ( e != cache.end() ) {
            if ( ts_now - e->second.cache_ts < e->second.cache_dt ) {
                return do_contract_in(e->second.objectives.data(),
                                e->second.knobs.data());
            }
        }

        return key;
    }

    OUT_INFO(
        std::cout << "[NMD|Info] Should never get here" << std::endl;
    )

    current_state = start;
    return do_start(true);
}

std::vector<std::size_t> NmdGeneric::do_warmup(const double measurements[], 
                            const std::size_t observed_knobs[])
{
    std::vector<std::size_t> ret;
    OUT_DEBUG(
        std::cout << "[NMD|Dbg]  INNER warmup" << std::endl;
    )

    if ( warmup_step > 0 ) {
        auto last = warmup_step - 1;
        ensure_profile_consistency(initial_config[last], observed_knobs);
        memcpy(simplex[last], initial_config[last], sizeof(std::size_t)*num_knobs);
        scores[last] = score(measurements);
        auto key = std::vector<size_t>();
        key.assign(observed_knobs, observed_knobs+num_knobs);
        
        logistics entry;

        entry.cache_dt = cache_expire_dt_ms;
        entry.cache_ts = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        entry.knobs.assign(observed_knobs, observed_knobs+num_knobs);
        entry.objectives.assign(measurements, measurements+num_objectives);

        cache[key] = entry;
        
        OUT_DEBUG(
            auto s = score(measurements);
            std::cout << "[NMD|Dbg]  Score: " << s << " for ";
            for( auto i=0ul; i<num_knobs; ++i)
                std::cout << observed_knobs[i] << " ";
            std::cout << std::endl;
        )
    }

    if ( warmup_step == num_knobs +1 ) {
        OUT_DEBUG(
            std::cout << "[NMD|Dbg]  Warmup results" << std::endl;

            for (const auto &e:cache) {
                for (auto i=0ul; i<num_knobs; ++i)
                    std::cout << e.second.knobs[i] << " ";
                std::cout << ": ";
                for (auto i=0ul; i<num_objectives; ++i)
                    std::cout << e.second.objectives[i] << " ";
                std::cout << " = " << score(e.second.objectives.data());
                std::cout << std::endl;
            }
        )
        
        current_state = start;
        return do_start(false);
    }
    
    ret.assign(this->initial_config[warmup_step],
                this->initial_config[warmup_step]+num_knobs);
    
    warmup_step ++;

    OUT_INFO(
        std::cout << "[NMD|Info] Warmup Explore: ";
        for (auto i=0ul; i<num_knobs; ++i)
            std::cout << ret[i] << " ";
        
        std::cout << std::endl;
    )

    return ret;
}

std::pair<std::vector<std::size_t>, bool> NmdGeneric::get_next(const double measurements[], 
                            const std::size_t observed_knobs[])
{
    std::vector<std::size_t> ret;
    #if defined(NMD_DEBUG_) || defined(NMD_INFO_)
        const char *state_names[] = {
            "warmup",
            "start",
            "reflect",
            "expand",
            "contract_in",
            "contract_out",
            "shrink"
        };
    #endif

    OUT_DEBUG(
        std::cout << "[NMD|Dbg]  Current stage " << state_names[current_state] << std::endl;
    )
    
    switch (current_state) {
        case warmup:
            ret = do_warmup(measurements, observed_knobs);
            break;
        case start:
            times_reentered_start = 0;
            ret = do_start(true);
            break;
        case reflect:
            ret = do_reflect(measurements, observed_knobs);
            break;
        case expand:
            ret = do_expand(measurements, observed_knobs);
            break;
        case contract_in:
            ret = do_contract_in(measurements, observed_knobs);
            break;
        case contract_out:
            ret = do_contract_out(measurements, observed_knobs);
            break;
        case shrink:
            ret = do_shrink();
            break;
        default:
            std::cout << "Unknown state!" << std::endl;
    }

    OUT_INFO(
        std::cout << "[NMD|Info] State " << state_names[current_state] << " proposes ";

        for (auto i=0ul; i<num_knobs; ++i)
            std::cout << ret[i] << " ";
        std::cout << std::endl;
    )

    bool converged = false;
    if ( current_state != warmup )
    {
        converged = test_convergence();
    }

    if ( converged ) {
        sort_simplex(true);
        ret.assign(simplex[0], simplex[0] + num_knobs);
    }

    return std::make_pair(ret, converged);
}


bool NmdGeneric::test_convergence()
{
    double avg, sum;

    avg = 0.0;
    sum = 0.0;

    for ( auto i=0ul; i<num_knobs+1; ++i)
        avg += scores[i];
    
    avg /= (num_knobs+1);

    for ( auto i=0ul; i<num_knobs+1; ++i) {
        double t = scores[i] - avg;
        sum += t * t;        
    }

    sum /= num_knobs;
    sum = sqrt(sum);

    if (iteration >= max_iters || sum <= conv_threshold ) {
        // if ( final_explore == false ) {
        //     final_explore = true;

        //     return false;
        // } else {
        //     return true;
        // }
        OUT_INFO(
            std::cout << "[NMD|Info] Converged at " << sum 
                      << " threshold: " << conv_threshold << std::endl;

            std::cout << "[NMD|Info] Converged simplex" << std::endl;
            for ( auto i=0ul; i<num_knobs+1; ++i) {
                std::cout << "[NMD|Info] Score " << scores[i];
                for ( auto j=0ul; j<num_knobs; ++j)
                    std::cout << " " << simplex[i][j];
                std::cout << std::endl;
            }
        )
        return true;
    }

    return false;
}
