/*
 * DUSCAN.h
 *
 *  Created on: Feb 1, 2018
 *      Author: torkit
 */

#ifndef DUSCAN_H_
#define DUSCAN_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <set>

typedef int Id;
typedef int Degree;

namespace SCAN {

using std::string;
using std::vector;
using std::unordered_map;
using std::pair;
using std::set;

class DUSCAN {
	string input; //Input graph
	Id n, m; //number of nodes and 2*edges

	float eps, alpha;
	int eps_a2, eps_b2; //eps_a2/eps_b2 = eps^2
	int miu, oriMiu;

	Id *pstart; //offset of neighbors of nodes
	Id *edges; //adjacent ids of edges
	float *prbs; //edge probabilities of adjacent ids
	Id *reverse; //the position of reverse edge in edges
	int *min_cn; //minimum common neighbor: -2 means not similar; -1 means similar; 0 means not sure; > 0 means the minimum common neighbor

	Id *pa;
	int *rank; //pa and rank used for disjoint-set data structure

	Id *cid; //cluster ids

	Degree *degree;
	Degree *similar_degree; //number of adjacent edges with similarity no less than epsilon
	Degree *effective_degree; //number of adjacent edges not pruned by similarity

	vector<pair<Id, Id>> noncore_cluster;
	unordered_map<Id, string> ieids; // Map from internal to external ids of nodes
	unordered_map<string, float> edgePrbs;

	vector<pair<Id, Id>> cmm_nghbr;
	vector<Id> visitEdges;
	Degree *ndeg;
	Degree *sn;
	bool *explr;
	vector<pair<Id, Id>> res_cluster; //<id, edgeId>
	int cluNum;
	float densitySum, pccSum;

public:
	DUSCAN(float aeps, float alpha, int amiu, const char *ainput);
	virtual ~DUSCAN();

	void load();
	int get_common_neighbors(Id u, Id v);
	int compute_common_neighbor_lowerbound(Id du, Id dv);
	Id binary_search(const Id *edges, Id b, Id e, Id val);

	Id find_root(Id u);
	void my_union(Id u, Id v);
	void cluster_noncore_vertices(int mu);
	void output(string file);

	void prune_and_cross_link();

};

} /* namespace SCAN */
#endif /* DUSCAN_H_ */
