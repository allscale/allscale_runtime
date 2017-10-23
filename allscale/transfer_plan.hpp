#ifndef ALLSCALE_TRANSFER_PLAN
#define ALLSCALE_TRANSFER_PLAN

#include <allscale/data_item_reference.hpp>

namespace allscale{

template<typename DataItemType>
class transfer_plan {

    using reference_type = typename allscale::data_item_reference<DataItemType>;
    using region_type = typename DataItemType::region_type;
    using locality_type = hpx::id_type;
public:

    // the different kinds of transfers may be ...
    enum class Kind {
        Replication, 	// < Replications, where the ownership of original data is preserved
        Migration		// < Migrations, where the ownership of original data is destroyed
    };

    /**
     * A description of a single data transfer step.
     */
    struct transfer {

        // the data item to be transfered
        reference_type ref;

        // the region to be transfered
        region_type region;

        // the source locality of the transfer
        locality_type src;

        // the destination locality of the transfer
        locality_type dst;

        // the kind of transfer
        Kind kind;

        friend std::ostream& operator<<(std::ostream& out, const transfer& transf) {
            if (transf.kind == Kind::Migration) {
                out << "migrate ";
            } else {
                out << "replicate ";
            }
            return out << transf.region << " from " << transf.src << " to " << transf.dst;
        }

    };

private:

    // the list of transfer operations to be covered by this transfer plan
    std::vector<transfer> transfers;

public:

    void addTransfer(const reference_type& ref, const region_type& region, locality_type src, locality_type dst, Kind kind) {
        transfers.push_back(transfer{ ref, region, src, dst, kind });
    }

    const std::vector<transfer>& getTransfers() const {
        return transfers;
    }

    friend std::ostream& operator<<(std::ostream& out, const transfer_plan& plan) {
//         return out << "transfer_plan {" << allscale::utils::join(",",plan.transfers) << "}";
        return out;
    }
};

}//end of namespace allscale
#endif
