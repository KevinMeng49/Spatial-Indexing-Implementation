#include <set>
#include "QuadTree.h"

namespace hw6 {

/*
 * QuadNode
 */
void QuadNode::split(size_t capacity)
{
	for (int i = 0; i < 4; ++i) {
		delete nodes[i];
		nodes[i] = NULL;
	}

	// Task construction
	// Write your code here
	double midx = (this->bbox.getMinX() + this->bbox.getMaxX()) / 2;
	double midy = (this->bbox.getMinY() + this->bbox.getMaxY()) / 2;
 	nodes[0] = new QuadNode(Envelope(bbox.getMinX(), midx, midy, bbox.getMaxY())); //左上
	nodes[1] = new QuadNode(Envelope(midx, bbox.getMaxX(), midy, bbox.getMaxY())); //右上
	nodes[2] = new QuadNode(Envelope(midx, bbox.getMaxX(), bbox.getMinY(), midy)); //右下
	nodes[3] = new QuadNode(Envelope(bbox.getMinX(), midx, bbox.getMinY(), midy)); //左下
	for (int i = 0; i < features.size(); i++)
	{
		Feature& t = features[i];       //这里先使用引用，每个结点的features都是引用的之前的，没有额外分配空间
		for (int j = 0; j < 4; j++)
		{
			if (nodes[j]->bbox.intersect(t.getEnvelope()))
			{
				nodes[j]->add(t);	
			}
				
		}
	}
	for (int i = 0; i < 4; i++)
	{
		if (nodes[i]->getFeatureNum() > capacity)
			nodes[i]->split(capacity);     //递归分解
	}
}

void QuadNode::countNode(int& interiorNum, int& leafNum)
{
	if (isLeafNode()) {
		++leafNum;
	}
	else {
		++interiorNum;
		for (int i = 0; i < 4; ++i)
			nodes[i]->countNode(interiorNum, leafNum);
	}
}

int QuadNode::countHeight(int height)
{
	++height;
	if (!isLeafNode()) {
		int cur = height;
		for (int i = 0; i < 4; ++i) {
			height = max(height, nodes[i]->countHeight(cur));
		}
	}
	return height;
}

void QuadNode::rangeQuery(Envelope& rect, vector<Feature>& features)
{
	//当包围盒与选区不相交则直接返回
	if (!bbox.intersect(rect))
		return;
	//当选区完全包含包围盒，则将所有的要素都存入列表
	if (rect.contain(bbox))
	{
		for (int i = 0; i < this->features.size(); i++)
			features.push_back(this->features[i]);
		return;
	}
    //当当前节点无法再分时单独判断所有几何体包围盒是否和选区相交
	if (nodes[0] == NULL)
	{
		for (int i = 0; i < this->features.size(); i++)
		{
			if(this->features[i].getEnvelope().intersect(rect))
				features.push_back(this->features[i]);
		}
		return;
	}
	else //当可以再细分且不完全被选区包围时
	{
		nodes[0]->rangeQuery(rect, features);
		nodes[1]->rangeQuery(rect, features);
		nodes[2]->rangeQuery(rect, features);
		nodes[3]->rangeQuery(rect, features);
		return;
	}
	// Task range query
	// Write your code here

}

QuadNode* QuadNode::pointInLeafNode(double x, double y)
{
	// Task NN query
	// Write your code here
	//如果在该节点范围内且该没有叶节点
	QuadNode* q=NULL;
	if (nodes[0] == NULL&&x >= bbox.getMinX() && x <= bbox.getMaxX() && y >= bbox.getMinY() && y <= bbox.getMaxY() )
		return this;
	//如果在该节点范围内，该节点具有叶节点
	else if (nodes[0] != NULL && x >= bbox.getMinX() && x <= bbox.getMaxX() && y >= bbox.getMinY() && y <= bbox.getMaxY())
	{
		if (nodes[0] != NULL && (q=nodes[0]->pointInLeafNode(x, y))!= NULL)
			return q;
		if (nodes[1] != NULL && (q=nodes[1]->pointInLeafNode(x, y)) != NULL)
			return q;
		if (nodes[2] != NULL && (q=nodes[2]->pointInLeafNode(x, y)) != NULL)
			return q;
		if (nodes[3] != NULL && (q=nodes[3]->pointInLeafNode(x, y)) != NULL)
			return q;
	}
	else
		return NULL;
}

void QuadNode::draw()
{
	if (isLeafNode()) {
		bbox.draw();
	}
	else {
		for (int i = 0; i < 4; ++i)
			nodes[i]->draw();
	}
}

/*
 * QuadTree
 */
bool QuadTree::constructQuadTree(vector<Feature>& features)
{
	if (features.empty())
		return false;

	// Task construction
	// Write your code here
	bbox = features[0].getEnvelope();
	for (int i = 1; i < features.size(); i++)
	{
		bbox = bbox.unionEnvelope(features[i].getEnvelope());
	}

	root = new QuadNode(bbox);
	root->add(features);
	if (root->getFeatureNum() > capacity)
		root->split(capacity);
	

	return true;
}

void QuadTree::countQuadNode(int& interiorNum, int& leafNum)
{
	interiorNum = 0;
	leafNum = 0;
	if (root)
		root->countNode(interiorNum, leafNum);
}

void QuadTree::countHeight(int &height)
{
	height = 0;
	if (root)
		height = root->countHeight(0);
}

void QuadTree::rangeQuery(Envelope& rect, vector<Feature>& features) 
{ 
	features.clear();

	// Task range query
	// Write your code here
	root->rangeQuery(rect,features);
	// filter step (选择查询区域与几何对象包围盒相交的几何对象)
	// 注意四叉树区域查询仅返回候选集，精炼步在hw6的rangeQuery中完成
}

bool QuadTree::NNQuery(double x, double y, vector<Feature>& features)
{
	if (!root || !(root->getEnvelope().contain(x, y)))
		return false;
	features.clear();
	// Task NN query
	// Write your code here
	QuadNode* LeafNode = root->pointInLeafNode(x, y);
	// filter step (使用maxDistance2Envelope函数，获得查询点到几何对象包围盒的最短的最大距离，然后区域查询获得候选集)
	const Envelope& envelope = root->getEnvelope();
	double minDist = max(envelope.getWidth(), envelope.getHeight());  //找一个最大的做参考
	double dist;
	for (size_t i = 0; i < LeafNode->getFeatureNum(); i++) 
	{
		dist = LeafNode -> getFeature(i).maxDistance2Envelope(x, y);
		if (dist < minDist)
			minDist = dist;
	}

	Envelope rect = Envelope(x - minDist, x + minDist, y - minDist,  y + minDist);

	root->rangeQuery(rect, features);


	

	// 注意四叉树邻近查询仅返回候选集，精炼步在hw6的NNQuery中完成

	return true;
}

void QuadTree::draw()
{
	if (root)
		root->draw();
}

}
