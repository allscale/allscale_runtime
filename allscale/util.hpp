#ifndef ALLSCALE_UTIL
#define ALLSCALE_UTIL

#include <allscale/location_info.hpp>

#include <allscale/data_item_requirement.hpp>
#include <allscale/access_mode.hpp>



namespace allscale{

        template<typename Region>
		Region merge(const Region& a, const Region& b) {
			return Region::merge(a,b);
		}

		template<typename Region>
		Region intersect(const Region& a, const Region& b) {
			return Region::intersect(a,b);
		}

		template<typename Region>
		Region difference(const Region& a, const Region& b) {
			return Region::difference(a,b);
		}


//         /**
//          * A class summarizing the a set of data transfer operations.
//          */
//         template<typename DataItemType>
//         class TransferPlan {
//
//             using reference_type = data_item_reference<DataItemType>;
//             using region_type = typename DataItemType::region_type;
//
//         public:
//
//             // the different kinds of transfers may be ...
//             enum class Kind {
//                 Replication, 	// < Replications, where the ownership of original data is preserved
//                 Migration		// < Migrations, where the ownership of original data is destroyed
//             };
//
//
//             /**
//              * A description of a single data transfer step.
//              */
//             struct Transfer {
//
//                 // the data item to be transfered
//                 reference_type ref;
//
//                 // the region to be transfered
//                 region_type region;
//
//                 // the source locality of the transfer
//                 hpx::id_type src;
//
//                 // the destination locality of the transfer
//                 hpx::id_type dst;
//
//                 // the kind of transfer
//                 Kind kind;
//
//                 friend std::ostream& operator<<(std::ostream& out, const Transfer& transfer) {
//                     if (transfer.kind == Kind::Migration) {
//                         out << "migrate ";
//                     } else {
//                         out << "replicate ";
//                     }
//                     return out << transfer.region << " from " << transfer.src << " to " << transfer.dst;
//                 }
//
//             };
//
//         private:
//
//             // the list of transfer operations to be covered by this transfer plan
//             std::vector<Transfer> transfers;
//
//         public:
//
//             void addTransfer(const reference_type& ref, const region_type& region, hpx::id_type src, hpx::id_type dst, Kind kind) {
//                 transfers.push_back(Transfer{ ref, region, src, dst, kind });
//             }
//
//             const std::vector<Transfer>& getTransfers() const {
//                 return transfers;
//             }
//
//             friend std::ostream& operator<<(std::ostream& out, const TransferPlan& plan) {
//                 return out << "TransferPlan {" << allscale::utils::join(",",plan.transfers) << "}";
//             }
//         };
//
//
//
// 		/**
// 		 * A simple transfer plan generator, following a greedy heuristic.
// 		 */
// 		struct SimpleTransferPlanGenerator {
//
// 			template<typename DataItemType>
// 			TransferPlan<DataItemType> operator()(const location_info<DataItemType>& info, hpx::id_type targetLocation, const data_item_requirement<DataItemType>& requirement) {
//
// 				TransferPlan<DataItemType> res;
//
// 				auto rest = requirement.region;
//
// 				// first, remove everything that is stored at the target node
// 				for(const auto& cur : info.getParts()) {
// 					if (cur.location == targetLocation) {
// 						// we do not need to transfer this data
// 						rest = difference(rest,cur.region);
// 					}
// 				}
//
// 				// if this is all, we are done
// 				if (rest.empty()) return res;
//
// 				// determine the kind of data transfer
// 				auto kind = (requirement.mode == access_mode::ReadOnly)
// 						? TransferPlan<DataItemType>::Kind::Replication
// 						: TransferPlan<DataItemType>::Kind::Migration;
//
// 				// search for the remaining data
// 				for(const auto& cur : info.getParts()) {
// 					// check whether this is a possible contributor
// 					auto overlap = intersect(rest,cur.region);
// 					if (overlap.empty()) continue;
//
// 					// add this piece
// 					res.addTransfer(requirement.ref, overlap, cur.location, targetLocation,kind);
//
// 					// consider this part done
// 					rest = difference(rest,overlap);
// 				}
//
// 				// that's it, not more to do
// 				return res;
//
// 			}
//
// 		};
//
// 	/**
// 	 * A utility computing a transfer plan based on a given data distribution information,
// 	 * a target location, a region to be targeted, and a desired access mode.
// 	 *
// 	 * @param info a summary of the data distribution state covering the provided region
// 	 * @param targetLocation the location where the given region should be moved to
// 	 * @param region the region to be moved
// 	 * @param mode the intended access mode of the targeted region after the transfer
// 	 */
// 	template<typename TransferPlanGenerator = SimpleTransferPlanGenerator, typename DataItemType>
// 	TransferPlan<DataItemType> buildPlan(const location_info<DataItemType>& info, hpx::id_type targetLocation, const data_item_requirement<DataItemType>& requirement) {
// 		// use the transfer plan generator policy as requested
// 		return TransferPlanGenerator()(info,targetLocation,requirement);
// 	}
//
//
//          /**
// 		 * A base class for cost models.
// 		 */
// 		template<typename V>
// 		struct cost_model {
// 			using value_type = V;
// 		};
//
// 		/**
// 		 * A simple cost model for data transfers simply counting the number of included
// 		 * data transfers.
// 		 */
// 		struct NumberOfTransfersCostModel : public cost_model<std::size_t> {
//
// 			template<typename DataItemType>
// 			std::size_t evaluate(const TransferPlan<DataItemType>& plan) {
// 				return plan.getTransfers().size();
// 			}
//
// 		};
// 	/**
// 	 * A generalized interface for evaluating the costs of a data transfer plan.
// 	 */
// 	template<typename CostModel = NumberOfTransfersCostModel, typename DataItemType>
// 	typename CostModel::value_type estimateTransfereCosts(const TransferPlan<DataItemType>& plan) {
// 		return CostModel().evaluate(plan);
// 	}



}


#endif
