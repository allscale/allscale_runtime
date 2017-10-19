#ifndef ALLSCALE_SIMPLE_TRANSFER_PLAN_GENERATOR
#define ALLSCALE_SIMPLE_TRANSFER_PLAN_GENERATOR

#include <allscale/location_info.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/transfer_plan.hpp>
#include <allscale/access_mode.hpp>

namespace allscale{
 


struct simple_transfer_plan_generator {

    template<typename Region>
    static Region merge(const Region& a, const Region& b) {
        return Region::merge(a,b);
    }

    template<typename Region>
    static Region intersect(const Region& a, const Region& b) {
        return Region::intersect(a,b);
    }

    template<typename Region>
    static Region difference(const Region& a, const Region& b) {
        return Region::difference(a,b);
    }

    using locality_type = hpx::id_type;
    
    template<typename DataItemType>
    transfer_plan<DataItemType> operator()(const location_info<DataItemType>& info, locality_type target_location, const data_item_requirement<DataItemType>& requirement) {

        transfer_plan<DataItemType> res;

        auto rest = requirement.region;

        // first, remove everything that is stored at the target node
        for(const auto& cur : info.getParts()) {
            if (cur.location == target_location) {

                std::cout<<"cur.location " << cur.location << " target_location " << target_location << std::endl;
                // we do not need to transfer this data
                rest = difference(rest,cur.region);
            }
        }

        // if this is all, we are done
        if (rest.empty()) return res;

        // determine the kind of data transfer
        auto kind = (requirement.mode == access_mode::ReadOnly)
                ? transfer_plan<DataItemType>::Kind::Replication
                : transfer_plan<DataItemType>::Kind::Migration;

        // search for the remaining data
        for(const auto& cur : info.getParts()) {
            // check whether this is a possible contributor
            auto overlap = intersect(rest,cur.region);
            if (overlap.empty()) continue;

            // add this piece
            res.addTransfer(requirement.ref, overlap, cur.location, target_location,kind);

            // consider this part done
            rest = difference(rest,overlap);
        }

        // that's it, not more to do
        return res;

    }

};
}
#endif
