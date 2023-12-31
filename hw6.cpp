// hw6.cpp : 定义控制台应用程序的入口点。
//

#include "common.h"
#include "Geometry.h"
#include "shapelib/shapefil.h"

#include <gl/freeglut.h>       // Glut库头文件

#include <iostream>
#include <vector>
#include <cstdio> 
#include <ctime>
#include <map>
#include <list>

using namespace std;

extern void test(int t);
extern void QuadTreeAnalysis();

int screenWidth  = 640;
int screenHeight = 480;

double pointSize = 2.0;

int mode;

vector<hw6::Feature> features;
vector<hw6::Feature> roads;
bool showRoad = true;

hw6::QuadTree pointQTree;
hw6::QuadTree roadQTree;
bool showQuadTree = false;

hw6::RTree pointRTree;
hw6::RTree roadRTree;
hw6::RStar pointRStar;
hw6::RStar roadRStar;
bool showRTree = false;

hw6::Feature nearestFeature;

bool firstPoint = true;
hw6::Point corner[2];
hw6::Envelope selectedRect;
vector<hw6::Feature> selectedFeatures;

/*
 * shapefile文件中name和geometry属性读取
 */
vector<string> readName(const char* filename)
{
	DBFHandle file = DBFOpen(filename, "r");

	vector<string> res;
	int cct = DBFGetRecordCount(file);
	res.reserve(cct);
	for (int i = 0; i < cct; ++i){
		string a = to_string(i)+DBFReadStringAttribute(file, i, 0);
		res.push_back(a);
	}

	DBFClose(file);

	return res;
}

vector<hw6::Geometry *> readGeom(const char *filename)
{
	SHPHandle file = SHPOpen(filename, "r");

	int pnEntities, pnShapeType;
	double padfMinBound[4], padfMaxBound[4];
	SHPGetInfo(file, &pnEntities, &pnShapeType, padfMinBound, padfMaxBound);
	
	vector<hw6::Point> points;
	vector<hw6::Geometry *> geoms;
	geoms.reserve(pnEntities);
	switch (pnShapeType){
	case SHPT_POINT:
		for (int i = 0; i < pnEntities; ++i) {
			SHPObject *pt = SHPReadObject(file, i);
			geoms.push_back(new hw6::Point(pt->padfY[0], pt->padfX[0]));
			SHPDestroyObject(pt);
		}
		break;

	case SHPT_ARC:
		for (int i = 0; i < pnEntities; ++i) {
			points.clear();
			SHPObject *pt = SHPReadObject(file, i);
			for (int j = 0; j < pt->nVertices; ++j){
				points.push_back(hw6::Point(pt->padfY[j], pt->padfX[j]));
			}
			SHPDestroyObject(pt);
			geoms.push_back(new hw6::LineString(points));
		}
		break;

	case SHPT_POLYGON:
		for (int i = 0; i < pnEntities; ++i) {
			points.clear();
			SHPObject *pt = SHPReadObject(file, i);
			for (int j = 0; j < pt->nVertices; ++j){
				points.push_back(hw6::Point(pt->padfY[j], pt->padfX[j]));
			}
			SHPDestroyObject(pt);
			hw6::LineString line(points);
			hw6::Polygon *poly = new hw6::Polygon(line);
			geoms.push_back(new hw6::Polygon(line));
		}
		break;
	}
	
	SHPClose(file);
	return geoms;
}

/*
 * 输出几何信息
 */
void printGeom(vector<hw6::Geometry *>& geom)
{
	cout << "Geometry:" << endl;
	for (vector<hw6::Geometry *>::iterator it = geom.begin(); it != geom.end(); ++it) {
		(*it)->print();
	}
}

/*
 * 删除几何信息
 */
void deleteGeom(vector<hw6::Geometry *>& geom)
{
	for (vector<hw6::Geometry *>::iterator it = geom.begin(); it != geom.end(); ++it) {
		delete *it;
		*it = NULL;
	}
	geom.clear();
}

/*
 * 读取纽约道路数据
 */
