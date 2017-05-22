/*
 * queue.cpp
 *
 *  Created on: Jun 8, 2012
 *      Author: snikolenko
 */
#include "queue.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>

template <typename T> void Queue<T>::init() {
	if (multiqueue) {
		for (size_t i=0; i < k; ++i ) {
			mq.push_back( Queue<T>("FIFO", k, B, 1, false) );
		}
		curq = 0;
	}
	preempting = true;
	if (type == "NFIFO") {
		preempting = false;
	}
	if (type == "2LFIFO") {
		leavepartiallyprocessed = true;
	}
	if (type == "PQ" || type == "LPQ" || type == "RevPQ") {
		sorting = true;
	}

	// length-aware policies
	if ( (type == "PQValue") || (type == "PQvalue") || (type == "PQWork") || (type == "PQVoverW") || (type == "PQVoverWV") || (type == "PQVoverWW") ) {
		preempting = true;
		sorting = true;
		// lengthaware = true;
	}
	if ( (type == "FOPTUL") || (type == "FOPTUW") ) {
		preempting = true;
		sorting = true;
		// lengthaware = true;
		fractional = true;
		B = B * L;
	}
}

template <typename T> void Queue<T>::increment_counters(int work, double length, int delay, int squared_delay) {
	total_processed += use_value ? length : 1;
	total_processed_length += length;
	total_delay += delay;
	total_squared_delay += squared_delay;
}

template <typename T> void Queue<T>::remove_head_packet(int tick_num) {
	typename vector< Packet<T> >::iterator it = (this->reversing || this->use_value) ? (q.end()-1) : q.begin();
	D("[" << tick_num << "]: processed " << (*it) << " adding " << (tick_num - it->arrival - (int)it->r + 1));
	int delay = tick_num - it->arrival - (this->use_value ? 0 : ((int)it->r + 1));
	increment_counters(it->r, it->l, delay, delay * delay);
	q.erase(it);
	if ( type != "FOPTUW" ) {
		D(type << " processed a packet!\ntotal_proc=" << total_processed << "\ttotal_len=" << total_processed_length << "\ttotal_delay=" << total_delay << "\tlen_left=" << totallength());
	}
}

template <typename T> void Queue<T>::process_head_packet(int tick_num) {
	if ( q.empty() ) return;
	else {
		if (!fractional) {
			if (lazy) {
				// lazy packet processing
				if ( q[0].r != 1 && sendingout ) sendingout = false;
				if (sendingout) this->remove_head_packet(tick_num);
				else {
					size_t non_one = 0;
					for ( ; (non_one < q.size()) && (q[non_one] == 1); ++non_one );
					if (non_one == q.size()) {
						sendingout = true;
						this->remove_head_packet(tick_num);
					} else {
						q[non_one].dec();
					}
				}
			} else {
				size_t holindex = this->reversing ? (q.size()-1) : 0;
				// common integer packet processing
				// if we are using packet->r for value, we remove each packet anyway
				if ( use_value || q[holindex].r == 1 ) this->remove_head_packet(tick_num);
				else {
					q[holindex].dec();
					if ( recycling ) {
						this->recycle();
					}
				}
			}
		} else {
			// fractional packet processing
			float work_left = 1;
			while ( !q.empty() && work_left >= q[0].r ) {
				work_left -= q[0].r;
				this->remove_head_packet(tick_num);
			}
			if ( !q.empty() ) {
				q[0].r -= work_left;
			}
		}
	}
}

template <typename T> void Queue<T>::print_queue(string pref) {
	if (multiqueue) {
		D("Multiqueue state for " << type);
		for (size_t i=0; i<k; ++i) {
			mq[i].print_queue("   ");
		}
	} else {
		ostringstream s; s << pref << "\t";
		for (typename vector<Packet<T> >::const_iterator it = q.begin(); it != q.end(); ++it) {
			s << *it << " ";
		}
		D(s.str());
	}
}

