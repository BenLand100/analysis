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
 
#ifndef _FUNCTIONAL_HH
#define _FUNCTIONAL_HH

#include <valarray>
#include <functional>
#include <algorithm>
#include <vector>
#include <thread>

namespace analysis {

	//generates a list of [from,to] in increments of step. last is <= to with precision of step
	template<typename T> 
	std::valarray<T> range_incl(T from, T to, T step = 1) {
		size_t elems = (size_t)floor((to-from)/step) + 1;
		std::valarray<T> result(elems);
		for (size_t i = 0; i < elems; i++) {
			result[i] = from+step*i;
		}
		return result;
	}
	
	//generates a list of [begin,end) in increments of step. last is < end with precision of step
	template<typename T> 
	std::valarray<T> range_excl(T begin, T end, T step = 1) {
		size_t elems = (size_t)floor((end-begin)/step);
		std::valarray<T> result(elems);
		for (size_t i = 0; i < elems; i++) {
			result[i] = begin+step*i;
		}
		return result;
	}
	
	//end condition for fill (does nothing)
	template<int N, typename T> void fill(std::valarray<T> &list) { }
	
	//inserts the arguments into the list starting from index N
	template<int N, typename T, typename... Ts>
	void fill(std::valarray<T> &list, T first, Ts... rest) {
		list[N] = first;
		fill<N+1,T>(list,rest...);
	}
	
	//makes a list of the arguments
	template<typename T, typename... Ts>
	std::valarray<T> list(T first, Ts... rest) {
		std::valarray<T> result(1+sizeof...(rest));
		fill<0>(result,first,rest...);
		return result;
	}
	
	//returns a subset of a list with the given parameters, negative values count from the end (-1)
	template<typename T> 
	std::valarray<T> take(std::valarray<T> &in, size_t from, size_t to, size_t step = 1) {
		const size_t size = in.size();
		if (from < 0) from += size;
		if (to < 0) to += size;
		const size_t elems = (to-from+1)/step;
		std::valarray<T> out(elems);
		for (size_t i = 0; i < elems; i++) {
			out[i] = in[i*step+from];
		}
		return out;
	}
	
	//applies func to each set of arguments taken sequentially from the given lists and returns list of results
	template<typename F, typename T, typename... Ts> 
	std::valarray<decltype(std::declval<F>()(std::declval<T>(),std::declval<Ts>()...))> map(F func, std::valarray<T> &in, std::valarray<Ts>&... rest) {
		std::valarray<decltype(func(in[0],rest[0]...))> out(in.size());
		for (size_t i = 0; i < in.size(); i++) {
			out[i] = func(in[i],rest[i]...);
		}
		return out;
	}

	//applies func to each set of arguments taken sequentially from the given lists
	template<typename F, typename T, typename... Ts> 
	void mapv(F func, std::valarray<T> &in, std::valarray<Ts>&... rest) {
		for (size_t i = 0; i < in.size(); i++) {
			func(in[i],rest[i]...);
		}
	}
	
	//functor to spawn threads to map lists
	template<typename F, typename T, typename... Ts> 
	struct mapv_worker {
		inline void operator()(const size_t begin, const size_t end, const F &func, std::valarray<T> &in, std::valarray<Ts>&... rest) {
			for (size_t i = begin; i < end; i++) {
				func(in[i],rest[i]...);
			}
		}
	};

	//applies func to each set of arguments taken sequentially from the given lists
	template<typename F, typename T, typename... Ts> 
	void parallelmapv(const size_t nthreads, F func, std::valarray<T> &in, std::valarray<Ts>&... rest) {
		const size_t njobs = in.size();
		const size_t jobs_per_thread = njobs/nthreads;
		const size_t remainder = jobs_per_thread*nthreads - njobs;
		std::vector<std::thread> threads(nthreads);
		mapv_worker<F,T,Ts...> worker;
		for (size_t i = 0; i < nthreads; i++) {
			const size_t begin = jobs_per_thread*i;
			const size_t end = i == nthreads-1 ? njobs : jobs_per_thread*(i+1);
			threads[i] = std::thread(worker,begin,end,std::ref(func),std::ref(in),std::ref(rest)...);
		}
		for (std::thread &thread : threads) {
			thread.join();
		}
	}
	
	//functor to spawn threads to map lists and collect results
	template<typename F, typename T, typename... Ts> 
	struct map_worker {
		inline void operator()(const size_t begin, const size_t end, std::valarray<decltype(std::declval<F>()(std::declval<T>(),std::declval<Ts>()...))> &out, const F &func, std::valarray<T> &in, std::valarray<Ts>&... rest) {
			for (size_t i = begin; i < end; i++) {
				out[i] = func(in[i],rest[i]...);
			}
		}
	};

	//applies func to each set of arguments taken sequentially from the given lists
	template<typename F, typename T, typename... Ts> 
	std::valarray<decltype(std::declval<F>()(std::declval<T>(),std::declval<Ts>()...))> parallelmap(const size_t nthreads, F func, std::valarray<T> &in, std::valarray<Ts>&... rest) {
		std::valarray<decltype(func(in[0],rest[0]...))> out(in.size());
		const size_t njobs = in.size();
		const size_t jobs_per_thread = njobs/nthreads;
		const size_t remainder = jobs_per_thread*nthreads - njobs;
		std::vector<std::thread> threads(nthreads);
		map_worker<F,T,Ts...> worker;
		for (size_t i = 0; i < nthreads; i++) {
			const size_t begin = jobs_per_thread*i;
			const size_t end = i == nthreads-1 ? njobs : jobs_per_thread*(i+1);
			threads[i] = std::thread(worker,begin,end,std::ref(out),std::ref(func),std::ref(in),std::ref(rest)...);
		}
		for (std::thread &thread : threads) {
			thread.join();
		}
		return out;
	}

	//sorts the argument in place
	template<typename T, typename L> 
	void sortv(std::valarray<T> &in, L less = std::less<T>()) {
		std::sort(std::begin(in), std::end(in), less);
	} 

	//returns a sorted copy of the argument
	template<typename T, typename L> 
	std::valarray<T> sort(std::valarray<T> in, L less = std::less<T>()  ) {
		sortv(in,less);
		return in;
	} 

	//returns the indexes of the values in the argument sorted
	template<typename T, typename L> 
	std::valarray<size_t> sortidx(std::valarray<T> &in, L less = std::less<T>()) {
		std::valarray<size_t> idx = range_excl<size_t>(0,in.size());
		sortv(idx,[less,&in](size_t a, size_t b) { return less(in[a],in[b]); });
		return idx;
	} 

	//returns a list of the elements that passed the test
	template<typename T, typename F> 
	std::valarray<T> select(F test, std::valarray<T> &in) {
		std::valarray<bool> mask = map(test,in);
		return in[mask];
	}

	//dummy default equality functor
	template<typename T> struct _equals {
		bool operator()(const T &a, const T &b) {
			return a == b;
		}
	};

	//returns the unique elements in the argument
	template<typename T, typename E = _equals<T>, typename L = std::less<T>> 
	std::valarray<T> unique(std::valarray<T> in, E equal = _equals<T>(), L less = std::less<T>()) {
		//can this work with fewer than two copies?
		sortv(in,less);
		auto end = std::unique(std::begin(in),std::end(in),equal);
		size_t elems = std::distance(std::begin(in),end);
		std::valarray<T> result(elems);
		for (size_t i = 0; i < elems; i++) {
			result[i] = in[i];
		}
		return result;
	}
	
};

#endif