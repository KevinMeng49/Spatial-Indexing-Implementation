#include "RStar.h"

namespace hw6 {
void RSNode::draw()
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
void RSNode::updateEnvelope()
{
	//if leaf
	if (nodes.size() == 0)
	{
		bbox = features[0].getEnvelope();
		for (int i = 0; i < features.size(); i++)
		{
			bbox = bbox.unionEnvelope(features[i].getEnvelope());
		}
	}
	//if nonleaf
	else
	{
		bbox = nodes[0]->getEnvelope();
		for (int i = 0; i < nodes.size(); i++)
		{
			bbox = bbox.unionEnvelope(nodes[i]->getEnvelope());
		}
	}
}


void RStar::draw()
{
	if (root)
		root->draw();

}

void RStar::updateEnvlope(RSNode* rnode)
{
	rnode->updateEnvelope();
	if (rnode->getParent() != NULL)
		updateEnvlope(rnode->getParent());
}


RSNode* RStar::chooseSubtree(const Envelope& env,int level)
{
	RSNode* ptr = root;
	/*ptr is the leaf*/
	if (ptr->getChildNum() == 0)
		return ptr;
	/*if the child pointers do not point to leaves*/
	while(ptr->getChildNode(0)->calculateLevel()!=level)
	{
		double delta = ptr->getEnvelope().getArea();
		int index = 0;
		//find the node whose MRB expands the least
		for (int i = 0; i < ptr->getChildNum(); i++)
		{
			RSNode* child = ptr->getChildNode(i);
			double change = child->getEnvelope().unionEnvelope(env).getArea() - child->getEnvelope().getArea();
			if (change < delta)
			{
				delta = change;
				index = i;
			}
			else if (fabs(change - delta) < 0.0000000005)
			{
				//resolve ties by choosing the entry with the rectangle of smallest area
				//TO-DO
				if (ptr->getChildNode(index)->getEnvelope().getArea() > child->getEnvelope().getArea())
					index = i;
			}
		}
		//do not insert the current bounding box
		//move to the next node
		ptr = ptr->getChildNode(index);
	}
	//if (level != 0)
	//	ptr = ptr->getParent();//the ptr points to "leaf"
	//TODO when the childpointers in ptr point to leaves
	//sort by area enlargement increasing order
	struct enlargement
	{
		int index;
		double enlargement;
		double overlap;
	};
	vector<struct enlargement>indexArray;
	for (int i = 0; i < ptr->getChildNum(); i++)
	{
		RSNode* temp1 = ptr->getChildNode(i);
		struct enlargement e;
		e.index = i;
		e.enlargement = temp1->getEnvelope().unionEnvelope(env).getArea() - temp1->getEnvelope().getArea();
		indexArray.push_back(e);
	}
	sort(begin(indexArray), end(indexArray), [](struct enlargement& e1, struct enlargement& e2) {return e1.enlargement < e2.enlargement; });
	//Let A be the group of the first p entries
	int p = indexArray.size() > 32 ? 32 : indexArray.size();
	//choose the least overlap 
	for (int i = 0; i < p; i++)
	{
		double overlap = 0;
		//after union feature;s bbox
		Envelope ek = ptr->getChildNode(indexArray[i].index)->getEnvelope().unionEnvelope(env);
		for (int j = 0; j < p; j++)
		{
			if (j == i)
				continue;
			Envelope ei= ptr->getChildNode(indexArray[j].index)->getEnvelope();
			overlap += ek.intersection(ei).getArea();
		}
		indexArray[i].overlap = overlap;
	}
	//sort by overlap
	sort(begin(indexArray), begin(indexArray) + p, 
		[](struct enlargement& e1, struct enlargement& e2) {
		if (e1.overlap < e2.overlap)return true;
		if (fabs(e1.overlap - e2.overlap) < 0.0000000005) return (e1.enlargement < e2.enlargement);
		else return false; });
	ptr = ptr->getChildNode(indexArray[0].index);
	return ptr;
}

/*Split of the R*-tree, insert the newnode into the parent*/
void RStar::Split(RSNode* rnode)
{
	int axis = chooseSplitAxis(rnode);
	int index = chooseSplitIndex(rnode, axis);
	/*split*/
	
	/*if this is leaf*/
	if (rnode->getChildNum() == 0)
	{
		RSNode* newNode = new RSNode(rnode->getFeature(M), M);
		rnode->pop_backFeature();
		for (int i = M-1; i >= index; i--)
		{
			newNode->add(rnode->getFeature(i));
			rnode->pop_backFeature();
		}
		//split only affect the bounding box of the current node, not parent
		rnode->updateEnvelope();
		if(rnode->getParent()!=NULL)
			rnode->getParent()->add(newNode);
		else
		{
			RSNode* newRoot = new RSNode(rnode, M);
			newRoot->add(newNode);
			root = newRoot;
		}
	}
	/*if non leaf*/
	else
	{
		RSNode* newNode = new RSNode(rnode->getChildNode(M), M);
		rnode->pop_backNode();
		for (int i = M - 1; i >= index; i--)
		{
			newNode->add(rnode->getChildNode(i));
			rnode->pop_backNode();
		}
		rnode->updateEnvelope();
		
		if (rnode->getParent() != NULL)
			rnode->getParent()->add(newNode);
		else
		{
			RSNode* newRoot = new RSNode(rnode, M);
			newRoot->add(newNode);
			root = newRoot;
		}
	}
}

//return 1 if choose x, return 2 if choose y
int RStar::chooseSplitAxis(RSNode*rnode)
{
	/*if this is leaf node*/
	if (rnode->getChildNum() == 0)
	{
		vector<Feature> &list = rnode->getAllFeature();
		/*axis x*/
		sort(begin(list), end(list), [](Feature& f1, Feature& f2) {
			if (f1.getEnvelope().getMinX() < f2.getEnvelope().getMinX()) return true;
			if (fabs(f1.getEnvelope().getMinX() - f2.getEnvelope().getMinX()) < 0.00000000005) return (f1.getEnvelope().getMaxX() < f2.getEnvelope().getMaxX());
			else return false;
		});
		/*sort(begin(list), end(list), [](Feature& f1, Feature& f2) {
			return f1.getEnvelope().getMinX() < f2.getEnvelope().getMinX();
		});
		sort(begin(list), end(list), [](Feature& f1, Feature& f2) {
			return f1.getEnvelope().getMaxX() < f2.getEnvelope().getMaxX();
		});*/
		double S1 = 0;//the margin value
		for (int k = m; k <= M + 1 - m; k++)
		{
			/*group 1 from 0 to k-1*/
			Envelope e1=list[0].getEnvelope();
			double sumArea = 0;
			for (int i = 1; i < k; i++)
			{
				sumArea += list[i].getEnvelope().getArea();
				e1 = e1.unionEnvelope(list[i].getEnvelope());
			}
			S1 += e1.getArea() - sumArea;
			/*group 2 from k to M+1 -1*/
			e1 = list[k].getEnvelope();
			sumArea = 0;
			for (int i = k + 1; i < M + 1; i++)
			{
				sumArea += list[i].getEnvelope().getArea();
				e1 = e1.unionEnvelope(list[i].getEnvelope());
			}
			S1 += e1.getArea() - sumArea;
		}

		/*axis y*/
		sort(begin(list), end(list), [](Feature& f1, Feature& f2) {
			if (f1.getEnvelope().getMinY() < f2.getEnvelope().getMinY()) return true;
			if (fabs(f1.getEnvelope().getMinY() - f2.getEnvelope().getMinY()) < 0.00000000005) return (f1.getEnvelope().getMaxY() < f2.getEnvelope().getMaxY());
			else return false;
		});
		/*sort(begin(list), end(list), [](Feature& f1, Feature& f2) {
			return f1.getEnvelope().getMinY() < f2.getEnvelope().getMinY();
		});
		sort(begin(list), end(list), [](Feature& f1, Feature& f2) {
			return f1.getEnvelope().getMaxY() < f2.getEnvelope().getMaxY();
		});*/
		double S2 = 0;
		for (int k = m; k <= M + 1 - m; k++)
		{
			/*group 1 from 0 to k-1*/
			Envelope e1 = list[0].getEnvelope();
			double sumArea = 0;
			for (int i = 1; i < k; i++)
			{
				sumArea += list[i].getEnvelope().getArea();
				e1 = e1.unionEnvelope(list[i].getEnvelope());
			}
			S2 += e1.getArea() - sumArea;
			/*group 2 from k to M+1 -1*/
			e1 = list[k].getEnvelope();
			sumArea = 0;
			for (int i = k + 1; i < M + 1; i++)
			{
				sumArea += list[i].getEnvelope().getArea();
				e1 = e1.unionEnvelope(list[i].getEnvelope());
			}
			S2 += e1.getArea() - sumArea;
		}
		if (S1 < S2)return 1;
		else return 2;
	}

	/*if this is nonleaf node*/
	else
	{
		vector<RSNode*>& list = rnode->getAllNodes();
		/*axis x*/
		sort(begin(list), end(list), [](RSNode* f1, RSNode* f2) {
			if (f1->getEnvelope().getMinX() < f2->getEnvelope().getMinX()) return true;
			if (fabs(f1->getEnvelope().getMinX() - f2->getEnvelope().getMinX()) < 0.00000000005) return (f1->getEnvelope().getMaxX() < f2->getEnvelope().getMaxX());
			else return false; });
		/*sort(begin(list), end(list), [](RSNode* f1, RSNode* f2) {
			return f1->getEnvelope().getMinX() < f2->getEnvelope().getMinX(); });
		sort(begin(list), end(list), [](RSNode* f1, RSNode* f2) {
			return f1->getEnvelope().getMaxX() < f2->getEnvelope().getMaxX(); });*/
		double S1 = 0;//the margin value
		for (int k = m; k <= M + 1 - m; k++)
		{
			/*group 1 from 0 to k-1*/
			Envelope e1 = list[0]->getEnvelope();
			double sumArea = 0;
			for (int i = 1; i < k; i++)
			{
				sumArea += list[i]->getEnvelope().getArea();
				e1 = e1.unionEnvelope(list[i]->getEnvelope());
			}
			S1 += e1.getArea() - sumArea;
			/*group 2 from k to M+1 -1*/
			e1 = list[k]->getEnvelope();
			sumArea = 0;
			for (int i = k + 1; i < M + 1; i++)
			{
				sumArea += list[i]->getEnvelope().getArea();
				e1 = e1.unionEnvelope(list[i]->getEnvelope());
			}
			S1 += e1.getArea() - sumArea;
		}
		/*axis y*/
		sort(begin(list), end(list), [](RSNode* f1, RSNode*& f2) {
			if (f1->getEnvelope().getMinY() < f2->getEnvelope().getMinY()) return true;
			if (fabs(f1->getEnvelope().getMinY() - f2->getEnvelope().getMinY()) < 0.00000000005) return (f1->getEnvelope().getMaxY() < f2->getEnvelope().getMaxY());
			else return false;
		});
		/*sort(begin(list), end(list), [](RSNode* f1, RSNode* f2) {
			return f1->getEnvelope().getMinY() < f2->getEnvelope().getMinY(); });
		sort(begin(list), end(list), [](RSNode* f1, RSNode* f2) {
			return f1->getEnvelope().getMaxY() < f2->getEnvelope().getMaxY(); });*/
		double S2 = 0;
		for (int k = m; k <= M + 1 - m; k++)
		{
			/*group 1 from 0 to k-1*/
			Envelope e1 = list[0]->getEnvelope();
			double sumArea = 0;
			for (int i = 1; i < k; i++)
			{
				sumArea += list[i]->getEnvelope().getArea();
				e1 = e1.unionEnvelope(list[i]->getEnvelope());
			}
			S2 += e1.getArea() - sumArea;
			/*group 2 from k to M+1 -1*/
			e1 = list[k]->getEnvelope();
			sumArea = 0;
			for (int i = k + 1; i < M + 1; i++)
			{
				sumArea += list[i]->getEnvelope().getArea();
				e1 = e1.unionEnvelope(list[i]->getEnvelope());
			}
			S2 += e1.getArea() - sumArea;
		}
		if (S1 < S2)return 1;
		else return 2;
	}
}
int RStar::chooseSplitIndex(RSNode* rnode, int axis)
{
	double minOverlap = 666;
	int index = 0;
	double areavalue = 0;
	/*if this is leaf*/
	if (rnode->getChildNum() == 0)
	{
		/*if this is x - axis*/
		vector<Feature>& list = rnode->getAllFeature();
		
		if (axis == 1)
		{
			/*axis x*/
			sort(begin(list), end(list), [](Feature& f1, Feature& f2) {
				if (f1.getEnvelope().getMinX() < f2.getEnvelope().getMinX()) return true;
				if (fabs(f1.getEnvelope().getMinX() - f2.getEnvelope().getMinX()) < 0.00000000005) return (f1.getEnvelope().getMaxX() < f2.getEnvelope().getMaxX());
				else return false;
			});
			/*sort(begin(list), end(list), [](Feature& f1, Feature& f2) {
				return f1.getEnvelope().getMinX() < f2.getEnvelope().getMinX();
			});
			sort(begin(list), end(list), [](Feature& f1, Feature& f2) {
				return f1.getEnvelope().getMaxX() < f2.getEnvelope().getMaxX();
			});*/
		}
		/*if this is y - axis*/
		else
		{
			/*no need to sort because it has already been sorted along y axis*/
		}
		for (int k = m; k <= M + 1 - m; k++)
		{
			double overlap,area;
			/*the first group*/
			Envelope e1 = list[0].getEnvelope();
			for (int i = 1; i < k; i++)
			{
				e1 = e1.unionEnvelope(list[i].getEnvelope());
			}
			/*the second group*/
			Envelope e2 = list[k].getEnvelope();
			for (int i = k + 1; i < M + 1; i++)
			{
				e2 = e2.unionEnvelope(list[i].getEnvelope());
			}
			overlap = e1.intersection(e2).getArea();
			area = e1.getArea() + e2.getArea();
			if (overlap < minOverlap)
			{
				index = k;
				minOverlap = overlap;
				areavalue = e1.getArea() + e2.getArea();
			}
			/*resolve ties by area value*/
			else if (fabs(overlap - minOverlap) < 0.000000005)
			{
				if (area < areavalue)
				{
					index = k;
					minOverlap = overlap;
					areavalue = area;
				}
			}
		}

	}
	/*if this is nonleaf*/
	else
	{
		/*if this is x - axis*/
		vector<RSNode*>& list = rnode->getAllNodes();
		

		if (axis == 1)
		{
			/*axis x*/
			sort(begin(list), end(list), [](RSNode* f1, RSNode* f2) {
				if (f1->getEnvelope().getMinX() < f2->getEnvelope().getMinX()) return true;
				if (fabs(f1->getEnvelope().getMinX() - f2->getEnvelope().getMinX()) < 0.00000000005) return (f1->getEnvelope().getMaxX() < f2->getEnvelope().getMaxX());
				else return false; });
			/*sort(begin(list), end(list), [](RSNode* f1, RSNode* f2) {
				return f1->getEnvelope().getMinX() < f2->getEnvelope().getMinX(); });
			sort(begin(list), end(list), [](RSNode* f1, RSNode* f2) {
				return f1->getEnvelope().getMaxX() < f2->getEnvelope().getMaxX(); });*/

		}
		/*if this is y - axis*/
		else
		{
			/*no need to sort because it has already been sorted along y axis*/
		}
		for (int k = m; k <= M + 1 - m; k++)
		{
			double overlap, area;
			/*the first group*/
			Envelope e1 = list[0]->getEnvelope();
			for (int i = 1; i < k; i++)
			{
				e1 = e1.unionEnvelope(list[i]->getEnvelope());
			}
			/*the second group*/
			Envelope e2 = list[k]->getEnvelope();
			for (int i = k + 1; i < M + 1; i++)
			{
				e2 = e2.unionEnvelope(list[i]->getEnvelope());
			}
			overlap = e1.intersection(e2).getArea();
			area = e1.getArea() + e2.getArea();
			if (overlap < minOverlap)
			{
				index = k;
				minOverlap = overlap;
				areavalue = e1.getArea() + e2.getArea();
			}
			/*resolve ties by area value*/
			else if (fabs(overlap - minOverlap) < 0.000000005)
			{
				if (area < areavalue)
				{
					index = k;
					minOverlap = overlap;
					areavalue = area;
				}
			}
		}
	}

	return index;
}

void RStar::insert(Feature& f)
{
	RSNode* leaf = chooseSubtree(f.getEnvelope(),0);
	leaf->add(f);
	if (leaf->getFeatureNum() > M)
	{
		OverflowTreatment(leaf);
	}
	else
	{
		RSNode* ptr = leaf->getParent();
		while (ptr != NULL)
		{
			ptr->setEnvelope(ptr->getEnvelope().unionEnvelope(f.getEnvelope()));
			ptr = ptr->getParent();
		}
		
	}
}
void RStar::insert(RSNode* rnode)
{
	RSNode* ptr = chooseSubtree(rnode->getEnvelope(), rnode->calculateLevel()+1);
	ptr->add(rnode);
	if (ptr->getChildNum() > M)
	{
		OverflowTreatment(ptr);
	}
	else
	{
		RSNode* p = ptr->getParent();
		while (p != NULL)
		{
			p->setEnvelope(p->getEnvelope().unionEnvelope(ptr->getEnvelope()));
			p = p->getParent();
		}

	}
}
void RStar::OverflowTreatment(RSNode* rnode) //this is a propagate upwards function
{
	if (rnode->isFirstOverflow()&&rnode->getParent()!=NULL)
	{
		rnode->notFirst();
		reInsert(rnode);
		
	}
	else
	{
		Split(rnode);
		RSNode* parent = rnode->getParent();
		if (parent->getChildNum() > M)
			OverflowTreatment(parent);
	}
}

void RStar::reInsert(RSNode* rnode)
{
	
	struct indexDist {
		int index; //index
		double distance; //the square of the distance of the center
	};
	vector<struct indexDist>list;
	int p = M * 0.3;
	/*if this is leaf*/
	if (rnode->getChildNum() == 0)
	{
		for (int i = 0; i < rnode->getFeatureNum();i++)
		{
			struct indexDist id;
			id.index = i;
			id.distance = rnode->getFeature(i).getEnvelope().distance(rnode->getEnvelope());
			list.push_back(id);
		}
		//sort by decreasing order of their distances
		sort(begin(list), end(list), [](struct indexDist& i1, struct indexDist& i2) {
			return i1.distance > i2.distance;
		});
		vector<Feature>reinsertList;
		for (int i = 0; i < p; i++)
		{
			Feature f = rnode->getFeature(list[i].index);
			reinsertList.push_back(f);
		}
		for (int i = 0; i < reinsertList.size(); i++)
			rnode->deleteFeature(reinsertList[i]);
		updateEnvlope(rnode);
		//reinsert
		for (int i = 0; i < reinsertList.size(); i++)
		{
			insert(reinsertList[i]);
		}
		return;
	}
	//if this is non leaf
	else
	{
		for (int i = 0; i < rnode->getChildNum(); i++)
		{
			struct indexDist id;
			id.index = i;
			id.distance = rnode->getChildNode(i)->getEnvelope().distance(rnode->getEnvelope());
			list.push_back(id);
		}
		//sort by decreasing order of their distances
		sort(begin(list), end(list), [](struct indexDist& i1, struct indexDist& i2) {
			return i1.distance > i2.distance;
		});
		vector<RSNode*>reinsertList;
		for (int i = 0; i < p; i++)
		{
			RSNode* r = rnode->getChildNode(list[i].index);
			reinsertList.push_back(r);
		}
		for (int i = 0; i < reinsertList.size(); i++)
			rnode->deleteNode(reinsertList[i]);
		updateEnvlope(rnode);
		//reinsert
		for (int i = 0; i < reinsertList.size(); i++)
		{
			insert(reinsertList[i]);
		}
		return;
	}
}

bool RStar::constructRStar(vector<Feature>& features)
{
	cout << "R*-tree" << endl;
	if (features.size() == 0)
		return false;

	root = new RSNode(features[0], M);
	for (int i = 1; i < features.size(); i++)
		insert(features[i]);

	return true;
}

double RStar::CalculateMINDIST(double x, double y, const Envelope& rect)
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

void RStar::generateActiveBrachList(double x, double y, RSNode* rnode, ABLS& list)
{
	double mindist;
	for (int i = 0; i < rnode->getChildNum(); i++)
	{
		RSNode* p = rnode->getChildNode(i);
		mindist = CalculateMINDIST(x, y, p->getEnvelope());
		ABNS temp;
		temp.MINDIST = mindist;
		temp.rsnode = p;
		list.ABList.push_back(temp);
	}

}

void RStar::INN_Search(double x, double y, RSNode* rnode, NNTempS& NN_temp)
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
		ABLS list;
		generateActiveBrachList(x, y, rnode, list);//generate and calculate mindist
		sort(begin(list.ABList), end(list.ABList), [](const ABNS& p1, const ABNS& p2) {return p1.MINDIST < p2.MINDIST; });  //sort by mindist ascending
		for (int i = 0; i < list.ABList.size(); i++)
		{
			if (list.ABList[i].MINDIST < NN_temp.nearestDist)

				INN_Search(x, y, list.ABList[i].rsnode, NN_temp);
		}

	}
}

bool RStar::NNQuery(double x, double y, vector<Feature>& features)
{
	if (!root || !(root->getEnvelope().contain(x, y)))
		return false;
	features.clear();

	NNTempS nntemp;
	nntemp.nearestDist = 66666;
	INN_Search(x, y, root, nntemp);
	features.push_back(nntemp.f);
	return true;
}



}