void loadRoadData()
{
	vector<hw6::Geometry *> geom = readGeom(".//data/highway");

	roads.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		roads.push_back(hw6::Feature(to_string(i), geom[i]));

	cout << "road number: " << geom.size() << endl;
	roadQTree.setCapacity(20);
	roadQTree.constructQuadTree(roads);
}
void loadRoadDataR()
{
	vector<hw6::Geometry*> geom = readGeom(".//data/highway");

	roads.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		roads.push_back(hw6::Feature(to_string(i), geom[i]));

	cout << "road number: " << geom.size() << endl;
	roadRTree.setConfig(10);
	roadRTree.constructRTree(roads);
	cout <<"R-tree height: " << roadRTree.getHeight() << endl;
	roadRStar.setConfig(10);
	roadRStar.constructRStar(roads);
	cout << "R*-tree height: " << roadRStar.getHeight() << endl;
}

/*
 * 读取纽约自行车租赁点数据
 */
void loadStationData()
{
	vector<hw6::Geometry *> geom = readGeom(".//data/station");
	vector<string> name = readName(".//data/station");

	features.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		features.push_back(hw6::Feature(name[i], geom[i]));

	cout << "station number: " << geom.size() << endl;
	pointQTree.setCapacity(5);
	pointQTree.constructQuadTree(features);
}
/*
 * 用R树读取纽约自行车租赁点数据
 */
void loadStationDataR()
{
	vector<hw6::Geometry*> geom = readGeom(".//data/station");
	vector<string> name = readName(".//data/station");

	features.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		features.push_back(hw6::Feature(name[i], geom[i]));

	cout << "station number: " << geom.size() << endl;
	pointRTree.setConfig(4);
	pointRTree.constructRTree(features);

	pointRStar.setConfig(6);
	pointRStar.constructRStar(features);

}


/*
 * 读取纽约出租车打车点数据
 */
void loadTaxiData()
{
	vector<hw6::Geometry *> geom = readGeom(".//data/taxi");
	vector<string> name = readName(".//data/taxi");

	features.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		features.push_back(hw6::Feature(name[i], geom[i]));

	cout << "taxi number: " << geom.size() << endl;
	pointQTree.setCapacity(100);
	pointQTree.constructQuadTree(features);
}

/*
 * 区域查询
 */
void rangeQuery()
{
	vector<hw6::Feature> candidateFeatures;

	// filter step (使用四叉树获得查询区域和几何特征包围盒相交的候选集）
	if (mode == RANGEPOINT)
		pointQTree.rangeQuery(selectedRect, candidateFeatures);
	else if (mode == RANGELINE)
		roadQTree.rangeQuery(selectedRect, candidateFeatures);

	selectedFeatures.clear();
	// refine step (精确判断时，需要去重，避免查询区域和几何对象的重复计算)
	// write your here to update selectedFeatures
	for (int i = 0; i < candidateFeatures.size(); i++)
	{
		if (selectedRect.contain(candidateFeatures[i].getEnvelope()))
		{
			selectedFeatures.push_back(candidateFeatures[i]);
			continue;
		}
		else if (candidateFeatures[i].getGeom()->intersects(selectedRect))
		{
			selectedFeatures.push_back(candidateFeatures[i]);
		}		
	}
}
void rangeQueryR()
{
	vector<hw6::Feature> candidateFeatures;

	// filter step (使用四叉树获得查询区域和几何特征包围盒相交的候选集）
	if (mode == RANGEPOINTR)
		pointRTree.rangeQuery(selectedRect, candidateFeatures);
	else if (mode == RANGELINER)
		roadRTree.rangeQuery(selectedRect, candidateFeatures);

	selectedFeatures.clear();
	// refine step (精确判断时，需要去重，避免查询区域和几何对象的重复计算)
	// write your here to update selectedFeatures
	for (int i = 0; i < candidateFeatures.size(); i++)
	{
		if (selectedRect.contain(candidateFeatures[i].getEnvelope()))
		{
			selectedFeatures.push_back(candidateFeatures[i]);
			continue;
		}
		else if (candidateFeatures[i].getGeom()->intersects(selectedRect))
		{
			selectedFeatures.push_back(candidateFeatures[i]);
		}
	}
}

/*
 * 邻近查询
 */
