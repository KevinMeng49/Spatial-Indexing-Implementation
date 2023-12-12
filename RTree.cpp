#include "RTree.h"
namespace hw6 {

/*
R-node operations
*/
void RNode::draw()
{
	if (this->getChildNum() == 0)  //is leaf node
		bbox.draw();
	else
	{
		bbox.draw();
		for (int i = 0; i < nodes.size(); i++)
			nodes[i]->draw();
	}
}
void RNode::rangeQuery(Envelope& rect, vector<Feature>& features)
{
	//当包围盒与选区不相交则直接返回
	if (!bbox.intersect(rect))
		return;
	//当选区完全包含包围盒，则将所有的要素都存入列表
	/*if (rect.contain(bbox))
	{
		for (int i = 0; i < this->features.size(); i++)
			features.push_back(this->features[i]);
		return;
	}*/
	//当当前节点无法再分时单独判断所有几何体包围盒是否和选区相交
	if (this->nodes.size()==0)
	{
		for (int i = 0; i < this->features.size(); i++)
		{
			if (this->features[i].getEnvelope().intersect(rect))
				features.push_back(this->features[i]);
		}
		return;
	}
	else //当可以再细分且不完全被选区包围时
	{
		for (int i = 0; i < this->nodes.size(); i++)
		{
			nodes[i]->rangeQuery(rect, features);
		}
		return;
	}
	// Task range query
	// Write your code here

}
RNode* RNode::pointInLeafNode(double x, double y)
{

	//如果在该节点范围内且该没有叶节点
	if (x >= bbox.getMinX() && x <= bbox.getMaxX() && y >= bbox.getMinY() && y <= bbox.getMaxY() && nodes.size() == 0)
		return this;
	//如果在该节点范围内，该节点具有叶节点
	else if (x >= bbox.getMinX() && x <= bbox.getMaxX() && y >= bbox.getMinY() && y <= bbox.getMaxY() && nodes.size()!=0)
	{
		int flag = 0;
		for (size_t i = 0; i < nodes.size(); i++) 
		{
			Envelope box = nodes[i]->getEnvelope();
			if (x >= box.getMinX() && x <= box.getMaxX() && y >= box.getMinY() && y <= box.getMaxY()) 
			{
				flag = 1;
				break;
			}
		}
		if (flag == 0)
			return this;

			

		for (size_t i = 0; i < nodes.size(); i++)
		{
			if (nodes[i]->pointInLeafNode(x, y) != NULL) 
			{
				return nodes[i]->pointInLeafNode(x, y);
			}
		}
	}
	else
		return NULL;
	//this is ok

}
/*
R-tree operations
*/
void RTree::insert(Feature& feature)
{
	//if already exists, do not insert
	/*if (exists(feature))
		return NULL;*/
	currentObject++;
	// if the tree is empty
	if (root == NULL)
	{
		root = new RNode(feature.getEnvelope(), M);
		root->add(feature);
		return;
	}
	//if the root is not NULL
	RNode* p=chooseLeaf(feature);  //p is the leafnode that already has inserted feature
	if (p->getFeatureNum() > M)
		root = quadraticSplit(p);
	return;

}

RNode* RTree::chooseLeaf(Feature& feature)
{
	
	//if root is the leaf
	if (root->getChildNum() == 0)
	{
		root->add(feature);
		return root;
	}
		
    Envelope e = feature.getEnvelope();
	
	//if root is not leaf, find the node whose MRB expands the least
	RNode* ptr = root;
	while (ptr->getChildNum()!=0) {  
		Envelope env = ptr->getEnvelope();
		double delta = env.getArea();
		int index=0;
		//find the node whose MRB expands the least
		for (int i = 0; i < ptr->getChildNum(); i++)
		{
			RNode* child = ptr->getChildNode(i);
			double change = child->getEnvelope().unionEnvelope(e).getArea() - child->getEnvelope().getArea();
			if (change < delta)
			{
				delta = change;
				index = i;
			}
			else if (fabs(change - delta)<0.0000000005)
			{
				//如果面积增量相同，选择面积小的包围盒插入
				//TO-DO
				if (ptr->getChildNode(index)->getEnvelope().getArea() > child->getEnvelope().getArea())
					index = i;
			}
		}
		//expand the current node
		ptr->setEnvelope(ptr->getEnvelope().unionEnvelope(e));
		//move to the next node
		ptr = ptr->getChildNode(index);
	}
	ptr->add(feature);
	return ptr;
}

void RTree::pickSeedsFeature(RNode* rnode, Feature& f1, Feature& f2)
{
	double maxcost = 0;
	//choose two rectangle that has the max distance as the seeds
	for (int i = 0; i < rnode->getFeatureNum() - 1; i++)
	{
		Feature temp = rnode->getFeature(i);
		for (int j = i + 1; j < rnode->getFeatureNum(); j++)
		{
			Feature temp2 = rnode->getFeature(j);
			//choose the most wasteful pair
			double cost = temp.getEnvelope().unionEnvelope(temp2.getEnvelope()).getArea() - temp.getEnvelope().getArea() - temp2.getEnvelope().getArea();
			if (cost >= maxcost)
			{
				maxcost = cost;
				f1 = temp;
				f2 = temp2;
			}
		}
	}
}

//rnode is the node to split
void RTree::pickNextFeature(RNode* rnode, RNode* ptrnew1,RNode* ptrnew2)
{
	Feature f=rnode->getFeature(0);
	double cost = 0;
	double dis1=0, dis2=0;

	for (int i = 0; i < rnode->getFeatureNum(); i++)
	{
		double d1, d2;
		double difference;
		Feature temp = rnode->getFeature(i);
		d1 = temp.getEnvelope().unionEnvelope(ptrnew1->getEnvelope()).getArea() - temp.getEnvelope().getArea();
		d2= temp.getEnvelope().unionEnvelope(ptrnew2->getEnvelope()).getArea() - temp.getEnvelope().getArea();
		if ((difference=fabs(d2 - d1)) > cost)
		{
			cost = difference;
			dis1 = d1;
			dis2 = d2;
			f = temp;
		}
	}
	if (dis1 < dis2)
		ptrnew1->add(f);
	else
		ptrnew2->add(f);
	rnode->deleteFeature(f);
	return;

}
RNode* RTree::quadraticSplit(RNode* rnode)
{
	//if this is a leaf, Quadratic Split
	if (rnode->getChildNum() == 0)
	{
		Feature f1, f2;
		//choose the pair that would waste the most area if both were put in the same group
		pickSeedsFeature(rnode, f1, f2);

		rnode->deleteFeature(f1);
		rnode->deleteFeature(f2);
		//align other features to the nearest rectangle
		RNode* ptrnew1 = new RNode(f1, M);
		RNode* ptrnew2 = new RNode(f2, M);
		while (rnode->getFeatureNum() > 0)
		{
			pickNextFeature(rnode, ptrnew1, ptrnew2);
			//The total is M+1, and each should have at least m objects
			if ((ptrnew1->getFeatureNum() >= M - m) || (ptrnew2->getFeatureNum() >= M - m))
				break;
		}

		//make sure that the quadraticSplit is fair for each new node
		if (ptrnew1->getFeatureNum() >= M - m)
		{
			for (int i = rnode->getFeatureNum() - 1; i >= 0; i--)
			{
				Feature &f = rnode->getFeature(i);
				rnode->pop_backFeature();//delete this feature
				ptrnew2->add(f);
				
			}
		}
		else if (ptrnew2->getFeatureNum() >= M-m)
		{
			for (int i = rnode->getFeatureNum() - 1; i >= 0; i--)
			{
				Feature &f = rnode->getFeature(i);
				rnode->pop_backFeature();//delete this feature
				ptrnew1->add(f);
				
			}
		}
		//two new nodes has been created,insert them to parent
		//if the leaf is also root
		if (rnode->getParent() == NULL)
		{
			RNode* r = new RNode(ptrnew1->getEnvelope().unionEnvelope(ptrnew2->getEnvelope()), M);
			r->add(ptrnew1);
			r->add(ptrnew2);
			delete rnode;
			return r;
		}
		else
		{
			RNode* parent = rnode->getParent();
			//delete the original node
			parent->deleteNode(rnode);
			delete rnode;
			//replace by new node
			parent->add(ptrnew1);
			parent->add(ptrnew2);
			//if overflow, quadraticSplit parent
			if (parent->getChildNum() > M)
				return adjustParent(parent);
			//if not, finish
			else
				return root;	
		}
	}
	
}


void RTree::pickNextNode(RNode* rnode, RNode* ptrnew1, RNode* ptrnew2)
{
	RNode* f=rnode->getChildNode(0);
	double cost = 0;
	double dis1=0, dis2=0;
	for (int i = 0; i < rnode->getChildNum(); i++)
	{
		double d1, d2;
		double difference;
		RNode* temp = rnode->getChildNode(i);
		d1 = temp->getEnvelope().unionEnvelope(ptrnew1->getEnvelope()).getArea() - temp->getEnvelope().getArea();
		d2 = temp->getEnvelope().unionEnvelope(ptrnew2->getEnvelope()).getArea() - temp->getEnvelope().getArea();
		if ((difference = fabs(d2 - d1)) > cost)
		{
			cost = difference;
			dis1 = d1;
			dis2 = d2;
			f = temp;
		}
	}
	if (dis1 < dis2)
		ptrnew1->add(f);
	else
		ptrnew2->add(f);
	rnode->deleteNode(f);
	return;
}
void RTree::pickSeedsNode(RNode* rnode, RNode*& p1, RNode*& p2)
{
	double maxdis = 0;
	//choose two rectangle that has the max distance as the seeds
	for (int i = 0; i < rnode->getChildNum() - 1; i++)
	{
		RNode* temp = rnode->getChildNode(i);
		for (int j = i + 1; j < rnode->getChildNum(); j++)
		{
			RNode* temp2 = rnode->getChildNode(j);
			double dis = temp->getEnvelope().unionEnvelope(temp2->getEnvelope()).getArea()-temp->getEnvelope().getArea()-temp2->getEnvelope().getArea();
			if (dis >= maxdis)
			{
				maxdis = dis;
				p1 = temp;
				p2 = temp2;
			}
		}
	}
}

RNode* RTree::adjustParent(RNode* rnode)
{
	//if this is a non-leaf node, use the same quadric method,but the object is node
	if(rnode->getChildNum()>M)
	{
		RNode* p1, * p2;
		pickSeedsNode(rnode, p1, p2);
		//remove the selected two node
		rnode->deleteNode(p1);
		rnode->deleteNode(p2);

		//align other features to the nearest rectangle
		RNode* ptrnew1 = new RNode(p1, M);
		RNode* ptrnew2 = new RNode(p2, M);
		while (rnode->getChildNum() != 0)
		{
			pickNextNode(rnode, ptrnew1, ptrnew2);
			//The total is M+1, and each should have at least m objects
			if ((ptrnew1->getChildNum() >= M - m) || (ptrnew2->getChildNum() >= M - m))
				break;
		}
		//make sure that the quadraticSplit is fair for each new node
		//ptr 1 is ok, add to ptr2
		if (ptrnew1->getChildNum() >= M - m)
		{
			for (int i = rnode->getChildNum() - 1; i >= 0; i--)
			{
				RNode* p = rnode->getChildNode(i);
				rnode->pop_backNode();//delete this node
				ptrnew2->add(p);

			}
		}
		//ptr2 is ok, add to ptr1
		else if (ptrnew2->getChildNum() >= M - m)
		{
			for (int i = rnode->getChildNum() - 1; i >= 0; i--)
			{
				RNode* p = rnode->getChildNode(i);
				rnode->pop_backNode();//delete this node
				ptrnew1->add(p);

			}
		}
		//two new nodes has been created,insert them to parent
		//if the current node is root, create a new root
		if (rnode->getParent() == NULL)
		{
			RNode* r = new RNode(ptrnew1->getEnvelope().unionEnvelope(ptrnew2->getEnvelope()), M);
			r->add(ptrnew1);
			r->add(ptrnew2);
			delete rnode;
			return r;
		}

		else
		{
			RNode* parent = rnode->getParent();
			parent->deleteNode(rnode);
			parent->add(ptrnew1);
			parent->add(ptrnew2);
			if (parent->getChildNum() > M)
				return adjustParent(parent);
			//if not, finish
			else
				return root;
		}
	}
}



bool RTree::constructRTree(vector<Feature>& features)
{
	cout << "R-tree" << endl;
	for (int i = 0; i < features.size(); i++)
	{
		if (i == 11)
			int a = 0;
		insert(features[i]);
	}
	return true;
}

void RTree::draw()
{
	if (root)
		root->draw();

}

void RTree::rangeQuery(Envelope& rect, vector<Feature>& features)
{
	features.clear();

	// Task range query
	// Write your code here
	root->rangeQuery(rect, features);
	// filter step (选择查询区域与几何对象包围盒相交的几何对象)
	// 注意四叉树区域查询仅返回候选集，精炼步在hw6的rangeQuery中完成
}
bool RTree::NNQuery(double x, double y, vector<Feature>& features) 
{
	if (!root || !(root->getEnvelope().contain(x, y)))
		return false;
	features.clear();
	
	NNTemp nntemp;
	nntemp.nearestDist = 66666;
	INN_Search(x, y, root, nntemp);
	features.push_back(nntemp.f);
	return true;
}


void RTree::INN_Search(double x, double y, RNode* rnode, NNTemp& NN_temp)
{
	/*the current node is at leaf level*/
	if (rnode->getChildNum() == 0)
	{
		for (int i = 0; i < rnode->getFeatureNum(); i++)
		{
			Feature& f = rnode->getFeature(i);
			double dist = f.distance(x, y);
			dist = dist * dist;
			if (dist < NN_temp.nearestDist)
			{
				NN_temp.nearestDist = dist;
				NN_temp.f = f;
			}
		}
	}
	/*not leaf node*/
	else
	{
		ABL list;
		generateActiveBrachList(x, y, rnode, list);//generate and calculate mindist
		sort(begin(list.ABList), end(list.ABList), [](const ABN& p1, const ABN& p2) {return p1.MINDIST < p2.MINDIST; });  //sort by mindist ascending
		for (int i = 0; i < list.ABList.size(); i++)
		{
			if (list.ABList[i].MINDIST < NN_temp.nearestDist)
				
				INN_Search(x, y, list.ABList[i].node, NN_temp);
		}

	}
}
double RTree::CalculateMINDIST(double x, double y, const Envelope& rect)
{
	double mindist = 0;
	double r;
	if (x < rect.getMinX())
		r = rect.getMinX();
	else if (x > rect.getMaxX())
		r = rect.getMaxX();
	else
		r = x;
	mindist += (x - r) * (x - r);

	if (y < rect.getMinY())
		r = rect.getMinY();
	else if (y > rect.getMaxY())
		r = rect.getMaxY();
	else
		r = y;
	mindist += (y - r) * (y - r);

	return mindist;
}

void RTree::generateActiveBrachList(double x, double y, RNode* rnode, ABL& list)
{
	double mindist;
	for (int i = 0; i < rnode->getChildNum(); i++)
	{
		RNode* p = rnode->getChildNode(i);
		mindist = CalculateMINDIST(x, y, p->getEnvelope());
		ABN temp;
		temp.MINDIST = mindist;
		temp.node = p;
		list.ABList.push_back(temp);
	}

}

}


