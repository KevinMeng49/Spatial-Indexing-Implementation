#ifndef RSTAR_H_INCLUDED
#define RSTAR_H_INCLUDED

#include "Geometry.h"
#include"QuadTree.h"   //Feature 



const double percent = 0.4;

namespace hw6 {

class RSNode {
private:
	size_t M;  //max number of features in the leaf, nonleaf nodes (except the root) have between M/2 and M children
	size_t m;  //m equals M/argM, the minimum number in a node except root
	Envelope bbox;
	RSNode* parent;
	int visited;
	vector<Feature> features;  //limt is M
	vector<RSNode*> nodes;     //limit is M
	RSNode() {}
	
public:
	
	RSNode(Envelope box, size_t M) :M(M) {
		bbox = box;
		parent = NULL;
		m = M * percent;
		visited = 0;
	}

	RSNode(Feature& f, size_t M) :M(M), parent(NULL) {
		features.push_back(f);
		bbox = f.getEnvelope();
		m = M *percent;
		visited = 0;
	}

	RSNode(RSNode* son, size_t M) :M(M), parent(NULL) {
		nodes.push_back(son);
		son->setParent(this);
		bbox = son->getEnvelope();
		m = M *percent;
		visited = 0;
	}
	~RSNode() {
		for (int i = 0; i < nodes.size(); i++) {
			delete nodes[i];
			nodes[i] = NULL;
		}
	}
	const Envelope& getEnvelope() { return bbox; }
	RSNode* getChildNode(size_t i) { return nodes[i]; }

	size_t    getFeatureNum() { return features.size(); }
	size_t  getChildNum() { return nodes.size(); }
	Feature& getFeature(size_t i) {
		return features[i];
	}  
	vector<Feature>& getAllFeature() { return features; }
	vector<RSNode*>& getAllNodes() { return nodes; }
	int isFirstOverflow() { return !visited; }
	void notFirst() { visited = 1; }
	//add feature and update bbox
	void add(Feature& f)
	{

		features.push_back(f);
		//update envelope
		bbox = bbox.unionEnvelope(f.getEnvelope());
	}
	//add node and update bbox, parent
	void add(RSNode* node)
	{

		nodes.push_back(node);
		bbox = bbox.unionEnvelope(node->getEnvelope());
		node->parent = this;
	}

	void setParent(RSNode* node)
	{
		parent = node;
	}
	void setEnvelope(Envelope env) { bbox = env; }
	void pop_backFeature() { features.pop_back(); }
	void pop_backNode() { nodes.pop_back(); }
	RSNode* getParent() { return parent; }
	void draw();
	void updateEnvelope();

	//delete certain node
	void deleteNode(RSNode* p)
	{
		vector<RSNode*>::iterator it;
		it = find(nodes.begin(), nodes.end(), p);
		if (it != nodes.end())
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
	int calculateLevel()
	{
		int level = 0;
		RSNode* ptr = this;
		while (ptr->getChildNum() != 0)
		{
			level++;
			ptr = ptr->getChildNode(0);
		}
		return level;
	}

};

class NNTempS
{
public:
	Feature f;
	double nearestDist;
};


class ABNS {
public:
	RSNode* rsnode;
	double MINDIST;
};


class ABLS
{
public:
	vector<ABNS> ABList;
};

class RStar
{
private:
	size_t M;  //max number of features in the leaf, max number of siblings in each node
	size_t m;  //m equals M/argM, the minimum number in a node except root
	RSNode* root;
	int currentObject;

	void generateActiveBrachList(double x, double y, RSNode* rnode, ABLS& list);
	double CalculateMINDIST(double x, double y, const Envelope& rect);

public:
	RStar():root(NULL), M(3), currentObject(0) { m = (int)M *percent; }
	RStar(size_t M) :M(M), root(NULL), currentObject(0) { m = M *percent; }
	void setConfig(size_t M) { this->M = M; m = M * percent; }
	int getMaxLeafNum()const { return M; }
	int getMaxNodeNum()const { return M; }
	const Envelope& getEnvelope() const { return root->getEnvelope(); }
	bool constructRStar(vector<Feature>& features);
	RSNode* chooseSubtree(const Envelope& env,int level);
	void Split(RSNode* rnode);
	int chooseSplitAxis(RSNode*rnode);
	int chooseSplitIndex(RSNode* rnode, int axis);
	void insert(Feature& f);
	void insert(RSNode* rnode);
	void OverflowTreatment(RSNode* rnode); //this is a propagate upwards function
	void reInsert(RSNode* rnode);
	void updateEnvlope(RSNode* rnode);     //update from a certain level to the root
	void draw();

	bool NNQuery(double x, double y, vector<Feature>& features);
	//pick the next feature from rnode and insert it into 

	void INN_Search(double x, double y, RSNode* rnode, NNTempS& NN_temp);
	int getHeight()
	{
		return root->calculateLevel();
	}
};
}



#endif // !RSTAR_H_INCLUDED