void NNQuery(hw6::Point p)
{
	vector<hw6::Feature> candidateFeatures;
	// filter step (使用四叉树获得距离较近的几何特征候选集)
	if (mode == NNPOINT)
		pointQTree.NNQuery(p.getX(), p.getY(), candidateFeatures);
	else if (mode == NNLINE)
		roadQTree.NNQuery(p.getX(), p.getY(), candidateFeatures);

	// refine step (精确计算查询点与几何对象的距离)
	// write your here to update nearestFeature
	double dist;
	double minDist;

	size_t index=0;
	for (size_t i = 0; i < candidateFeatures.size(); i++) 
	{
		if (i == 0)
		{
			dist = (candidateFeatures[i].getGeom())->distance(&p);
			minDist = dist;
			index = 0;
		}

		else 
		{
			dist = (candidateFeatures[i].getGeom())->distance(&p);
			if (dist <= minDist) 
			{
				minDist = dist;
				index = i;
			}
		}
	}
	if (candidateFeatures.size() != 0) 
	{
		nearestFeature = candidateFeatures[index];
	}
	
}
void NNQueryR(hw6::Point p)
{
	vector<hw6::Feature> candidateFeatures;
	nearestFeature = hw6::Feature();
	// filter step (使用四叉树获得距离较近的几何特征候选集)
	if (mode == NNPOINTR)
		//pointRTree.NNQuery(p.getX(), p.getY(), candidateFeatures);
		pointRStar.NNQuery(p.getX(), p.getY(), candidateFeatures);
	else if (mode == NNLINER)
		//roadRTree.NNQuery(p.getX(), p.getY(), candidateFeatures);
		roadRStar.NNQuery(p.getX(), p.getY(), candidateFeatures);

	// refine step (精确计算查询点与几何对象的距离)
	// write your here to update nearestFeature
	double dist;
	double minDist;

	size_t index = 0;
	for (size_t i = 0; i < candidateFeatures.size(); i++)
	{
		if (i == 0)
		{
			dist = (candidateFeatures[i].getGeom())->distance(&p);
			minDist = dist;
			index = 0;
		}

		else
		{
			dist = (candidateFeatures[i].getGeom())->distance(&p);
			if (dist <= minDist)
			{
				minDist = dist;
				index = i;
			}
		}
	}
	if (candidateFeatures.size() != 0)
	{
		nearestFeature = candidateFeatures[index];
	}

}

/*
 * spatial join
 */
void spatialjoin(hw6::Point p)
{
	vector<hw6::Feature> candidateFeatures;
	// filter step (使用四叉树获得距离较近的几何特征候选集)
	roadQTree.NNQuery(p.getX(), p.getY(), candidateFeatures);
	// refine step (精确计算查询点与几何对象的距离)
	// write your here to update nearestFeature
	double dist;
	double minDist;
	size_t index = 0;
	for (size_t i = 0; i < candidateFeatures.size(); i++)
	{
		if (i == 0)
		{
			dist = (candidateFeatures[i].getGeom())->distance(&p);
			minDist = dist;
			index = 0;
		}

		else
		{
			dist = (candidateFeatures[i].getGeom())->distance(&p);
			if (dist <= minDist)
			{
				minDist = dist;
				index = i;
			}
		}
	}
	if (candidateFeatures.size() != 0)
	{
		nearestFeature = candidateFeatures[index];
	}
	//接下来寻找这条边上的点
	//设定包围盒
	hw6::Envelope Rect = nearestFeature.getEnvelope();
	// filter step (使用四叉树获得查询区域和几何特征包围盒相交的候选集）
	pointQTree.rangeQuery(Rect, candidateFeatures);
	selectedFeatures.clear();
	// refine step (精确判断时，需要去重，避免查询区域和几何对象的重复计算)
	// write your here to update selectedFeatures
	for (int i = 0; i < candidateFeatures.size(); i++)
	{
		const hw6::Point* p1;
		p1 = dynamic_cast<const hw6::Point*>(candidateFeatures[i].getGeom());
		if ((nearestFeature.getGeom())->distance(p1) < 0.0005)
		{
			selectedFeatures.push_back(candidateFeatures[i]);
		}
	}
}
void spatialjoinR(hw6::Point p)
{
	vector<hw6::Feature> candidateFeatures;
	// filter step (使用四叉树获得距离较近的几何特征候选集)
	roadRTree.NNQuery(p.getX(), p.getY(), candidateFeatures);
	// refine step (精确计算查询点与几何对象的距离)
	// write your here to update nearestFeature
	double dist;
	double minDist;
	size_t index = 0;
	for (size_t i = 0; i < candidateFeatures.size(); i++)
	{
		if (i == 0)
		{
			dist = (candidateFeatures[i].getGeom())->distance(&p);
			minDist = dist;
			index = 0;
		}

		else
		{
			dist = (candidateFeatures[i].getGeom())->distance(&p);
			if (dist <= minDist)
			{
				minDist = dist;
				index = i;
			}
		}
	}
	if (candidateFeatures.size() != 0)
	{
		nearestFeature = candidateFeatures[index];
	}
	//接下来寻找这条边上的点
	//设定包围盒
	hw6::Envelope Rect = nearestFeature.getEnvelope();
	// filter step (使用四叉树获得查询区域和几何特征包围盒相交的候选集）
	pointRTree.rangeQuery(Rect, candidateFeatures);
	selectedFeatures.clear();
	// refine step (精确判断时，需要去重，避免查询区域和几何对象的重复计算)
	// write your here to update selectedFeatures
	for (int i = 0; i < candidateFeatures.size(); i++)
	{
		const hw6::Point* p1;
		p1 = dynamic_cast<const hw6::Point*>(candidateFeatures[i].getGeom());
		if ((nearestFeature.getGeom())->distance(p1) < 0.0005)
		{
			selectedFeatures.push_back(candidateFeatures[i]);
		}
	}
}