template <typename T> void Queue<T>::process_subqueue(int tick_num, int i) {
	D("  " << type << ": processing subqueue " << (i+1));
	curq = i;
	T prevtotal = mq[i].total_processed;
	uint prevtotallen = mq[i].total_processed_length;
	uint prevtotaldelay = mq[i].total_delay;
	uint prevtotalsqdelay = mq[i].total_squared_delay;
	mq[i].process(tick_num);
	if ( mq[i].total_processed > prevtotal) {
		total_processed++;
		total_processed_length += mq[i].total_processed_length - prevtotallen;
		total_delay += mq[i].total_delay - prevtotaldelay;
		total_squared_delay += mq[i].total_squared_delay - prevtotalsqdelay;
		D(type << " processed a packet!\ntotal_proc=" << total_processed << "total_len=" << total_processed_length);
	}
}

template <typename T> void Queue<T>::process(int tick_num) {
	for (size_t j=0; j<C; ++j) {
		if (multiqueue) {
			process_multiqueue(tick_num);
			continue;
		}
		if ( q.empty() ) return;
		process_head_packet(tick_num);
	}
}

template <typename T> void Queue<T>::recycle() {
	if (q.size() > 1) {
		Packet<T> hol = q[0];
		q[0] = q[q.size()-1];
		q[q.size()-1] = hol;
	}
}

template <typename T> int Queue<T>::get_next_queue(size_t cur) {
	D("get_next_queue cur=" << cur);
	size_t first_i = (cur == k-1) ? 0 : cur+1;
	size_t i = first_i;
	while (true) {
		if (i == k) i=0;
		if ( mq[i].has_packets() ) return i;
		if (i == cur) break;
		++i;
	}
	D("		return " << 0);
	return 0;
}

template <typename T> void Queue<T>::process_multiqueue(int tick_num) {
	//TODO: pm
	int res = -1;
	if (type == "MQF") {
		res = 0;
		for ( size_t i=0; i < k; ++i ) {
			if (mq[i].has_packets() ) {
				res = i; break;
			}
		}
	} else if (type == "MaxQF") {
		res = k-1;
		for ( int i=(int)k-1; i >= 0; --i ) {
			if (mq[i].has_packets() ) {
				res = i; break;
			}
		}
	} else if (type == "CRR") {
		res = get_next_queue(curq);
	} else if (type == "PRR") {
		if (mq[curq].has_packets() && mq[curq].hol() < (curq+1) ) res = curq;
		else res = get_next_queue(curq);
		D("  PRR curq=" << curq+1 << "  res=" << res+1);
	} else if (type == "LQF") {
		res = max_element( mq.begin(), mq.end(), compare_queues_by_length ) - mq.begin();
		D("  LQF longest=" << res+1);
	} else if (type == "SQF") {
		size_t minind = 0;
		size_t minlen = (mq[0].has_packets() ? mq[0].size() : (B+1));
		for ( size_t i=1; i < k; ++i ) {
			if (mq[i].has_packets() && mq[i].size() < minlen ) {
				minlen = mq[i].size(); minind = i;
			}
		}
		res = minlen > 0 ? minind : 0;
		D("  SQF shortest=" << res+1);
	} else {
		res = 0;
	}
	D("Processing queue " << (res+1));
	process_subqueue(tick_num, res);
}

template <typename T> void Queue<T>::add_packets( const vector<IntPacket> & v ) {
	D("Adding " << v.size() << " packets");
	double worst_priority = q.size() == 0 ? 0 : this->getPriority(q[q.size()-1]);
	for ( vector<IntPacket>::const_iterator it = v.begin(); it != v.end(); ++it ) {
		if (this->beta > 1 && this->B == this->q.size() && this->getPriority(*it) < worst_priority * this->beta ) {
			D("\t skip packet " << *it << " due to beta=" << this->beta << " and worst priority " << worst_priority);
			continue;
		}
		add_packet( *it, false );
	}
	if (sorting) {
		#ifdef DEBUG
			print_queue("Before sort: ");
		#endif
		doSort();
		#ifdef DEBUG
			print_queue("After sort: ");
		#endif
	}
	if (preempting) {
		#ifdef DEBUG
			print_queue("Before preempting: ");
		#endif
		if (lengthaware) {
			int preempt_to = B-2*L+1;
			if ( (type == "FOPTUL") || (type == "FOPTUW") ) preempt_to = B;
			D("Preempting to " << preempt_to << "\tB=" << B << " L=" << L);
			// preemption for length-aware policies
			double len = totallength();
			// uint cur_remove_candidate = q.size() - 1;
			while ( len > preempt_to + EPSILON ) {
				len -= q[q.size()-1].l;
				q.erase(q.begin()+(q.size()-1));
			}
		} else {
			D("Preempting to B=" << B);
			if (random_pushout) {
				// bool do_remove = bernoulli(q[cur_remove_candidate].l);
			} else {
				while ( q.size() > B ) {
					q.erase(q.begin()+(q.size()-1));
				}
			}
		}
	}
}

