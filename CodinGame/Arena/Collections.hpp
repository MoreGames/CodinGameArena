#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "Random.hpp"

// a copy of Java shuffle

template <class T>
void shuffle(vector<T>& list, Random& rnd) {
	for (int i = (int)list.size(); i > 1; i--) {
		swap(list[i-1], list[rnd.nextInt(i)]);
	}
}

#endif