/*
 * 从屏幕坐标转换到地理坐标
 */
void transfromPt(hw6::Point &pt)
{
	const hw6::Envelope bbox = pointQTree.getEnvelope();
	double width = bbox.getMaxX() - bbox.getMinX() + 0.002;
	double height = bbox.getMaxY() - bbox.getMinY() + 0.002;

	double x = pt.getX() * width / screenWidth + bbox.getMinX() - 0.001;
	double y = pt.getY() * height / screenHeight + bbox.getMinY() - 0.001;
	
	x = max(bbox.getMinX(), x);
	x = min(bbox.getMaxX(), x);
	y = max(bbox.getMinY(), y);
	y = min(bbox.getMaxY(), y);
	pt = hw6::Point(x, y);
}

/*
 * 绘制代码
 */
void display()
{
	//glClearColor(241 / 255.0, 238 / 255.0, 232 / 255.0, 0.0); 
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	const hw6::Envelope bbox = pointQTree.getEnvelope();
	gluOrtho2D(bbox.getMinX() - 0.001, bbox.getMaxX() + 0.001, bbox.getMinY() - 0.001, bbox.getMaxY() + 0.001);

	// 道路绘制
	if (showRoad) {
		glColor3d(252 / 255.0, 214 / 255.0, 164 / 255.0);
		for (size_t i = 0; i < roads.size(); ++i)
			roads[i].draw();
	}
	
	// 点绘制
	if (!(mode == RANGELINE || mode == NNLINE || mode == RANGELINER || mode==NNLINER)) {
		glPointSize((float)pointSize);
		glColor3d(0.0, 146 / 255.0, 247 / 255.0);
		for (size_t i = 0; i < features.size(); ++i)
			features[i].draw();
	}
	
	// 四叉树绘制
	if (showQuadTree) {
		glColor3d(0.0, 146 / 255.0, 247 / 255.0);
		if (mode == RANGELINE || mode == NNLINE )
			roadQTree.draw();
		else
			pointQTree.draw();
	}

	//R树绘制
	if (showRTree) {
		glColor3d(0.0, 146 / 255.0, 247 / 255.0);
		if (mode == RANGELINE || mode == NNLINE || mode == RANGELINER || mode == NNLINER)
			//roadRTree.draw();
			roadRStar.draw();
		else
			pointRTree.draw();
			//pointRStar.draw();
	}

	// 离鼠标最近点绘制
	if (mode == NNPOINT || NNPOINTR) {
		glPointSize(5.0);
		glColor3d(0.9, 0.0, 0.0);
		nearestFeature.draw();
	}

	// 离鼠标最近道路绘制
	if (mode == NNLINE || NNLINER || mode == SPATIALJOIN || mode == SPATIALJOINR) {
		glLineWidth(3.0);
		glColor3d(0.9, 0.0, 0.0);
		nearestFeature.draw();
		glLineWidth(1.0);
	}


	// 区域选择绘制
	if (mode == RANGEPOINT || mode == RANGELINE || mode == RANGEPOINTR || mode == RANGELINER ) {
		glColor3d(0.0, 0.0, 0.0);
		selectedRect.draw();
		glColor3d(1.0, 0.0, 0.0);
		for (size_t i = 0; i < selectedFeatures.size(); ++i)
			selectedFeatures[i].draw();
	}
	else if (mode == SPATIALJOIN || mode == SPATIALJOINR) {
		glColor3d(1.0, 0.5, 0.0);
		for (size_t i = 0; i < selectedFeatures.size(); ++i)
			selectedFeatures[i].draw();
	}

	glFlush();
	glutSwapBuffers();
}

