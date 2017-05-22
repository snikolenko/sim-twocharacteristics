//============================================================================
// Name        : queuesim.cpp
// Author      : Sergey Nikolenko
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <boost/random.hpp>
#include <boost/random/poisson_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/program_options.hpp>
#include "genpacket.hpp"
#include "queue.hpp"

// #define DEBUG

using namespace std;
using namespace boost;
namespace po = boost::program_options;

int num_runs = 5000000;
int max_threads = 1;

int min_usernum = 5;
int max_usernum = 25;
int default_usernum = 10;
int min_psnum = 5;
int max_psnum = 15;
int default_psnum = 10;
int min_maxval = 3;
int max_maxval = 100;
int step_maxval = 1;
int default_maxvalue = 10;
int max_capnum = 20;

int default_firstcap = 1;

double default_lmb = 0.25;

string caida_infile = "";
PacketGenerator<int>* caida_pg = NULL;

// networking variables

int k_min = 10;
int k_max = 10;
int k_step = 2;
int b_min = 10;
int b_max = 10;
int b_step = 1;
int c_min = 1;
int c_max = 1;
int l_min = 1;
int total_l = 1;


double default_beta = 1;
double beta = default_beta;


bool only_two_inputs = false;
bool two_different_inputs = false;
bool no_smart = false;
bool no_depth_two = false;
bool use_value = false;

bool twovalued_uniform = false;
bool twovalued_biased = false;
bool gen_biased = false;

void one_tick (int tick_num, PacketGenerator<int> * pg, vector<QueueContainer *> & v, uint k, bool add_packets = true) {
	D("\t\ttick");
	vector<IntPacket> p;
	if (add_packets) {
		p = pg->gen_packets();
		for (vector<IntPacket>::iterator it = p.begin(); it != p.end(); ++it) {
			it->setArrival(tick_num);
			D((*it) << " ");
		}
	}
	for (vector<QueueContainer *>::iterator it = v.begin(); it != v.end(); ++it) {
		D("\n\n\t### " << (*it)->type() << " ###");
		#ifdef DEBUG
			ostringstream s; for( vector<IntPacket>::const_iterator itt = p.begin(); itt != p.end(); ++itt ) s << *itt << " ";
			D("adding packets: " << s.str());
		#endif
		(*it)->add_packets( p );
		#ifdef DEBUG
			(*it)->print_queue("Added:");
		#endif
		(*it)->process(tick_num);
		#ifdef DEBUG
			(*it)->print_queue("Procd:");
		#endif
	}
}

void populate_policy_vector( vector<QueueContainer *> & v, uint k, uint min_B, uint max_B, uint B_step, uint min_C, uint max_C, uint min_L, uint max_L, double beta ) {
	v.clear();
	for (uint B = min_B; B <= max_B; B+=B_step ) {
		for (uint C = min_C; C <= max_C; ++C ) {
			for (uint L = min_L; L <= max_L; ++L ) {
				D("Generating: " << k << "\t" << B << "\t" << C << "\t" << L);
				v.push_back( new IntQueueContainer("FOPTUW", k, B, C, L, false, beta) );
				v.push_back( new IntQueueContainer("PQvalue", k, B, C, L, false, beta) );
				v.push_back( new IntQueueContainer("PQWork", k, B, C, L, false, beta) );
				v.push_back( new IntQueueContainer("PQVoverWV", k, B, C, L, false, beta) );
				v.push_back( new IntQueueContainer("PQVoverWW", k, B, C, L, false, beta) );
			}
		}
	}
}

// packet generator parameters
double off_on_prob, on_off_prob, lmb, large_lmb;
int num_streams;

