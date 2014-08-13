/**
 *  Copyright 2014 by Benjamin Land (a.k.a. BenLand100)
 *
 *  analysis is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  analysis is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with analysis. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _NDARRAY_HH
#define _NDARRAY_HH

#include <stdexcept>

#include "functional.hh"

namespace analysis {
	
	//for ALL of a particular dimension
	static const std::tuple<int,int> ALL = std::make_tuple(0,-1);
	
	//for a RANGE of indexes in a particular dimension (begin,end[,step])
	template <typename... Ds> std::tuple<Ds...> RANGE(Ds... range) { return std::make_tuple(range...); } 
	
	//for a LIST of indexes in a particular dimension (idx1,idx2,idx3,...)
	template <typename... Ds> std::valarray<int> LIST(Ds... indexes) { return list<int>(indexes...); } 

	template<typename T> class ndslice;
	template<typename T> class ndarray;
	
	//represents a sliced view of an ndarray
	template<typename T>
	class ndslice {
		friend ndarray<T>;
		public:
			//scalar assignment to a slice
			void operator=(T val) {
				if (_shape.size() == 1 && _shape[0] == 1) {
					(*_arr)[_indexes] = {val};
				} else {
					throw std::invalid_argument("ndarray is not a scalar");
				}
			}
			
			//shaped or flat assignment to a slice
			void operator=(std::valarray<T> vals) {
				(*_arr)[_indexes] = vals;
			}
			
			//cast to scalar?
			
		protected:
			ndarray<T> *_arr;
			std::valarray<size_t> _indexes, _shape;
			
			ndslice(ndarray<T> *arr, std::valarray<size_t> indexes, std::valarray<size_t> shape) : 
				_arr(arr), _indexes(indexes), _shape(shape) { 
			}
	};
	
	//represents an n-dimensional rectangular datastructure that can be sliced and indexed
	template<typename T>
	class ndarray : public std::valarray<T> {
		public:
			//constructs an ndarray filled with the given value and shape
			ndarray(T init, std::valarray<size_t> shape) {
				if (shape.size() == 0)  	 std::length_error("ndarray cannot have zero dimensions");
				size_t size = 1;
				for (size_t i = 0; i < shape.size(); i++) {
					size *= shape[i];	
				}
				std::valarray<T>::resize(size,init);
				reshape(shape);
			}
		
			//constructs an ndarry from a flat list and a shape
			ndarray(std::valarray<T> flat, std::valarray<size_t> shape = {}) : std::valarray<T>(flat) {
				if (shape.size() == 0) shape = {flat.size()};
				reshape(shape);
			}
			
			//constructs an ndarray copy of an ndslice
			ndarray(ndslice<T> slice) : std::valarray<T>((*slice._arr)[slice._indexes]) {
				reshape(slice._shape);
			}
			
			//changes the shape of the ndarray (must have same total size)
			void reshape(std::valarray<size_t> shape) {
				size_t size = shape[0];
				for (size_t i = 1; i < shape.size(); i++) {
					size *= shape[i];
				}
				if (size == std::valarray<T>::size()) {
					_shape = shape;
					_size = std::valarray<size_t>(_shape);
					for (size_t i = shape.size() - 1; i > 0; i--) {
						_size[i-1] *= _size[i];
					}
				} else {
					throw std::length_error("ndarray cannot take the given shape");
				}
			}
			
			//returns the shape of the ndarray
			std::valarray<size_t> shape() {
				return _shape;
			}
			
			//returns a particular index in the ndarray
			template<typename... Ds>
			T index(Ds... spec) {
				if (sizeof...(spec) == _size.size()) {
					return std::valarray<T>::operator[](index<sizeof...(Ds),0>(spec...));
				} else {
					throw std::out_of_range("ndarray must be indexed with proper dimensions");
				}
			}
			
			//returns an indirect_array (flat) representing the given arbitrary dimensional slice
			template <typename... Ds>
			std::indirect_array<T> take(Ds... spec) {
				std::vector<size_t> shape(0);
				std::valarray<size_t> indexes = slice<sizeof...(Ds),0>(shape,spec...);
				return std::valarray<T>::operator[](indexes);
			}
			
			//returns an ndslice (shaped) representing the given arbitrary dimensional slice
			template <typename... Ds>
			ndslice<T> slice(Ds... spec) {
				std::vector<size_t> shape_cache(0);
				std::valarray<size_t> indexes = slice<sizeof...(Ds),0>(shape_cache,spec...);
				std::valarray<size_t> shape(shape_cache.size() == 0 ? 1 : shape_cache.size());
				if (shape_cache.size() == 0) {
					shape[0] = 1;
				} else {
					for (size_t i = 0; i < shape.size(); i++) shape[i] = shape_cache[i];
				}
				return ndslice<T>(this,indexes,shape);
			}
			
		protected:
			std::valarray<size_t> _shape, _size;
			
			//computes the index of a particular position in the ndarray (negative counts from last element -1)
			template<int REM, int DEP, typename D, typename... Ds>
			size_t index(D first, Ds... rest) {
				return (first >= 0 ? first : first + _size[DEP])*_size[DEP+1] + index<REM-1,DEP+1>(rest...);
			}
			
			//end conditioin for index
			template<int REM, int DEP, typename D, typename... Ds>
			size_t index(D first) {
				return (first >= 0 ? first : first + _size[DEP]);
			}
			
			//computes the indexes of an arbitrary slice in arbitrary dimensions (negative counts from last element -1)
			template<int REM, int DEP, typename D, typename... Ds>
			std::valarray<size_t> slice(std::vector<size_t> &shape, D first, Ds... rest) {
				std::valarray<size_t> here = level<DEP>(first);
				if (here.size() > 1) {
					shape.push_back(here.size());
				}
				std::valarray<size_t> sub = slice<REM-1,DEP+1>(shape, rest...);
				std::valarray<size_t> indexes(here.size()*sub.size());
				for (int i = 0; i < here.size(); i++) {
					for (int j = 0; j < sub.size(); j++) {
						indexes[i*sub.size()+j] = sub[j]+here[i]*_size[DEP+1];
					}
				}
				return indexes;
			}
			
			//end condition for slice
			template<int REM, int DEP, typename D>
			inline std::valarray<size_t> slice(std::vector<size_t> &shape, D first) {
				std::valarray<size_t> here = level<DEP>(first);
				if (here.size() > 1) {
					shape.push_back(here.size());
				}
				if (DEP+1 < _shape.size()) {
					std::valarray<size_t> sub = range_excl<size_t>(0,_size[DEP+1]);
					for (size_t i = DEP+1; i < _size.size(); i++) {
						shape.push_back(_size[i]);
					}
					std::valarray<size_t> indexes = std::valarray<size_t>(here.size()*sub.size());
					for (int i = 0; i < here.size(); i++) {
						for (int j = 0; j < sub.size(); j++) {
							indexes[i+here.size()*j] = sub[j]+here[i]*_size[DEP+1];
						}
					}
					return indexes;
				} else {
					return here;
				}
			}
			
			//handles slicing by index
			template <int DEP>
			inline std::valarray<size_t> level(int idx) {
				std::valarray<size_t> indexes(idx < 0 ? idx + _shape[DEP] : idx,1);
				return indexes;
			}
			
			//handles slicing by index list
			template <int DEP>
			inline std::valarray<size_t> level(std::valarray<int> list) {
				const size_t dim = _shape[DEP];
				std::valarray<size_t> indexes(list.size());
				for (size_t i = 0; i < list.size(); i++) {
					if (list[i] < 0) {
						indexes[i] = list[i] + dim;
					} else {
						indexes[i] = list[i];
					}
				}
				return indexes;
			}

			//handles slicing by a range
			template <int DEP>
			inline std::valarray<size_t> level(std::tuple<int,int> range) {
				int begin = std::get<0>(range);
				if (begin < 0) begin += _shape[DEP];
				int end = std::get<1>(range);
				if (end < 0) end += _shape[DEP];
				return range_incl<size_t>(begin,end);
			}

			//handles slicing by a range with a step 
			template <int DEP>
			inline std::valarray<size_t> level(std::tuple<int,int,int> range) {
				int begin = std::get<0>(range);
				if (begin < 0) begin += _shape[DEP];
				int end = std::get<1>(range);
				if (end < 0) end += _shape[DEP];
				return range_incl<size_t>(begin,end,std::get<2>(range));
			}
	};

};

#endif