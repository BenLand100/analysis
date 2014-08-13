#include "functional.hh"
#include "ndarray.hh"

#include <iostream>

using namespace std;
using namespace analysis;

int main(int argc, char **argv) {
	
	cout << "range/map\n";
    valarray<double> vals = range_incl(0.0,6.28,0.5);
    mapv([](double i) { cout << i << ','; }, vals); cout << '\n';
    valarray<double> sins = map([](double i) { return sin(i); },vals);
    mapv([](double i) { cout << i << ','; }, sins); cout << '\n';
    valarray<double> pos = select([](double i) { return i > 0.0; }, sins);
    mapv([](double i) { cout << i << ','; }, pos); cout << '\n';
    
	cout << "unique\n";
    valarray<int> dups = list(1,1,5,6,2,4,1,5,1,4,2,5,3,7);
    valarray<int> uniq = unique(dups);
    mapv([](int i) { cout << i << ','; }, uniq); cout << '\n';
    
    auto first = range_incl(0.0,1.0,0.1);
    auto second = range_incl(0.0,10.0,1.0);
    auto third = range_incl(0.0,100.0,10.0);
	cout << "parallel\n";
    auto res  = parallelmap(4,[](double a, double b, double c) { return (a+b)*c; },first,second,third);
    mapv([](double i) { cout << i << ','; }, res); cout << '\n';
	
	auto flat = range_excl<double>(0,16);
	ndarray<double> arr(flat,list<size_t>(4,4));
	
	cout << "grab val:\n";
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			cout << arr.index(i,j) << ", ";
		}
		cout << '\n';
	}
	
	cout << "grab rows:\n";
	for (int i = 0; i < 4; i++) {
		ndarray<double> vals = arr.slice(i);
		mapv([](double idx) { cout << idx << ','; }, vals); cout << '\n';
	}
	
	cout << "grab cols:\n";
	for (int i = 0; i < 4; i++) {
		ndarray<double> vals = arr.slice(ALL,i);
		mapv([](double idx) { cout << idx << ','; }, vals); cout << '\n';
	}
	
	cout << "grab middle:\n";
	ndarray<double> middle = arr.slice(RANGE(1,2),RANGE(1,2));
	mapv([](double idx) { cout << idx << ','; }, middle); cout << '\n';
	
	cout << "grab corner:\n";
	ndarray<double> corner = arr.slice(RANGE(2,3),RANGE(2,3));
	mapv([](double idx) { cout << idx << ','; }, corner); cout << '\n';
	
	cout << "grab tips:\n";
	ndarray<double> tips = arr.slice(LIST(0,3),LIST(0,3));
	mapv([](double idx) { cout << idx << ','; }, tips); cout << '\n';
	
	cout << "grab last 3 elems of 0th and 3rd row\n";
	ndarray<double> hard = arr.slice(LIST(0,3),RANGE(1,3));
	mapv([](double idx) { cout << idx << ','; }, hard); cout << '\n';
    
}   
