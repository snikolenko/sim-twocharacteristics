/*
 * queue.hpp
 *
 *  Created on: Jun 8, 2012
 *      Author: snikolenko
 */

#ifndef QUEUE_HPP_
#define QUEUE_HPP_

#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <math.h>

#include "packet.hpp"

// #define DEBUG
#ifdef DEBUG
	#define D(a) cout << a << endl
#else
	#define D(a)
#endif

#define EPSILON 0.001

using namespace std;
typedef unsigned int uint;

// forward declaration
template <typename T> struct t_compare_queues_by_length;

template <typename T> class Queue {
public:
	Queue(string iType, uint ik, uint iB, uint iC, bool iMultiqueue) : type(iType), B(iB), C(iC), L(1), k(ik), beta(1),
			total_processed(0), total_processed_length(0), total_admitted(0), total_delay(0), total_squared_delay(0),
			multiqueue(iMultiqueue), recycling(false), preempting(false), sorting(false), lengthaware(false),
			fractional(false), lazy(false), reversing(false), leavepartiallyprocessed(false), random_pushout(false), use_value(false), sendingout(false) {
		init();
	}
	Queue(string iType, uint ik, uint iB, uint iC, bool iMultiqueue, double bt) : type(iType), B(iB), C(iC), L(1), k(ik), beta(bt),
			total_processed(0), total_processed_length(0), total_admitted(0), total_delay(0), total_squared_delay(0),
			multiqueue(iMultiqueue), recycling(false), preempting(false), sorting(false), lengthaware(false),
			fractional(false), lazy(false), reversing(false), leavepartiallyprocessed(false), random_pushout(false), use_value(false), sendingout(false) {
		init();
	}
	Queue(string iType, uint ik, uint iB, uint iC, uint iL, bool iMultiqueue) : type(iType), B(iB), C(iC), L(iL), k(ik), beta(1),
			total_processed(0), total_processed_length(0), total_admitted(0), total_delay(0), total_squared_delay(0),
			multiqueue(iMultiqueue), recycling(false), preempting(false), sorting(false), lengthaware(false),
			fractional(false), lazy(false), reversing(false), leavepartiallyprocessed(false), random_pushout(false), use_value(false), sendingout(false) {
		init();
	}

	string type;
	size_t B;
	size_t C;
	size_t L;
	size_t k;

	double beta;

	T total_processed;
	double total_processed_length;
	int total_admitted;
	unsigned int total_delay;
	unsigned int total_squared_delay;

	bool multiqueue;
	bool recycling;
	bool preempting;
	bool sorting;
	bool lengthaware;
	bool fractional;
	bool lazy;
	bool reversing;
	bool leavepartiallyprocessed;
	bool random_pushout;

	bool use_value;
	bool sendingout;

	vector< Packet<T> > q;
	vector< Queue > mq;
	size_t curq;

	t_compare_queues_by_length<T> compare_queues_by_length;

	bool has_packets() const { return !q.empty(); }
	const Packet<T> hol() const { return (q.empty() ? Packet<T>(-1, -1) : q[0]); }
	typename vector<Packet<T> >::iterator get_max_to_preempt();
	virtual size_t size() const { return q.size(); }
	double totallength() const;
	void increment_counters(int work, int length, int delay, int squared_delay);

	void print_queue(string pref);

	void process_head_packet(int tick_num);
	void remove_head_packet(int tick_num);
	void simple_add_packet( Packet<T> to_add, bool dosortifneeded );
	void add_packet( IntPacket i, bool dosortifneeded = true );
	void add_packets( const vector<IntPacket> & v );

	virtual void process(int tick_num);
	void process_multiqueue(int tick_num);
	void process_subqueue(int tick_num, int i);
	void doSort();
	void init();
	
	virtual void do_use_value() { use_value = true; }
	
	int get_next_queue(size_t cur);

	void recycle();
};

#define OUT_SSQ(a) "[" << (a).k << ":" << (a).size << ":" << (a).hol << "] "
#define OUT_THIS_SSQ "[" << k << ":" << size << ":" << hol << "] "
class SharedSubQueue {
public:
	size_t hol;
	int k;
	multiset<int> q; // only for use_value
	bool use_value;
	size_t size;

	SharedSubQueue(int ik) : hol(0), k(ik), use_value(false), size(0) {}

	int work() const {
		if (!use_value) return (size == 0 ? 0 : ((size-1) * k + hol));
		return accumulate(q.begin(), q.end(), 0);
	}
	double ratio() const {
		if (!use_value) return (size / (double)k);
		return (size == 0) ? 0 : (size * size / (double)work());
	}
	int min_val() const {
		return (use_value && size) ? *q.begin() : (1 << 15);
	}
	void drop_packet() {
		if (size == 0) {
			cout << "\t\t\tERROR: dropping packet from empty queue" << endl;
		} else {
			size--;
			if (!use_value) {
				if (size == 0) hol = 0;
			} else {
				q.erase(q.begin());
			}
		}
	}
	void add_packet(const IntPacket & p) {
		size++;
		if (!use_value) {
			if (size == 0) hol = k;
		} else {
			q.insert(p.l);
		}
	}
	int one_tick(bool use_value = false) {
		D("\t\t\tone tick for " << OUT_THIS_SSQ);
		if (size == 0) return 0;
		if (use_value) { 		// if we are using value all processing requirements are unit
			size--;
			int best_val = *prev(q.end());
			q.erase(prev(q.end()));
			return best_val;
		} else if (hol == 1) {		// without value we need hol packet to have work 1 left
			size--;
			hol = size > 0 ? k : 0;
			return 1;
		}
		// default case: nothing gets processed
		if (hol > 0) hol--;
		return 0;
	}
	void do_use_value() { use_value = true; }
	string print_full() const {
		stringstream ss;
		ss << "[";
		for (auto it = q.begin(); it != q.end(); ++it) {
			ss << " " << *it;
		}
		ss << " ]";
		return ss.str();
	}
};

