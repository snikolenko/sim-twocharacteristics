/*
 * packet.cpp
 *
 *  Created on: Jun 30, 2012
 *      Author: snikolenko
 */

#include "packet.hpp"

bool bernoulli( double prob ) {
	return ( ( rand() / (double)RAND_MAX ) < prob );
}

int get_random_int(int min, int max) {
	return min + (rand() % (max-min+1));
}