/*
 * 鼠标和键盘交互
 */
void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (mode == RANGEPOINT || mode == RANGELINE) {
			if (firstPoint) {
				selectedFeatures.clear();
				nearestFeature = hw6::Feature();
				corner[0] = hw6::Point(x, screenHeight - y);
				transfromPt(corner[0]);
			}
			else {
				corner[1] = hw6::Point(x, screenHeight - y);
				transfromPt(corner[1]);
				selectedRect = hw6::Envelope(min(corner[0].getX(), corner[1].getX()), max(corner[0].getX(), corner[1].getX()),
										     min(corner[0].getY(), corner[1].getY()), max(corner[0].getY(), corner[1].getY()));
				rangeQuery();
			}
			firstPoint = !firstPoint;
			glutPostRedisplay();
		}
		else if (mode == RANGEPOINTR || mode == RANGELINER) {
			if (firstPoint) {
				selectedFeatures.clear();
				nearestFeature = hw6::Feature();
				corner[0] = hw6::Point(x, screenHeight - y);
				transfromPt(corner[0]);
			}
			else {
				corner[1] = hw6::Point(x, screenHeight - y);
				transfromPt(corner[1]);
				selectedRect = hw6::Envelope(min(corner[0].getX(), corner[1].getX()), max(corner[0].getX(), corner[1].getX()),
					min(corner[0].getY(), corner[1].getY()), max(corner[0].getY(), corner[1].getY()));
				rangeQueryR();
			}
			firstPoint = !firstPoint;
			glutPostRedisplay();
		}
	}
}

void passiveMotion(int x, int y)
{
	corner[1] = hw6::Point(x, screenHeight - y);

	if ((mode == RANGEPOINT || mode == RANGELINE) && !firstPoint) {
		corner[1] = hw6::Point(x, screenHeight - y);
		transfromPt(corner[1]);
		selectedRect = hw6::Envelope(min(corner[0].getX(), corner[1].getX()), max(corner[0].getX(), corner[1].getX()),
								     min(corner[0].getY(), corner[1].getY()), max(corner[0].getY(), corner[1].getY()));
		rangeQuery();

		glutPostRedisplay();
	}
	else if (mode == NNPOINT || mode == NNLINE) {
		hw6::Point p(x, screenHeight - y);
		transfromPt(p);
		NNQuery(p);

		glutPostRedisplay();
	}
	else if ((mode == RANGEPOINTR || mode == RANGELINER) && !firstPoint) {
		corner[1] = hw6::Point(x, screenHeight - y);
		transfromPt(corner[1]);
		selectedRect = hw6::Envelope(min(corner[0].getX(), corner[1].getX()), max(corner[0].getX(), corner[1].getX()),
			min(corner[0].getY(), corner[1].getY()), max(corner[0].getY(), corner[1].getY()));
		rangeQueryR();

		glutPostRedisplay();
	}
	else if (mode == NNPOINTR || mode == NNLINER) {
		hw6::Point p(x, screenHeight - y);
		transfromPt(p);
		NNQueryR(p);

		glutPostRedisplay();
	}
	else if (mode == SPATIALJOIN)
	{
		hw6::Point p(x, screenHeight - y);
		transfromPt(p);
		spatialjoin(p);

		glutPostRedisplay();
	}
	else if (mode == SPATIALJOINR)
	{
		hw6::Point p(x, screenHeight - y);
		transfromPt(p);
		spatialjoinR(p);

		glutPostRedisplay();
	}
}

