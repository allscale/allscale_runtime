/*
 * Some utilities to navigate in an hierarchical overlay-network.
 *
 *  Created on: Jul 26, 2018
 *      Author: herbert
 */

#pragma once

#include <ostream>

#include <allscale/get_num_localities.hpp>
#include <allscale/get_num_numa_nodes.hpp>

#include "allscale/utils/assert.h"
#include "allscale/utils/serializer.h"

#include <hpx/runtime/get_locality_id.hpp>
#include <hpx/lcos/barrier.hpp>

namespace allscale {
namespace runtime {

	// define the type utilized to address layers
	using layer_t = std::uint32_t;

	// defines the address of a virtual node in the hierarchy
	class HierarchyAddress : public allscale::utils::trivially_serializable {

		// the addressed rank
        std::size_t rank;
        std::size_t numa_node;

		// the addressed layer
		layer_t layer;

	public:
        static std::size_t numaCutOff;

		// creates a new address targeting the given rank on the given node
		HierarchyAddress(std::size_t rank = 0, std::size_t numa_node = 0, layer_t layer = 0)
          : rank(rank), numa_node(numa_node), layer(layer) {
			// assert that the provided combination is valid
			assert_true(check());
		}

		// --- observer ---

		// obtains the rank of the node the addressed virtual node is hosted
		std::size_t getRank() const {
			return rank;
		}

        std::size_t getNumaNode() const {
            return numa_node;
        }

		// obtains the layer on the hosting node this address is referencing
		layer_t getLayer() const {
			return layer;
		}

		// tests whether this address is referencing an actual node
		bool isLeaf() const {
			return layer == 0;
		}

		// tests whether this address is referencing a virtual management node
		bool isVirtualNode() const {
			return !isLeaf();
		}

		// determine whether this node is the left child of its parent
		bool isLeftChild() const {
			return !isRightChild();
		}

		// determines whether this node is the right child of its parent
		bool isRightChild() const {
            return getParent().getRightChild() == *this;
// 			return rank & (1<<layer);
		}

		// --- factories ---

		/**
		 * Obtains the root node of a network of the given size.
		 */
		static HierarchyAddress getRootOfNetworkSize(std::size_t numa_size, std::size_t size);

		// --- navigation ---

		/**
		 * Obtains the address of the parent node.
		 */
		HierarchyAddress getParent() const {
			// computes the address of the parent (always possible)
            if (layer < numaCutOff)
            {
                return {rank, numa_node & ~(1 << (layer)), layer+1 };
            }

			return { rank & ~(1<<(layer - numaCutOff)), 0, layer+1 };
		}

		/**
		 * Obtains the address of an anchester node.
		 */
		HierarchyAddress getParent(int steps) const {
			if (steps == 0) return *this;
			return getParent().getParent(steps-1);
		}

		/**
		 * Obtains the address of the left child.
		 * ATTENTION: only supported for virtual nodes
		 */
		HierarchyAddress getLeftChild() const {
			// computes the address of the left child
			assert_true(isVirtualNode());
			return { rank, numa_node, layer-1 };
		}

		/**
		 * Obtains the address of the left child.
		 * ATTENTION: only supported for virtual nodes
		 */
		HierarchyAddress getRightChild() const {
			// computes the address of the right child
			assert_true(isVirtualNode());
            if (layer <= numaCutOff)
            {
                return {rank, numa_node + (1<<layer - 1), layer-1 };
            }

            assert_true(layer != 0);

			return { rank+(1<<(layer - 1 - numaCutOff)), 0, layer-1 };
		}

		// --- operators ---

		bool operator==(const HierarchyAddress& other) const {
			return rank == other.rank && layer == other.layer && numa_node == other.numa_node;
		}

		bool operator!=(const HierarchyAddress& other) const {
			return !(*this == other);
		}

		bool operator<(const HierarchyAddress& other) const {
            return std::tie(rank, numa_node, layer) < std::tie(other.rank, other.numa_node, other.layer);
		}