void network_runsim(int k, int val, int b_min, int b_max, int b_step, int c_min, int c_max, double beta, int num_runs) {
	D("\toff_on_prob=" << off_on_prob << "\ton_off_prob=" << on_off_prob << "\tlmb=" << lmb << "\tlarge_lmb=" << large_lmb << "\tnum_streams=" << num_streams);
	D("\tmaxval = " << val);

	// creating a generator
	PacketGenerator<int> * pg;
	if (caida_infile != "" && caida_infile != "None") {
		if (caida_pg != NULL) {
			pg = caida_pg;
			pg->reset(k, val, large_lmb);
		} else {
			pg = (PacketGenerator<int>*)(new CAIDAPacketGenerator<int>(k, val, caida_infile, large_lmb));
		}
	} else {
		if (twovalued_biased) {
			pg = (PacketGenerator<int>*)(new MMPPVectorPoissonPacketGenerator<int, MMPPoissonTwoValuedBiasedPacketGenerator<int> >(k, val, off_on_prob, on_off_prob, lmb, large_lmb, num_streams));
		} else if (twovalued_uniform) {
			pg = (PacketGenerator<int>*)(new MMPPVectorPoissonPacketGenerator<int, MMPPoissonTwoValuedUniformPacketGenerator<int> >(k, val, off_on_prob, on_off_prob, lmb, large_lmb, num_streams));
		} else if (gen_biased) {
			pg = (PacketGenerator<int>*)(new MMPPVectorPoissonPacketGenerator<int, MMPPoissonBiasedPacketGenerator<int> >(k, val, off_on_prob, on_off_prob, lmb, large_lmb, num_streams));
		} else {
			pg = (PacketGenerator<int>*)(new MMPPVectorPoissonPacketGenerator<int>(k, val, off_on_prob, on_off_prob, lmb, large_lmb, num_streams));
		}
	}

	D("\tgenerator created");

	// populating vector of algorithms
	vector<QueueContainer *> v;
	populate_policy_vector(v, k, b_min, b_max, b_step, c_min, c_max, val, val, beta);

	// main loop
	for (int i=0; i < num_runs; ++i) {
		one_tick(i, pg, v, k);
		// emulate flush and restart
		if (i > 0 && i % 100000 == 0) {
			for (int j=0; j<k*b_max; ++j) {
				one_tick(i+j, pg, v, k, false);
			}
			i = i+k*b_max-1;
		}
	}

	// printing out the results
	#pragma omp critical
	{
		cout << "lambda=" << lmb << "\tlambda_on=" << large_lmb << "\tk=" << k << "\tv=" << val <<
				"\ttotal_packets=" << pg->total_packets <<
				"\ttotal_packets_len=" << pg->total_packets_len <<
				"\tnum_streams=" << num_streams << endl;
		// double max_total_length = v[0]->final_total_processed_length();
		for (vector<QueueContainer *>::iterator it = v.begin(); it != v.end(); ++it) {
			cout << (*it)->type().c_str() << "\tlambda=" << large_lmb << "\tk=" << (*it)->k() << "\tv=" << val << "\tB=" << (*it)->B()
					<< "\tC=" << (*it)->C() << "\tbeta=" << std::setprecision(2) << (*it)->beta() << "\t"
					<< (*it)->final_total_processed() << "\t"
					<< int((*it)->final_total_processed_length()) << "\t"
					// << std::setprecision(5) << double((*it)->final_total_processed_length() / max_total_length) << "\t"
					<< (*it)->total_admitted() << "\t"
					<< std::setprecision(5) << ((*it)->total_delay() / (double)(*it)->total_processed()) << "\t"
					<< std::setprecision(5) << sqrt((*it)->total_squared_delay()) / (double)(*it)->total_processed()
					<< endl;
		}
	}
}


