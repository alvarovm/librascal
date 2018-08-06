/**
 * file   property_typed.hh
 *
 * @author Till Junge <till.junge@epfl.ch>
 *
 * @date   06 Aug 2018
 *
 * @brief  Implements intermediate property class for which the type of stored
 *          objects is known, but not the size
 *
 * Copyright © 2018 Federico Giberti, Till Junge, COSMO (EPFL), LAMMM (EPFL)
 *
 * librascal is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3, or (at
 * your option) any later version.
 *
 * librascal is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Emacs; see the file COPYING. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "structure_managers/property_base.hh"


namespace rascal {

  namespace internal {

    template <typename T, Dim_t NbRow, Dim_t NbCol>
    struct Value {
      using type = Eigen::Map<Eigen::Matrix<T, NbRow, NbCol>>;
      using reference = type;

      static reference get_ref(T & value, int nb_row, int nb_col) {
        return type(&value, nb_row, nb_col);
      }

      static reference get_ref(T & value) {
        return type(&value);
      }

      static void push_in_vector(std::vector<T> & vec, reference ref) {
        for (size_t j{0}; j < NbCol; ++j) {
          for (size_t i{0}; i < NbRow; ++i) {
            vec.push_back(ref(i,j));
          }
        }
      }

      // Used for extending cluster_indices
      template<typename Derived>
      static void push_in_vector(std::vector<T> & vec,
                                 const Eigen::DenseBase<Derived> & ref) {
        static_assert(Derived::RowsAtCompileTime==NbRow,
                      "NbRow has incorrect size.");
        static_assert(Derived::ColsAtCompileTime==NbCol,
                      "NbCol has incorrect size.");
        for (size_t j{0}; j < NbCol; ++j) {
          for (size_t i{0}; i < NbRow; ++i) {
            vec.push_back(ref(i,j));
          }
        }
      }
    };

    //! specialisation for scalar properties
    template <typename T>
    struct Value<T, 1, 1> {
      constexpr static Dim_t NbRow{1};
      constexpr static Dim_t NbCol{1};
      using type = T;
      using reference = T&;

      static reference get_ref(T & value) {return value;}

      static void push_in_vector(std::vector<T> & vec, reference ref) {
        vec.push_back(ref);
      }

      // Used for extending cluster_indices
      template<typename Derived>
      static void push_in_vector(std::vector<T> & vec,
                                 const Eigen::DenseBase<Derived> & ref) {
        static_assert(Derived::RowsAtCompileTime==NbRow,
                      "NbRow has incorrect size.");
        static_assert(Derived::ColsAtCompileTime==NbCol,
                      "NbCol has incorrect size.");
        vec.push_back(ref(0,0));
      }
    };

    template<typename T, size_t NbRow, size_t NbCol>
    using Value_t = typename Value<T, NbRow, NbCol>::type;

    template<typename T, size_t NbRow, size_t NbCol>
    using Value_ref = typename Value<T, NbRow, NbCol>::reference;

  }  // internal


  template <typename T>
  class TypedProperty: public PropertyBase
  {
    using Parent = PropertyBase;
    using Value = internal::Value<T, Eigen::Dynamic, Eigen::Dynamic>;

  public:

    using value_type = typename Value::type;
    using reference = typename Value::reference;

    //! constructor
    TypedProperty(StructureManagerBase & manager, Dim_t nb_row, Dim_t nb_col, size_t order):
      Parent{manager, nb_row, nb_col, order}
    {}

    //! Default constructor
    TypedProperty() = delete;

    //! Copy constructor
    TypedProperty(const TypedProperty &other) = delete;

    //! Move constructor
    TypedProperty(TypedProperty &&other) = default;

    //! Destructor
    virtual ~TypedProperty() = default;

    //! Copy assignment operator
    TypedProperty& operator=(const TypedProperty &other) = delete;

    //! Move assignment operator
    TypedProperty& operator=(TypedProperty &&other) = default;

    //! return runtime info about the stored (e.g., numerical) type
    const std::type_info & get_type_info() const override final {
      return typeid(T);
    };

    /**
     * Fill sequence for *_cluster_indices to initialize
     */
    inline void fill_sequence() {
      this->resize();
      for (size_t i{0}; i<this->values.size(); ++i) {
        values[i] = i;
      }
    }
    //! Adjust size of values (only increases, never frees)
    void resize() {
      this->values.resize(this->base_manager.nb_clusters(this->get_order()) * this->get_nb_comp());
    }

    /**
     * shortens the vector so that the manager can push_back into it
     * (capacity not reduced)
     */
    void resize_to_zero() {
      this->values.resize(0);
    }

    /**
     * Accessor for property by index for dynamically sized properties
     */
    reference operator[](const size_t & index) {
      return Value::get_ref(this->values[index*this->get_nb_comp()],
                            this->get_nb_row(),
                            this->get_nb_col());
    }


  protected:
    std::vector<T> values{}; //!< storage for properties
  private:
  };

}  // rascal
