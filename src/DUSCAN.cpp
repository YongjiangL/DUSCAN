/*
 * DUSCAN.cpp
 *
 *  Created on: Feb 1, 18
 *      Author: torkit
 */

#include "DUSCAN.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>  // strtok
#include <cmath>
#include <algorithm>
#include <sstream>
#include <cctype>  // tolower
#include <set>
#include <queue>
#include <stdexcept>
#include <fstream>
#include <cerrno>
#include <ctime>
#include <cassert>
#include <iostream>
#include <sys/time.h>  // gettimeofday
#include "ProbSCAN.h"

using std::set;
using std::invalid_argument;
using std::domain_error;
using std::ios_base;
using std::ifstream;
using std::to_string;
using std::queue;

//Accessory Functions
//!< Convert string to lower case
void tolower2(char* text) {
	if (!text)
		return;
	while (*text)
		*text++ = tolower(*text);
}

namespace SCAN {

DUSCAN::DUSCAN(float aeps, float alpha, int amiu, const char *ainput) :
		input(ainput), n(0), m(0), eps(aeps), alpha(alpha), miu(amiu), pstart(
				nullptr), edges(nullptr), reverse(nullptr), min_cn(nullptr), pa(
				nullptr), rank(nullptr), cid(nullptr), degree(nullptr), similar_degree(
				nullptr), effective_degree(nullptr), noncore_cluster(), ieids(), prbs(
				nullptr), ndeg(nullptr), sn(nullptr), explr(nullptr) {
	if (aeps < 0 || aeps > 1) {
		throw invalid_argument("eps should E [0,1]");
	}
	if (aeps == 0 || aeps == 1) {
		eps_b2 = 1;
		eps_a2 = aeps;
		return;
	}
//eps_a2/eps_b2 = eps^2
	eps_b2 = 1e4;
	eps_a2 = round(aeps * eps_b2);
	eps_a2 *= eps_a2;
	eps_b2 *= eps_b2;
	oriMiu = miu;
	miu--;

}

DUSCAN::~DUSCAN() {
	if (pstart) {
		delete[] pstart;
		pstart = nullptr;
	}
	if (edges) {
		delete[] edges;
		edges = nullptr;
	}
	if (prbs) {
		delete[] prbs;
		prbs = nullptr;
	}
	if (reverse) {
		delete[] reverse;
		reverse = nullptr;
	}
	if (min_cn) {
		delete[] min_cn;
		min_cn = nullptr;
	}
	if (cid) {
		delete[] cid;
		cid = nullptr;
	}
	if (degree) {
		delete[] degree;
		degree = nullptr;
	}
	if (effective_degree) {
		delete[] effective_degree;
		effective_degree = nullptr;
	}
	if (similar_degree) {
		delete[] similar_degree;
		similar_degree = nullptr;
	}
	if (pa) {
		delete[] pa;
		pa = nullptr;
	}
	if (rank) {
		delete[] rank;
		rank = nullptr;
	}

	if (ndeg) {
		delete[] ndeg;
		ndeg = nullptr;
	}
	if (sn) {
		delete[] sn;
		sn = nullptr;
	}
	if (explr) {
		delete[] explr;
		explr = nullptr;
	}
}

void DUSCAN::load() {
	ifstream finp;
	finp.exceptions(ifstream::badbit); //  | ifstream::failbit  - raises exception on EOF
	finp.open(input);
	if (!finp.is_open()) {
		perror(("Error opening the file " + input).c_str());
		throw std::ios_base::failure(strerror(errno));
	}

	// Intermediate data structures to fill internal data structures
	unordered_map<string, Id> eiids; // Map from external to internal ids
	vector < set<pair<Id, float>>> nodes; // Nodes with arcs specified with internal ids

	// Parse NSE/A file
	string line;

	// Parse the header
	// [Nodes: <nodes_num>[,]	<Links>: <links_num>[,] [Weighted: {0, 1}]]
	// Note: the comma is either always present as a delimiter or always absent
	while (getline(finp, line)) {
		// Skip empty lines
		if (line.empty())
			continue;
		// Consider only subsequent comments
		if (line[0] != '#')
			break;

		// 1. Replace the staring comment mark '#' with space to allow "#clusters:"
		// 2. Replace ':' with space to allow "Clusters:<clsnum>"
		for (size_t pos = 0; pos != string::npos; pos = line.find(':', pos + 1))
			line[pos] = ' ';

		// Parse nodes num
		char *tok = strtok(const_cast<char*>(line.data()), " \t");
		if (!tok)
			continue;
		tolower2(tok);
		if (strcmp(tok, "nodes"))
			continue;
		// Read nodes num
		tok = strtok(nullptr, " \t");
		if (tok) {
			// Note: optional trailing ',' is allowed here
			n = strtoul(tok, nullptr, 10);
			// Read the number of links
			tok = strtok(nullptr, " \t");
			if (tok) {
				tolower2(tok);
				tok = strtok(nullptr, " \t");
				if (tok) {
					// Note: optional trailing ',' is allowed here
					m = strtoul(tok, nullptr, 10);
					// Read Weighted flag
					tok = strtok(nullptr, " \t");
					if (tok && (tolower2(tok), !strcmp(tok, "weighted"))
							&& (tok = strtok(nullptr, " \t"))
							&& strtoul(tok, nullptr, 10) != 0)
						fputs(
								"WARNING, the network is weighted and this algorithm does not support weights"
										", so the weights are omitted.\n",
								stderr);
				}
			}
		}
	}
	std::cout << "n: " << n << ", m: " << m << std::endl;

	// Preallocate containers if possible
	if (n) {
		nodes.reserve(n);
		eiids.reserve(n);
		ieids.reserve(n);
	}
	//	if (m)
	//		this->edgePrbs.reserve(m);

	// Parse the body
	// Note: the processing is started from the read line
	size_t iline = 0; // Payload line index, links counter
	do {
		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;

		char *tok = strtok(const_cast<char*>(line.data()), " \t");
		if (!tok)
			continue;

		string sid(tok); // External source id
		if (sid == "Nodes:")
			continue;

		tok = strtok(nullptr, " \t");
		if (!tok)
			throw domain_error(
					string(line).insert(0,
							"Destination link id is not specified in this line: ").c_str());
		string did(tok); // External destination id
		tok = strtok(nullptr, " \t");
		float prob = strtof(tok, nullptr);

		// Make the mappings and fill the nodes
		auto ies = eiids.find(sid); // Index of the external src id
		if (ies == eiids.end()) {
			ies = eiids.emplace(sid, eiids.size()).first;
			ieids.emplace(ies->second, sid);
			nodes.emplace_back();
		}
		auto ied = eiids.find(did); // Index of the external dst id
		if (ied == eiids.end()) {
			ied = eiids.emplace(did, eiids.size()).first;
			ieids.emplace(ied->second, did);
			nodes.emplace_back();
		}
		nodes[ies->second].insert(std::make_pair(ied->second, prob));
		//		fprintf(stderr, "+ arc: %u %u  [%u %u]\n", ies->second, ied->second, sid, did);
		// Insert back arc in case of edges
		nodes[ied->second].insert(std::make_pair(ies->second, prob));
		++iline;

		//		std::cout << eKey << ", " << prob << std::endl;
	} while (getline(finp, line));
	assert(
			eiids.size() == ieids.size() && nodes.size() == ieids.size()
					&& "Node mappings are not synchronized");

	// Initialize internal data structures using nodes
	if (n != nodes.size())
		n = nodes.size();

	const size_t arcsnum = iline * (1 + 1);
	if (m != arcsnum)
		m = arcsnum;


	if (reverse)
		delete[] reverse;
	reverse = new Id[m];
	memset(reverse, 0, sizeof(Id) * m);
	if (min_cn)
		delete[] min_cn;
	min_cn = new int[m];
	memset(min_cn, 0, sizeof(int) * m);

	if (edges)
		delete[] edges;
	edges = new Id[m];
	if (prbs)
		delete[] prbs;
	prbs = new float[m];
	if (pstart)
		delete[] pstart;
	pstart = new Id[n + 1];
	if (degree)
		delete[] degree;
	degree = new Degree[n];
	if (ndeg)
		delete[] ndeg;
	ndeg = new Degree[n];
	if (sn)
		delete[] sn;
	sn = new Degree[n];

	size_t in = 0;
	size_t ie = 0;
	pstart[0] = 0;
	float avgDeg = 0;
	float avgP = 0;
	for (auto& nd : nodes) {
		//		fprintf(stderr, "nodes[%lu] size: %lu\n", in, nd.size());
		int v = in;
		degree[in++] = nd.size();
		for (auto did : nd) {
			int u = did.first;
			float pp = did.second;
			//			string eKey;
			//			eKey.append(std::to_string(std::min(u, v)));
			//			eKey.append("---");
			//			eKey.append(std::to_string(std::max(u, v)));
			//            prbs[ie] = edgePrbs[eKey];
			prbs[ie] = pp;
			edges[ie++] = u;
			avgP += prbs[ie - 1];
			//           std::cout << "key: " << v << "|" << u << ", " << prbs[ie-1] << std::endl;
		}
		pstart[in] = ie;
		assert(
				ie == pstart[in - 1] + degree[in - 1]
						&& "pstart item validation failed");
	}
	if (ie != m) {
		fprintf(stderr, "ie: %lu, m: %u\n", ie, m);
		m = ie;
	}
}

int DUSCAN::get_common_neighbors(Id u, Id v) {
	if (degree[u] < 2 || degree[v] < 2)
		return 0;
	cmm_nghbr.clear();
	cmm_nghbr.resize(0);

	Id i = pstart[u], j = pstart[v];
	while (i < pstart[u + 1] && j < pstart[v + 1]) {
		if (edges[i] < edges[j]) {
			++i;
		} else if (edges[i] > edges[j]) {
			++j;
		} else {
			cmm_nghbr.emplace_back(i, j);
			++i;
			++j;
		}
	}
	return cmm_nghbr.size();
}

int DUSCAN::compute_common_neighbor_lowerbound(Id du, Id dv) {
	int c = (int) (sqrtl(
			(((long double) du) * ((long double) dv) * eps_a2) / eps_b2));
	if (((long long) c) * ((long long) c) * eps_b2
			< ((long long) du) * ((long long) dv) * eps_a2)
		++c;
	return c;
}

Id DUSCAN::binary_search(const Id *edges, Id b, Id e, Id val) {
	assert(b <= e && "Invalid indices");
	if (b == e || edges[e - 1] < val)
		return e;
	--e;
	while (b < e) {
		Id mid = b + (e - b) / 2;
		if (edges[mid] >= val)
			e = mid;
		else
			b = mid + 1;
	}
	return e;
}

Id DUSCAN::find_root(Id u) {
	Id x = u;
	while (pa[x] != x)
		x = pa[x];

	while (pa[u] != x) {
		Id tmp = pa[u];
		pa[u] = x;
		u = tmp;
	}

	return x;
}

void DUSCAN::my_union(Id u, Id v) {
	Id ru = find_root(u);
	Id rv = find_root(v);

	if (ru == rv)
		return;

	if (rank[ru] < rank[rv])
		pa[ru] = rv;
	else if (rank[ru] > rank[rv])
		pa[rv] = ru;
	else {
		pa[rv] = ru;
		++rank[ru];
	}
}

void DUSCAN::prune_and_cross_link() {
	for (Id i = 0; i < n; i++) { //must be iterating from 0 to n-1
		for (Id j = pstart[i]; j < pstart[i + 1]; j++) {
			if (edges[j] < i) {
				continue;
			}
			Id v = edges[j];
			Id r_id = binary_search(edges, pstart[v], pstart[v + 1], i);
			reverse[j] = r_id;
			reverse[r_id] = j;
		}
	}
}

} /* namespace SCAN */