int network_main() {
	srand((unsigned)time(0));

	double start_l = 0;
	double step_l = 0;
	// double large_l_start = 5;
	// double large_l_step = 10;
	double large_l_start = 0.025;
	double large_l_step = 0.01;
	if (caida_infile != "" && caida_infile != "None") {
		large_l_start = 0.00001;
		large_l_step = 0.00005;
	}

	num_streams = 500;
	off_on_prob = 0.01;
	on_off_prob = 0.2;

	size_t SIM_SIZE = num_runs;

	vector< vector< vector<QueueContainer *> > > v(total_l);
	vector< vector<int> > total_packets(total_l);
	vector< vector<int> > total_packets_len(total_l);

	omp_set_num_threads(2);

	for ( int l=0; l < total_l; ++l ) {
		lmb = start_l + l * step_l;
		large_lmb = large_l_start + l * large_l_step;
		if (total_l == 1) large_lmb = default_lmb;
		for ( int k=k_min; k <= k_max; k += k_step ) {
			for ( int v=min_maxval; v <= max_maxval; v += step_maxval ) {
			// num_streams = k * max_length;
			// large_lmb = (large_l_start + l * large_l_step) / (double)num_streams;
				D("Experiment: k=" << k << "\tv=" << v << "\tb_min=" << b_min << "\tb_max=" << b_max << "\tb_step=" << b_step << "\tbeta=" << beta << "\truns=" << SIM_SIZE);
				network_runsim(k, v, b_min, b_max, b_step, c_min, c_max, beta, SIM_SIZE);
			}
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	//return network_main();
	// Declare the supported options.

	srand(time(NULL));

	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help,?", "produce help message")
	    ("max_threads,t", po::value<int>(&max_threads)->default_value(1), "number of threads to run")
	    ("num_runs", po::value<int>(&num_runs)->default_value(10000), "number of simulation runs")
	    ("caida", po::value<string>(&caida_infile)->default_value(""), "CAIDA input file")
	    ("twovalued_uniform", "two-valued uniform Poisson generator")
	    ("twovalued_biased", "two-valued biased Poisson generator")
	    ("gen_biased", "biased Poisson generator")
	    ("min_psnum", po::value<int>(&min_psnum)->default_value(5), "min number of inputs")
	    ("max_psnum", po::value<int>(&max_psnum)->default_value(15), "max number of inputs")
	    ("psnum", po::value<int>(&default_psnum)->default_value(10), "default number of ports")
	    ("min_maxval", po::value<int>(&min_maxval)->default_value(10), "min maximal value")
	    ("max_maxval", po::value<int>(&max_maxval)->default_value(10), "max maximal value")
	    ("step_maxval", po::value<int>(&step_maxval)->default_value(1), "maximal value step")
	    ("maxval", po::value<int>(&default_maxvalue)->default_value(10), "default maximal value")
	    ("firstcap", po::value<int>(&default_firstcap)->default_value(1), "default first port capacity")
	    ("k_min", po::value<int>(&k_min)->default_value(10), "min k")
	    ("k_max", po::value<int>(&k_max)->default_value(10), "max k")
	    ("k_step", po::value<int>(&k_step)->default_value(2), "step k")
	    ("b_min", po::value<int>(&b_min)->default_value(50), "min b")
	    ("b_max", po::value<int>(&b_max)->default_value(50), "max b")
	    ("b_step", po::value<int>(&b_step)->default_value(1), "step b")
	    ("c_min", po::value<int>(&c_min)->default_value(1), "min c")
	    ("c_max", po::value<int>(&c_max)->default_value(1), "max c")
	    ("l_min", po::value<int>(&l_min)->default_value(1), "min l")
	    ("total_l", po::value<int>(&total_l)->default_value(1), "total l")
	    ("beta", po::value<double>(&beta)->default_value(1.0), "beta")
	    ("lmb", po::value<double>(&default_lmb)->default_value(0.25), "lambda")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
	    cout << desc << "\n";
	    return 1;
	}

	if (vm.count("twovalued_biased")) twovalued_biased = true;
	if (vm.count("twovalued_uniform")) twovalued_uniform = true;
	if (vm.count("gen_biased")) gen_biased = true;

	return network_main();
}