        bool operator>(const HierarchyAddress& other) const {
            return std::tie(rank, numa_node, layer) > std::tie(other.rank, other.numa_node, other.layer);
		}

        bool operator>=(const HierarchyAddress& other) const {
            return std::tie(rank, numa_node, layer) >= std::tie(other.rank, other.numa_node, other.layer);
		}

        bool operator<=(const HierarchyAddress& other) const {
            return std::tie(rank, numa_node, layer) <= std::tie(other.rank, other.numa_node, other.layer);
		}

		// add printer support
		friend std::ostream& operator<<(std::ostream&, const HierarchyAddress&);

		// --- utilities ---

		// computes the number of layers present on the given rank within a network of the given size
		static layer_t getLayersOn(std::size_t rank, std::size_t numa_node, std::size_t numa_size, std::size_t size);

	private:

		// tests whether the given rank/layer combination is valid
		bool check() const;

	};


	/**
	 * A wrapper for services transforming those into a hierarchical service.
	 * There will be one service instance per virtual node.
	 */
	template<typename Service>
	class HierarchyService {

		// the locally running services (one instance for each layer)
        std::vector<std::vector<Service>> services;

	public:

		// starts up a hierarchical service on this node
		template<typename ... Args>
		void init(const Args& ... args) {
			// start up services
            std::size_t num_numa_nodes = allscale::get_num_numa_nodes();
            std::size_t num_localities = allscale::get_num_localities();
            std::size_t num_services_global = num_numa_nodes * num_localities;

            std::size_t locality_id = hpx::get_locality_id();

            // Calculate the total number of local services
            services.resize(num_numa_nodes);
            for (std::size_t numa_node = 0; numa_node != num_numa_nodes; ++numa_node)
            {
                auto numServices = HierarchyAddress::getLayersOn(
                    locality_id, numa_node, num_numa_nodes, num_localities);

                services[numa_node].reserve(numServices);
                for(layer_t i=0; i< numServices; i++)
                {
                    services[numa_node].emplace_back(
                        HierarchyAddress(locality_id, numa_node, i), args...);
                }
            }

            hpx::lcos::barrier::synchronize();
		}

		// retrieves a service instance
		Service& get(std::size_t numa_node, layer_t layer) {
			assert_lt(numa_node,services.size());
			assert_lt(layer,services[numa_node].size());
			return services[numa_node][layer];
		}

		// applies an operation on all local services
		template<typename Op>
		void forAll(const Op& op) {
			for(auto& numa_services : services) {
                for(auto& cur : numa_services) {
                    op(cur);
                }
			}
		}

	};

	/**
	 * A hierarchical overlay network that can be placed on top of an existing network.
	 */
	class HierarchicalOverlayNetwork {

    private:
        template <typename S>
        static HierarchyService<S>& getService()
        {
            static HierarchyService<S> s;
            return s;
        }

	public:

		// creates the hierarchical view on this network
		HierarchicalOverlayNetwork()
        {}

		/**
		 * Obtains the address of the root node of this overlay network.
		 */
		HierarchyAddress getRootAddress() const {
			return HierarchyAddress::getRootOfNetworkSize(
                allscale::get_num_numa_nodes(), allscale::get_num_localities());
		}

		/**
		 * Installs a hierarchical service on all virtual nodes.
		 */
		template<typename S, typename ... Args>
		void installService(const Args& ... args) {
            getService<S>().init(args...);
		}

		/**
		 * Obtains a reference to a locally running service instance.
		 */
		template<typename S>
		static S& getLocalService(HierarchyAddress const& addr)
        {
            assert_eq(addr.getRank(), hpx::get_locality_id());
            auto& s = getService<S>();
			return s.get(addr.getNumaNode(), addr.getLayer());
		}

		/**
		 * Applies the given operation on all local service instances.
		 */
		template<typename S, typename Op>
		static void forAllLocal(const Op& op)
        {
			getService<S>().forAll(op);
		}

	};

} // end of namespace runtime
} // end of namespace allscale