template <typename T> void Queue<T>::doSort() {
	// if (!lengthaware) {
	// 	if (use_value) {
	// 		sort(q.begin(), q.end(), sortLengthAsValue<T>);
	// 	} else {
	// 		sort(q.begin(), q.end());
	// 	}
	// } else {
	if ( (type == "PQValue") || (type == "PQvalue") || (type == "FOPTUW") ) {
		sort(q.begin(), q.end(), sortLength<T>);
	}
	else if ((type == "PQWork") || (type == "FOPTUL")) {
		sort(q.begin(), q.end(), sortWork<T>);
	}
	else if (type == "PQVoverW") {
		sort(q.begin(), q.end(), sortValue<T>);
	}
	else if (type == "PQVoverWV") {
		sort(q.begin(), q.end(), sortVoverWthenValue<T>);
	}
	else if (type == "PQVoverWW") {
		sort(q.begin(), q.end(), sortVoverWthenWork<T>);
	}
	// }
}

template <typename T> double Queue<T>::getPriority(const Packet<T> &p) {
	if ( (type == "PQVoverW") || (type == "PQVoverWV") || (type == "PQVoverWW") ) {
		return p.l / p.r;
	}
	else if ((type == "PQWork") || (type == "FOPTUL")) {
		return 1. / p.r;
	}
	else {
		return p.l;
	}
}

template <typename T> typename vector<Packet<T> >::iterator Queue<T>::get_max_to_preempt() {
	if (this->leavepartiallyprocessed) {
		typename vector<Packet<T> >::iterator res = q.end();
		for (typename vector<Packet<T> >::iterator it = q.begin(); it != q.end(); it++) {
			if (it->touched && (res == q.end() || it->r > res->r)) {
				res = it;
			}
		}
		return res;
	} else {
		return this->use_value ? min_element( q.begin(), q.end(), sortLengthAsValue<T> ) : max_element( q.begin(), q.end() );
	}
}

template <typename T> void Queue<T>::simple_add_packet( Packet<T> to_add, bool dosortifneeded ) {
	// preemption for preempting policies is done later in bulk
	bool wehavespace = (q.size() < B);
	if ( wehavespace ) {
		this->total_admitted++;
		q.push_back(to_add);
	} else {
		// preemption for length-aware policies is done in bulk
		if (preempting && !lengthaware) {
			typename vector<Packet<T> >::iterator it = this->get_max_to_preempt();
			if ( it != q.end() && (this->use_value ? (it->l < to_add.l * this->beta) : (it->r > to_add.r * this->beta)) ) {
				this->total_admitted++;
				q.erase( it );
				q.push_back(to_add);
			}
		}
		if (dosortifneeded && sorting) doSort();
	}
}

template <typename T> void Queue<T>::add_packet( IntPacket i, bool dosortifneeded ) {
	if (multiqueue) {
		mq[i.r - 1].add_packet( i, dosortifneeded );
		return;
	}
	int num_subpackets = 1;
	if (type == "FOPTUW") num_subpackets = i.r;
	if (type == "FOPTUL") num_subpackets = i.l;
	for (int spcount = 0; spcount < num_subpackets; ++spcount) {
		Packet<T> to_add = i;
		if (type == "FOPTUW") { to_add.r = 1; to_add.l =  (i.l / i.r); }
		if (type == "FOPTUL") { to_add.r = (i.r / i.l); to_add.l =  1; }
		this->simple_add_packet(to_add, dosortifneeded);
	}
}

template <typename T> double Queue<T>::totallength() const {
	double res = 0;
	for ( typename vector<Packet<T> >::const_iterator it = q.begin(); it != q.end(); ++it ) {
		res += it->l;
	}
	return res;
}

template class Queue<int>;
template class Queue<float>;
