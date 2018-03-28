#ifndef ALLSCALE_WORK_ITEM_STATS_HPP
#define ALLSCALE_WORK_ITEM_STATS_HPP

#include <hpx/include/serialization.hpp>
#include <hpx/runtime/serialization/string.hpp>

namespace allscale
{
    struct work_item_stats 
    {
        work_item_stats()
        {
	  exclusive_time_ = inclusive_time_ = children_mean_ = children_SD_ = 0.0;
        }

        work_item_stats(std::string id, std::string name, double exclusive, double inclusive, double mean, double sd)
        {
          id_ = id;
          name_ = name;
          exclusive_time_ = exclusive;
          inclusive_time_ = inclusive;
          children_mean_ = mean;
          children_SD_ = sd;
        }


        bool operator < (const work_item_stats& w) const
        {
            return (id_ < w.id_);
        }

        std::string get_id() { return id_; }
        std::string get_name() { return name_; }
        double get_exclusive_time() { return exclusive_time_; }
        double get_inclusive_time() { return inclusive_time_; }

        double get_children_mean() { return children_mean_; }
        double get_children_SD() { return children_SD_; }


        private:
           std::string id_, name_;
           double exclusive_time_, inclusive_time_;
           double children_mean_, children_SD_;


           friend class hpx::serialization::access;
           template <typename Archive>
           void serialize(Archive& ar, unsigned)
           {
               ar & id_ & name_ & exclusive_time_ & inclusive_time_ 
	          & children_mean_ & children_SD_;
#ifdef HAVE_PAPI

#endif
                 
        }

    };



}

#endif
