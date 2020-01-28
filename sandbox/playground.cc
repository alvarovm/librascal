/**
 * @file   sandbox/playground.cc
 *
 * @author Felix Musil <felix.musil@epfl.ch>
 *
 * @date   26 June 2019
 *
 * @brief an executable to test ideas
 *
 * Copyright © 2019 Felix Musil, COSMO (EPFL), LAMMM (EPFL)
 *
 * librascal is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3, or (at
 * your option) any later version.
 *
 * librascal is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software; see the file LICENSE. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "rascal/utils/basic_types.hh"
#include "rascal/models/kernels.hh"
#include "rascal/utils/utils.hh"
#include "rascal/representations/calculator_sorted_coulomb.hh"
#include "rascal/representations/calculator_spherical_expansion.hh"
#include "rascal/representations/calculator_spherical_invariants.hh"
#include "rascal/structure_managers/adaptor_increase_maxorder.hh"
#include "rascal/structure_managers/cluster_ref_key.hh"
#include "rascal/structure_managers/adaptor_center_contribution.hh"
#include "rascal/structure_managers/adaptor_link_pairs.hh"
#include "rascal/structure_managers/adaptor_half_neighbour_list.hh"
#include "rascal/structure_managers/adaptor_neighbour_list.hh"
#include "rascal/structure_managers/adaptor_strict.hh"
#include "rascal/structure_managers/make_structure_manager.hh"
#include "rascal/structure_managers/structure_manager_centers.hh"
#include "rascal/structure_managers/structure_manager_collection.hh"

#include <cmath>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <list>
#include <random>
#include <string>
#include <algorithm>
#include <iterator>

using namespace rascal;  // NOLINT

using ManagerTypeHolder_t = StructureManagerTypeHolder<StructureManagerCenters,
                                    AdaptorNeighbourList,
                                    AdaptorCenterContribution, AdaptorStrict,
                                    LinkPairs>;
using ManagerTypeList_t = typename ManagerTypeHolder_t::type_list;
using Manager_t = typename ManagerTypeHolder_t::type;
using ManagerCollection_t =
    typename TypeHolderInjector<ManagerCollection, ManagerTypeList_t>::type;
using Representation_t = CalculatorSphericalInvariants;

constexpr static size_t ClusterLayer_{
          Manager_t::template cluster_layer_from_order<2>()};

int main() {

  std::string filename{"../reference_data/inputs/diamond_2atom_distorted.json"};

  double cutoff{2.5};

  // json hypers{{"max_radial", 6},
  //             {"max_angular", 6},
  //             {"compute_gradients", false},
  //             {"soap_type", "PowerSpectrum"},
  //             {"normalize", true},
  //             {"expansion_by_species_method", "environment wise"}};

  // json fc_hypers{{"type", "ShiftedCosine"},
  //                {"cutoff", {{"value", cutoff}, {"unit", "AA"}}},
  //                {"smooth_width", {{"value", 0.5}, {"unit", "AA"}}}};
  // json sigma_hypers{{"type", "Constant"},
  //                   {"gaussian_sigma", {{"value", 0.4}, {"unit", "AA"}}}};

  // hypers["cutoff_function"] = fc_hypers;
  // hypers["gaussian_density"] = sigma_hypers;
  // hypers["radial_contribution"] = {{"type", "GTO"}};

  // json kernel_hypers{
  //       {"zeta", 1}, {"target_type", "Atom"}, {"name", "Cosine"}};

  json structure{{"filename", filename}};
  json adaptors;
  json adaptors_half;
  json ad1a{{"name", "AdaptorNeighbourList"},
           {"initialization_arguments", {{"cutoff", cutoff}}}};
  json ad1b{{"name", "AdaptorHalfList"},
            {"initialization_arguments", {}}};
  json ad1c{{"name", "AdaptorCenterContribution"},
            {"initialization_arguments", {}}};
  json ad2a{{"name", "AdaptorStrict"},
           {"initialization_arguments", {{"cutoff", cutoff}}}};
  json ad2b{{"name", "AdaptorLinkPairs"},
           {"initialization_arguments", {}}};
  adaptors.emplace_back(ad1a);
  adaptors.emplace_back(ad1c);
  adaptors.emplace_back(ad2a);
  adaptors.emplace_back(ad2b);

  ManagerCollection_t collection{adaptors};
  collection.add_structures(filename, 0, 1);

  for (const auto & manager : collection) {
    for (auto center : manager) {
      std::cout << "Center: " << center.get_atom_tag() << std::endl;
      for (auto pair : center.pairs()) {
        std::cout << "Neigh ij: " << pair.get_cluster_index()
                  << " (" << pair.front() << ", " << pair.back() << ") ";
        auto && pair_ji = pair.get_pair_ji();
        std::cout << "Neigh ji: " << pair_ji.get_cluster_index()
                  << " (" << pair_ji.front() << ", " << pair_ji.back() << ") "<< std::endl;
      }
    }
  }

  // Representation_t soap{hypers};

  // soap.compute(collection);

  // Kernel kernel{kernel_hypers};

  // auto kk = kernel.compute(soap, collection, collection);

  // std::cout << kk << std::endl;
}