#define OUT_SSQPQ(a) "VW[" << (a).q.size() << ":" << ((a).q.empty() ? 0 : (a).q[0].r) << "] "
#define OUT_THIS_SSQPQ "VW[s" << q.size() << " h" << (q.empty() ? 0 : q[0].r) << "] "
class SharedSubQueuePQ {
public:
	vector<IntPacket> q;
	bool fifo;

	SharedSubQueuePQ(bool f) : fifo(f) { }

	size_t size() const { return q.size(); }
	int max_val() const { return ( q.empty() ? 0 : q.rbegin()->r ) ; }
	int work() const {
		int res = 0;
		for (size_t i=0; i<q.size(); ++i) {
			res += q[i].r;
		}
		return res;
	}
	void drop_packet() {
		if (q.empty()) {
			cout << "\t\t\tERROR: dropping packet from empty queue" << endl;
		} else {
			if (fifo) {
				q.erase( max_element( q.begin(), q.end(), sortWork<int> ) );
			} else {
				q.erase(prev(q.end()));
			}
		}
	}
	void add_packet(const IntPacket & p) {
		if (!fifo) {
			for (auto it = q.begin(); it != q.end(); ++it) {
				if (it->r > p.r) {
					q.insert(it, p);
					return;
				}
			}
		}
		q.push_back(p);
	}
	int one_tick() {
		D("\t\t\tone tick for " << OUT_THIS_SSQPQ);
		if (q.empty()) return 0;
		if (q[0].r == 1) {
			q.erase(q.begin());
			return 1;
		} else {
			q[0].r--;
			return 0;
		}
	}
	string print_full() const {
		stringstream ss;
		ss << "[";
		for (auto it = q.begin(); it != q.end(); ++it) {
			ss << " " << it->r;
		}
		ss << " ]";
		return ss.str();
	}
};

template <typename T> struct t_compare_queues_by_length {
	bool operator () ( const Queue<T> & q1, const Queue<T> & q2 ) {
		return q1.size() < q2.size();
	}
};

class QueueContainer {
public:
	virtual const string & type() const = 0;
	virtual size_t B() const = 0;
	virtual size_t C() const = 0;
	virtual size_t L() const = 0;
	virtual size_t k() const = 0;
	virtual double beta() const = 0;

	virtual unsigned int total_delay() const = 0;
	virtual unsigned int total_squared_delay() const = 0;
	virtual unsigned int total_processed() const = 0;
	virtual unsigned int total_processed_length() const = 0;
	virtual unsigned int final_total_processed() const = 0;
	virtual unsigned int total_admitted() const = 0;
	virtual double final_total_processed_length() const = 0;

	virtual void add_packets( const vector<IntPacket> & v ) = 0;
	virtual void process(int tick_num) = 0;
	virtual void print_queue(string pref) = 0;
	virtual void do_use_value() = 0;
};

class IntQueueContainer : public QueueContainer {
public:
	IntQueueContainer(string iType, uint ik, uint iB, uint iC, bool iMultiqueue) : q(iType, ik, iB, iC, iMultiqueue) {};
	IntQueueContainer(string iType, uint ik, uint iB, uint iC, bool iMultiqueue, double bt) : q(iType, ik, iB, iC, iMultiqueue, bt) {};
	IntQueueContainer(string iType, uint ik, uint iB, uint iC, uint iL, bool iMultiqueue) : q(iType, ik, iB, iC, iL, iMultiqueue) {};

	virtual const string & type() const { return q.type; }
	virtual size_t B() const { 
		if ( (q.type == "FOPTUL") || (q.type == "FOPTUW") ) {
			return (q.B / q.L);
		} else {
			return q.B;
		}
	}
	virtual size_t C() const { return q.C; }
	virtual size_t L() const { return q.L; }
	virtual size_t k() const { return q.k; }
	virtual double beta() const { return q.beta; }

	virtual unsigned int total_delay() const { return q.total_delay; }
	virtual unsigned int total_squared_delay() const { return q.total_squared_delay; }
	virtual unsigned int total_processed() const { return q.total_processed; }
	virtual unsigned int total_admitted() const { return q.total_admitted; }
	virtual unsigned int total_processed_length() const { return q.total_processed_length; }
	virtual unsigned int final_total_processed() const { return q.total_processed + q.size(); }
	virtual double final_total_processed_length() const { return q.total_processed_length + q.totallength(); }

	virtual void add_packets( const vector<IntPacket> & v ) { q.add_packets(v); }
	virtual void process(int tick_num) { q.process(tick_num); }
	virtual void print_queue(string pref) { q.print_queue(pref); }
	virtual void do_use_value() { q.do_use_value(); }
private:
	Queue<int> q;
};

#endif /* QUEUE_HPP_ */
