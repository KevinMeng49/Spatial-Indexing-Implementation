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
 	nodes[0] = new QuadNode(Envelope(bbox.getMinX(), midx, midy, bbox.getMaxY())); //����
	nodes[1] = new QuadNode(Envelope(midx, bbox.getMaxX(), midy, bbox.getMaxY())); //����
	nodes[2] = new QuadNode(Envelope(midx, bbox.getMaxX(), bbox.getMinY(), midy)); //����
	nodes[3] = new QuadNode(Envelope(bbox.getMinX(), midx, bbox.getMinY(), midy)); //����
	for (int i = 0; i < features.size(); i++)
	{
		Feature& t = features[i];       //������ʹ�����ã�ÿ������features�������õ�֮ǰ�ģ�û�ж������ռ�
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
			nodes[i]->split(capacity);     //�ݹ�ֽ�
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
	//����Χ����ѡ�����ཻ��ֱ�ӷ���
	if (!bbox.intersect(rect))
		return;
	//��ѡ����ȫ������Χ�У������е�Ҫ�ض������б�
	if (rect.contain(bbox))
	{
		for (int i = 0; i < this->features.size(); i++)
			features.push_back(this->features[i]);
		return;
	}
    //����ǰ�ڵ��޷��ٷ�ʱ�����ж����м������Χ���Ƿ��ѡ���ཻ
	if (nodes[0] == NULL)
	{
		for (int i = 0; i < this->features.size(); i++)
		{
			if(this->features[i].getEnvelope().intersect(rect))
				features.push_back(this->features[i]);
		}
		return;
	}
	else //��������ϸ���Ҳ���ȫ��ѡ����Χʱ
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
	//����ڸýڵ㷶Χ���Ҹ�û��Ҷ�ڵ�
	QuadNode* q=NULL;
	if (nodes[0] == NULL&&x >= bbox.getMinX() && x <= bbox.getMaxX() && y >= bbox.getMinY() && y <= bbox.getMaxY() )
		return this;
	//����ڸýڵ㷶Χ�ڣ��ýڵ����Ҷ�ڵ�
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
	// filter step (ѡ���ѯ�����뼸�ζ����Χ���ཻ�ļ��ζ���)
	// ע���Ĳ��������ѯ�����غ�ѡ������������hw6��rangeQuery�����
}

bool QuadTree::NNQuery(double x, double y, vector<Feature>& features)
{
	if (!root || !(root->getEnvelope().contain(x, y)))
		return false;
	features.clear();
	// Task NN query
	// Write your code here
	QuadNode* LeafNode = root->pointInLeafNode(x, y);
	// filter step (ʹ��maxDistance2Envelope��������ò�ѯ�㵽���ζ����Χ�е���̵������룬Ȼ�������ѯ��ú�ѡ��)
	const Envelope& envelope = root->getEnvelope();
	double minDist = max(envelope.getWidth(), envelope.getHeight());  //��һ���������ο�
	double dist;
	for (size_t i = 0; i < LeafNode->getFeatureNum(); i++) 
	{
		dist = LeafNode -> getFeature(i).maxDistance2Envelope(x, y);
		if (dist < minDist)
			minDist = dist;
	}

	Envelope rect = Envelope(x - minDist, x + minDist, y - minDist,  y + minDist);

	root->rangeQuery(rect, features);


	

	// ע���Ĳ����ڽ���ѯ�����غ�ѡ������������hw6��NNQuery�����

	return true;
}

void QuadTree::draw()
{
	if (root)
		root->draw();
}

}
