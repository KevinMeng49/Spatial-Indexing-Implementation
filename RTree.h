#ifndef RTREE_H_INCLUDED
#define RTREE_H_INCLUDED

#include "Geometry.h"
#include"QuadTree.h"   //Feature 
const int T = 3;
const int argM = 2;



namespace hw6 {

class RNode{
private:
	size_t M;  //max number of features in the leaf, nonleaf nodes (except the root) have between M/2 and M children
	size_t m;  //m equals M/argM, the minimum number in a node except root
	Envelope bbox;
	RNode* parent;
	vector<Feature> features;  //limt is M
	vector<RNode*> nodes ;     //limit is M
	
	RNode() {}
public:
	RNode(Envelope box, size_t M):M(M) {
		bbox = box;
		parent = NULL;
		m = M / argM;
	}

	RNode(Feature& f, size_t M) :M(M), parent(NULL){
		features.push_back(f);
		bbox = f.getEnvelope();
		m = M / argM;
	}

	RNode(RNode* son, size_t M) :M(M),parent(NULL) {
		nodes.push_back(son);
		son->setParent(this);
		bbox = son->getEnvelope();
		m = M / argM;
		
	}
	~RNode() {
		for (int i = 0; i < nodes.size(); i++) {
			delete nodes[i];
			nodes[i] = NULL;
		}
	}
	const Envelope& getEnvelope() { return bbox; }
	RNode* getChildNode(size_t i) { return nodes[i]; }

	size_t    getFeatureNum() { return features.size(); }
	size_t  getChildNum() { return nodes.size(); }
	Feature& getFeature(size_t i) {
		return features[i]; 
	}  //需要先判断是不是叶节点

	//add feature and update bbox
	void add(Feature& f)
	{ 
		
		features.push_back(f);
		//update envelope
		bbox = bbox.unionEnvelope(f.getEnvelope());
	}
	//add node and update bbox, parent
	void add(RNode* node) 
	{
		
		nodes.push_back(node);
		bbox = bbox.unionEnvelope(node->getEnvelope());
		node->parent = this;
	}

	void setParent(RNode* node)
	{
		parent = node;
	}
	void setEnvelope(Envelope env) { bbox = env; }
	void pop_backFeature() { features.pop_back(); }
	void pop_backNode() { nodes.pop_back(); }
	RNode* getParent() { return parent; }
	void draw();

	//delete certain node
	void deleteNode(RNode* p) 
	{
		vector<RNode*>::iterator it;
		it = find(nodes.begin(), nodes.end(), p);
		if(it!=nodes.end())
			nodes.erase(it);
	}

	void deleteFeature(Feature& f)
	{
		vector<Feature>::iterator it;
		it = find(features.begin(), features.end(), f);
		if (it != features.end())
			features.erase(it);
	}

	void rangeQuery(Envelope& rect, vector<Feature>& features);
	RNode* pointInLeafNode(double x, double y);
	int calculateLevel()
	{
		int level = 0;
		RNode* ptr = this;
		while (ptr->getChildNum() != 0)
		{
			level++;
			ptr = ptr->getChildNode(0);
		}
		return level;
	}
};
class NNTemp
{
public:
	Feature f;
	double nearestDist;
};


class ABN {
public:
	RNode* node;
	double MINDIST;
};


class ABL
{
public:
	vector<ABN> ABList;
};


class RTree {
private :
	size_t M;  //max number of features in the leaf, max number of siblings in each node
	size_t m;  //m equals M/argM, the minimum number in a node except root
	RNode* root;
	int currentObject;
	void generateActiveBrachList(double x, double y, RNode* rnode, ABL& list);
	void pickNextFeature(RNode* rnode, RNode* ptrnew1, RNode* ptrnew2);
	void pickSeedsFeature(RNode* rnode, Feature& f1, Feature& f2);
	void pickNextNode(RNode* rnode, RNode* ptrnew1, RNode* ptrnew2);
	void pickSeedsNode(RNode* rnode, RNode*& p1, RNode*& p2);
	double CalculateMINDIST(double x, double y, const Envelope& rect);
public:
	RTree() :root(NULL), M(3), currentObject(0) { m = M / argM; }
	RTree(size_t M) :M(M), root(NULL), currentObject(0) { m = M / argM; }
	void setConfig(size_t M) { this->M = M; m = M / argM; }
	int getMaxLeafNum()const { return M; }
	int getMaxNodeNum()const { return M; }
	const Envelope& getEnvelope() const { return root->getEnvelope(); }
	bool constructRTree(vector<Feature>& features);
	void insert(Feature& feature);
	RNode* chooseLeaf(Feature& feature);   //find the leaf to insert feature and insert
	//quadraticSplit the current node
	RNode* quadraticSplit(RNode* rnode);
	//quadraticSplit the parent
	RNode* adjustParent(RNode* rnode);
	void draw();
	void rangeQuery(Envelope& rect, vector<Feature>& features);
	bool NNQuery(double x, double y, vector<Feature>& features);
	//pick the next feature from rnode and insert it into 
	
	void INN_Search(double x, double y,RNode* rnode, NNTemp &NN_temp);
	int getHeight()
	{
		return root->calculateLevel();
	}
};


}
#endif