void changeSize(int w, int h)
{
	screenWidth = w;
	screenHeight = h;
	glViewport(0, 0, w, h);
	glutPostRedisplay();
}

void processNormalKeys(unsigned char key, int x, int y)
{
	switch(key) {
		case 27:exit(0); break;
		case 'N':
			mode = NNLINE; break;
		case 'n':
			mode = NNPOINT; break;
		case 'Z':
			mode = NNLINER; break;
		case 'z':
			mode = NNPOINTR; break;
		case 'S':
			mode = RANGELINE;
			firstPoint = true;
			break;
		case 's':
			mode = RANGEPOINT; 
			firstPoint = true; 
			break;
		case 'X':
			mode = RANGELINER;
			firstPoint = true;
			break;
		case 'x':
			mode = RANGEPOINTR;
			firstPoint = true;
			break;
		case 'J':
			mode = SPATIALJOIN;
			firstPoint = true;
			break;
		case 'j':
			mode = SPATIALJOINR;
			firstPoint = true;
			break;
		case 'B':
		case 'b':
			loadStationData();
			mode = Default;
			break;
		case 'T':
		case 't':
			loadTaxiData();
			mode = Default;
			break;
		case 'R':
		case 'r':
			showRoad = !showRoad;
			break;
		case 'Q':
		case 'q':
			showQuadTree = !showQuadTree;
			break;
		case 'O':
		case 'o':
			showRTree = !showRTree;
			break;
		case '+':
			pointSize *= 1.1;
			break;
		case '-':
			pointSize /= 1.1;
			break;
		case '1':
			test(TEST1); break;
		case '2':
			test(TEST2); break;
		case '3':
			test(TEST3); break;
		case '4':
			test(TEST4); break;
		case '5':
			test(TEST5); break;
		case '6':
			test(TEST6); break;
		case '7':
			test(TEST7); break;
		case '8':
			test(TEST8); break;
		case '9':
			test(TEST9); break;
		case '0':
			test(TEST10); break;
		default: 
			mode = Default; break;
	}
	glutPostRedisplay();
}

int main(int argc, char* argv[])
{
	cout << "Key Usage:\n"
		 << "  S  : range search for roads\n"
		 << "  s  : range search for stations\n"
		 << "  X  : range search for roads(Using RTree)\n"
		 << "  x  : range search for stations(Using RTree)\n"
		 << "  N  : nearest road search\n"
		 << "  n  : nearest station search\n"
		 << "  Z  : nearest road search(Using RTree)\n"
		 << "  z  : nearest station search(Using RTree)\n"
		 << "  J  : Spatial join\n"
		 << "  j  : Spatial join(Using RTree)\n"
		 << "  B/b: Bicycle data\n"
		 << "  T/t: Taxi data\n"
		 << "  R/r: show Road\n"
		 << "  Q/q: show QuadTree\n"
		 << "  O/o: show RTree\n "
		 << "  +  : increase point size\n"
		 << "  -  : decrease point size\n"
		 << "  1  : Test Envelope contain, interset and union\n"
		 << "  2  : Test distance between Point and LineString\n"
		 << "  3  : Test distance between Point and Polygon\n"
		 << "  4  : Test quadtree construction\n"
		 << "  5  : Test r-tree\n"
		 << "  8  : Quadtree performance analysis\n"
		 << "  9  : R-tree performance analysis\n"
		 << "  0  : R*-tree performance analysis\n"
		 << "  ESC: quit\n"
		 << endl;

	loadRoadData();
	loadRoadDataR();
	loadStationDataR();
	loadStationData();

	glutInit(&argc, argv);
	glutInitWindowSize(screenWidth, screenHeight);
	glutInitWindowPosition(0, 0);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("New York");

	glutMouseFunc(mouse);
	glutDisplayFunc(display);
	glutPassiveMotionFunc(passiveMotion);
	glutReshapeFunc(changeSize);
	glutKeyboardFunc(processNormalKeys);

	glutMainLoop();

	return 0;
}

