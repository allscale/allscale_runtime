//  Copyright (c) 2016 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef ALLSCALE_EXAMPLES_UTS_UTS_HPP
#define ALLSCALE_EXAMPLES_UTS_UTS_HPP

#include <examples/uts/rng/rng.h>

#include <hpx/include/serialization.hpp>

#include <cmath>
#include <iostream>
#include <string>

#define MAX_NUM_CHILDREN    100  // cap on children (BIN root is exempt)

// Interpret 32 bit positive integer as value on [0,1)
inline double rng_toProb(int n)
{
    if (n < 0)
    {
        std::cout << "*** toProb: rand n = " << n << " out of range\n";
    }
    return ((n<0)? 0.0 : ((double) n)/2147483648.0);
}

struct node
{
    enum tree_type {
        BIN = 0,
        GEO,
        HYBRID,
        BALANCED
    };

    static const char * tree_type_str(tree_type type)
    {
        switch (type)
        {
            case BIN:
                return "Binomial";
            case GEO:
                return "Geometric";
            case HYBRID:
                return "Hybrid";
            case BALANCED:
                return "Balanced";
            default:
                return "Unkown";
        }
    }

    enum geoshape {
        LINEAR = 0,
        EXPDEC,
        CYCLIC,
        FIXED
    };

    static std::string geoshape_str(geoshape type)
    {
        switch (type)
        {
            case LINEAR:
                return "Linear decrease";
            case EXPDEC:
                return "Exponential decrease";
            case CYCLIC:
                return "Cyclic";
            case FIXED:
                return "Fixed branching factor";
            default:
                return "Unkown";
        }
    }

    int type;
    std::size_t height;
    int num_children;

    state_t state;

    template <typename Archive>
    void serialize(Archive & ar, unsigned)
    {
        ar & type;
        ar & height;
        ar & num_children;
        ar & state.state;
    }

    template <typename Params>
    void init_root(Params const & p)
    {
        type = p.type;
        height = 0;
        num_children = -1;
        rng_init(state.state, p.root_id);

        if(p.debug & 1)
        {
            std::cout << "root node of type " << p.type
                << " at " << std::hex << this << std::dec << "\n";
        }
    }

    template <typename Params>
    int child_type(Params const & p)
    {
        switch (p.type)
        {
            case BIN:
              return BIN;
            case GEO:
              return GEO;
            case HYBRID:
              if (height < p.shift_depth * p.gen_mx)
                return GEO;
              else
                return BIN;
            case BALANCED:
              return BALANCED;
            default:
              throw std::logic_error("node::child_type(): Unknown tree type");
              return -1;
        }
    }

    template <typename Params>
    int get_num_children_bin(Params const & p)
    {
        int v = rng_rand(state.state);
        double d = rng_toProb(v);

        return (d < p.non_leaf_prob) ? p.non_leaf_bf : 0;
    }

    template <typename Params>
    int get_num_children_geo(Params const & p)
    {
        double b_i = p.b_0;
        std::size_t depth = height;

        // use shape function to compute target b_i
        if(depth != 0)
        {
            switch(p.shape_fn)
            {
                // expected size polynomial in depth
                case EXPDEC:
                    b_i = p.b_0 * std::pow(
                        (double)depth, -std::log(p.b_0)/std::log((double)p.gen_mx));
                    break;
                // cyclic tree size
                case CYCLIC:
                    if(depth > 5 * p.gen_mx)
                    {
                        b_i = 0.0;
                        break;
                    }
                    b_i = std::pow(p.b_0,
                        std::sin(2.0 * 3.141592653589793*(double)depth / (double)p.gen_mx));
                    break;
                case FIXED:
                    b_i = (depth < p.gen_mx) ? p.b_0 : 0;
                    break;
                case LINEAR:
                default:
                    b_i = p.b_0 * (1.0 - (double)depth / (double)p.gen_mx);
                    break;
            }
        }

        // given target b_i, find prob p so expected value of
        // geometric distribution is b_i.
        double prob = 1.0 / (1.0 + b_i);

        // get uniform random number on [0,1)
        int h = rng_rand(state.state);
        double u = rng_toProb(h);

        // max number of children at this cumulative probability
        // (from inverse geometric cumulative density function)
        return (int) std::floor(std::log(1.0 - u) / std::log(1.0 - prob));
    }

    template <typename Params>
    int get_num_children(Params const & p)
    {
        int num = 0;
        switch (p.type)
        {
            case BIN:
                if(height == 0)
                {
                    num = static_cast<int>(std::floor(p.b_0));
                }
                else
                {
                    num = get_num_children_bin(p);
                }
                break;
            case GEO:
                num = get_num_children_geo(p);
                break;
            case HYBRID:
                if(height < p.shift_depth * p.gen_mx)
                {
                    num = get_num_children_geo(p);
                }
                else
                {
                    num = get_num_children_bin(p);
                }
                break;
            case BALANCED:
                if(height < p.gen_mx)
                {
                    num = (int)p.b_0;
                }
                break;
            default:
                throw std::logic_error("node::get_num_children(): Unknown tree type");
        }

        // limit number of children
        // only a BIN root can have more than MAX_NUM_CHILDREN
        if(height == 0 && type == BIN)
        {
            int root_BF = (int)std::ceil(p.b_0);
            if(num > root_BF)
            {
                std::cout << "*** Number of children of root truncated from "
                    << num << " to " << root_BF << "\n";
                num = root_BF;
            }
        }
        else if (p.type != BALANCED)
        {
            if (num > MAX_NUM_CHILDREN)
            {
                std::cout << "*** Number of children truncated from "
                    << num << " to " << MAX_NUM_CHILDREN << "\n";
                num = MAX_NUM_CHILDREN;
            }
        }
        return num;
    }
};

inline std::ostream & operator<<(std::ostream & os, node::tree_type type)
{
    os << node::tree_type_str(type);
    return os;
}

inline std::istream & operator>>(std::istream & is, node::tree_type & type)
{
    int i;
    is >> i;
    switch (i)
    {
        case node::BIN:
            type = node::BIN;
            break;
        case node::GEO:
            type = node::GEO;
            break;
        case node::HYBRID:
            type = node::HYBRID;
            break;
        case node::BALANCED:
            type = node::BALANCED;
            break;
        default:
            type = node::BIN;
            break;
    }
    return is;
}

inline std::ostream & operator<<(std::ostream & os, node::geoshape type)
{
    os << node::geoshape_str(type);
    return os;
}


inline std::istream & operator>>(std::istream & is, node::geoshape & type)
{
    int i;
    is >> i;
    switch (i)
    {
        case node::LINEAR:
            type = node::LINEAR;
            break;
        case node::EXPDEC:
            type = node::EXPDEC;
            break;
        case node::CYCLIC:
            type = node::CYCLIC;
            break;
        case node::FIXED:
            type = node::FIXED;
            break;
        default:
            type = node::LINEAR;
            break;
    }
    return is;
}

#endif
