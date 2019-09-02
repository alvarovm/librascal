/**
 * file   test_adaptor_center_contribution.cc
 *
 * @author Felix Musil <felix.musil@epfl.ch>
 *
 * @date   02 Sept 2019
 *
 * @brief  tests the implementation of the adaptor center contribution
 *
 * Copyright  2019 Felix Musil COSMO (EPFL), LAMMM (EPFL)
 *
 * Rascal is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3, or (at
 * your option) any later version.
 *
 * Rascal is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software; see the file LICENSE. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "tests.hh"
#include "test_structure.hh"
#include "test_adaptor.hh"

#include <vector>

namespace rascal {


  BOOST_AUTO_TEST_SUITE(adaptor_center_contribution_test);

  using multiple_fixtures = boost::mpl::list<
      MultipleStructureFixture<MultipleStructureManagerNLFixture>>;

  /* ---------------------------------------------------------------------- */
  /**
   * test strict neighbourhood constructor, this is done here instead
   * of in the Fixture, because of the default constructor is deleted.
   */
  BOOST_FIXTURE_TEST_CASE_TEMPLATE(constructor_test, Fix, multiple_fixtures, Fix) {
    auto && managers = Fix::managers;
    for (auto & manager: managers) {
      auto adaptor{
          make_adapted_manager<AdaptorCenterContribution>(manager)};

    }
  }

  /* ---------------------------------------------------------------------- */
  /**
   * Update test
   */
  BOOST_FIXTURE_TEST_CASE_TEMPLATE(update_test, Fix, multiple_fixtures, Fix) {
    auto && managers = Fix::managers;
    for (auto & manager: managers) {
      auto adaptor{
          make_adapted_manager<AdaptorCenterContribution>(manager)};
      adaptor->update();
    }
  }

  /* ---------------------------------------------------------------------- */
  /**
   * Iteration test for strict adaptor. It also checks if the positions and
   * types of the original atoms are accessed correctly.
   */
  BOOST_FIXTURE_TEST_CASE_TEMPLATE(iterator_test, Fix, multiple_fixtures, Fix) {
    auto && managers = Fix::managers;
    for (auto & manager: managers) {
      auto adaptor_strict{
          make_adapted_manager<AdaptorCenterContribution>(manager)};
      adaptor_strict->update();

      auto structure = extract_underlying_manager<0>(adaptor_strict);
      auto atom_types = structure->get_atom_types();

      int atom_counter{};
      int pair_counter{};
      constexpr bool verbose{false};

      for (auto atom : adaptor_strict) {
        auto index{atom.get_global_index()};
        BOOST_CHECK_EQUAL(index, atom_counter);

        auto type{atom.get_atom_type()};
        BOOST_CHECK_EQUAL(type, atom_types[index]);
        ++atom_counter;

        for (auto pair : atom) {
          auto pair_offset{pair.get_global_index()};
          auto pair_type{pair.get_atom_type()};
          if (verbose) {
            std::cout << "pair (" << atom.back() << ", " << pair.back()
                      << "), pair_counter = " << pair_counter
                      << ", pair_offset = " << pair_offset
                      << ", atom types = " << type << ", " << pair_type
                      << std::endl;
          }

          BOOST_CHECK_EQUAL(pair_counter, pair_offset);
          ++pair_counter;
        }
      }
    }
  }

  /* ---------------------------------------------------------------------- */
  /**
   * Test that the atom index from a neighbour matches the atom tag of the
   * ClusterRefKey returned by get_atom_j
   */
  BOOST_FIXTURE_TEST_CASE_TEMPLATE(get_atom_j_test, Fix, multiple_fixtures, Fix) {
    auto && managers = Fix::managers;
    constexpr bool verbose{false};
    for (auto & manager: managers) {
      auto adaptor_strict{
            make_adapted_manager<AdaptorCenterContribution>(manager)};
      adaptor_strict->update();

      for (auto atom : adaptor_strict) {
        for (auto pair : atom) {
          auto atom_j_index = adaptor_strict->get_atom_index(pair.back());
          auto atom_j = pair.get_atom_j();
          auto atom_j_tag = atom_j.get_atom_tag_list();
          if (verbose) {
            std::cout << "neigh: " << atom_j_index
                      << " tag_j: " << atom_j_tag[0] << std::endl;
          }

          BOOST_CHECK_EQUAL(atom_j_index, atom_j_tag[0]);
        }
      }
    }
  }


  /* ---------------------------------------------------------------------- */
  /**
   * Test that
   */
  BOOST_FIXTURE_TEST_CASE_TEMPLATE(get_atom_ii_test, Fix, multiple_fixtures,
                                   Fix) {
    auto && managers = Fix::managers;
    bool verbose{false};
    for (auto & manager: managers) {
      auto adaptor{
            make_adapted_manager<AdaptorCenterContribution>(manager)};
      adaptor->update();

      for (auto atom : adaptor) {
        auto ctag = atom.get_atom_tag();
        for (auto pair : atom) {
          auto atom_ii = pair.get_atom_ii();
          auto atom_ii_tag = atom_ii.get_atom_tag_list();

          if (verbose) {
            std::cout << "Center: " << ctag
                      << " neigh: " << atom_ii_tag[0] << ", " << atom_ii_tag[1]
                      << std::endl;
          }

          BOOST_CHECK_EQUAL(ctag, atom_ii_tag[0]);
          BOOST_CHECK_EQUAL(ctag, atom_ii_tag[1]);
        }
      }


    }
  }


  BOOST_AUTO_TEST_SUITE_END();

}  // namespace rascal
