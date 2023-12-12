#include "Geometry.h"
#include <cmath>
#include <gl/freeglut.h> 

#define NOT_IMPLEMENT -1.0

namespace hw6 {

/*
 * Envelope functions
 */
bool Envelope::contain(double x, double y) const
{
	return x >= minX && x <= maxX && y >= minY && y <= maxY;
}
double Envelope::distance(const Envelope& e)const
{
	////����������Χ�е������룬�����Ǻϲ���ĶԽ��߳�
	//Envelope env = this->unionEnvelope(e);
	//return sqrt((env.maxX - env.minX) * (env.maxX - env.minX) + (env.maxY - env.minY) * (env.maxY - env.minY));
	//����������Χ�����ľ����ƽ��
	double deltaX = (minX + maxX) / 2 - (e.minX + e.maxX) / 2;
	double deltaY = (minY + maxY) / 2 - (e.minY + e.maxY) / 2;
	return deltaX * deltaX + deltaY * deltaY;
}

bool Envelope::contain(const Envelope& envelope) const
{
	// Task ����Envelope�Ƿ������ϵ
	// Write your code here
	if (envelope.getMinX() >= minX && envelope.getMinY() >= minY && envelope.getMaxX() <= maxX && envelope.getMaxY() <= maxY)
		return true;
	return false;
}

bool Envelope::intersect(const Envelope& envelope) const
{
	// Task ����Envelope�Ƿ��ཻ
	// Write your code here
	//���� !disjoint(geom) �����뼴�ཻ
	if (!(minX > envelope.maxX || maxX<envelope.minX || minY>envelope.maxY || maxY < envelope.minY))
		return true;
	return false;
}

Envelope Envelope::unionEnvelope(const Envelope& envelope) const
{
	// Task �ϲ�����Envelope����һ���µ�Envelope
	// Write your code here
	double minx = minX < envelope.minX ? minX : envelope.minX;
	double miny = minY < envelope.minY ? minY : envelope.minY;
	double maxx = maxX > envelope.maxX ? maxX : envelope.maxX;
	double maxy = maxY > envelope.maxY ? maxY : envelope.maxY;
	return Envelope(minx,maxx,miny,maxy);
}
Envelope Envelope::intersection(const Envelope& envelope)const
{
	//��������Χ���ཻ�Ĳ���
	if (!this->contain(envelope))
		return Envelope(0, 0, 0, 0);
	double minx = minX < envelope.minX ? envelope.minX: minX;
	double miny = minY < envelope.minY ? envelope.minY: minY;
	double maxx = maxX > envelope.maxX ? envelope.maxX: maxX;
	double maxy = maxY > envelope.maxY ? envelope.maxY: maxY;
	return Envelope(minx, maxx, miny, maxy);
}

void Envelope::draw() const
{
	glBegin(GL_LINE_STRIP);

	glVertex2d(minX, minY);
	glVertex2d(minX, maxY);
	glVertex2d(maxX, maxY);
	glVertex2d(maxX, minY);
	glVertex2d(minX, minY);

	glEnd();
}


/*
 * Points functions
 */
double Point::distance(const Point* point) const
{
	return sqrt((x - point->x) * (x - point->x) + (y - point->y) * (y - point->y));
}

double Point::distance(const LineString* line) const
{
	double mindist = line->getPointN(0).distance(this);
	for (size_t i = 0; i < line->numPoints() - 1; ++i) {
		double dist = 0;
		double x1 = line->getPointN(i).getX();
		double y1 = line->getPointN(i).getY();
		double x2 = line->getPointN(i + 1).getX();
		double y2 = line->getPointN(i + 1).getY();
		// Task calculate the distance between Point P(x, y) and Line [P1(x1, y1), P2(x2, y2)] (less than 10 lines)
		// Write your code here
		//�����߶ε�k��b�Լ��㵽�߶δ��ߵ�k��b
		int flag = 1;   //�жϵ�Ĵ��ߵ��߶��Ƿ��н��㣬����Ϊ1������Ϊ0
		if (x1 == x2)//�������1������
		{
			if ((y1 - this->getY()) * (y2 - this->getY()) <= 0)
			{
				dist = fabs(this->getX() - x1);
			}
			else flag = 0;
		}
		else if (y1 == y2)//�������2������
		{
			if ((x1 - this->getX()) * (x2 - this->getX()) <= 0)
			{
				dist = fabs(this->getY() - y1);
			}
			else flag = 0;
		}
		else //�������
		{
			double kl = (y1 - y2) / (x1 - x2);
			double bl = y1 - kl * x1;
			double kp = - 1.0 / kl;
			double bp = this->getY() - kp * this->getX();
			double xx = (bp - bl) / (kl - kp);
			double yy = kl * xx + bl;
			if ((x1 - xx) * (x2 - xx) <= 0)
			{
				Point p(xx, yy);
				dist = this->distance(&p);
			}
			else flag = 0;
		}
		if (flag==0)//������ߺ��߶��޽��㣬�򷵻ؾ�����С�Ķ˵����
		{
			dist = this->distance(&line->getPointN(i));
			double dist1 = this->distance(&line->getPointN(i + 1));
			if (dist1<dist)
			{
				dist = dist1;
			}
		}
		
		if (dist < mindist)
			mindist = dist;
	}
	return mindist;
}

double Point::distance(const Polygon* polygon) const
{
	LineString line = polygon->getExteriorRing();
	size_t n = line.numPoints();
	bool inPolygon = bInPolygon(polygon);
	
	double mindist = 0;
	if (!inPolygon)
		mindist = this->distance(&line);
	return mindist;
}

bool Point::bInPolygon(const Polygon* polygon)const
{
	bool inPolygon = false;
	// Task whether Point P(x, y) is within Polygon (less than 15 lines)
	// write your code here
	if (polygon->getEnvelope().contain(this->envelope))
	{
		//ʹ�����߷��жϵ�Ͷ����λ�ù�ϵ
		int count = 0;
		double k, b;
		for (size_t i = 0; i < polygon->getExteriorRing().numPoints() - 1; ++i)
		{
			double x1 = polygon->getExteriorRing().getPointN(i).getX();
			double y1 = polygon->getExteriorRing().getPointN(i).getY();
			double x2 = polygon->getExteriorRing().getPointN(i + 1).getX();
			double y2 = polygon->getExteriorRing().getPointN(i + 1).getY();
			//ע��һ�����ᷢ��k����Ϊ0���������Ϊ��x1=x2����(x2 - x1 > 0 ? x2 : x1) - this->getX() != 0һ����ͨ����ѹ���������k��
			if ((x2 - this->getX()) * (x1 - this->getX()) <= 0 && (x2 - x1 > 0 ? x2 : x1) - this->getX() != 0)//������ֲ��ڸõ�����ʱ���п����н���
			{
				k = (y2 - y1) / (x2 - x1);
				b = y2 - k * x2;
				if (k * this->getX() + b > this->getY())
					count++;
			}
		}
		if (count % 2 == 1)
		{
			inPolygon = true;
		}
	}
	return inPolygon;
}

bool Point::intersects(const Envelope& rect)  const
{
	return (x >= rect.getMinX()) && (x <= rect.getMaxX()) && (y >= rect.getMinY()) && (y <= rect.getMaxY());
}

void Point::draw()  const
{
	glBegin(GL_POINTS);
	glVertex2d(x, y);
	glEnd();
}


/*
 * LineString functions
 */
void LineString::constructEnvelope()
{
	double minX, minY, maxX, maxY;
	maxX = minX = points[0].getX();
	maxY = minY = points[0].getY();
	for (size_t i = 1; i < points.size(); ++i) {
		maxX = max(maxX, points[i].getX());
		maxY = max(maxY, points[i].getY());
		minX = min(minX, points[i].getX());
		minY = min(minY, points[i].getY());
	}
	envelope = Envelope(minX, maxX, minY, maxY);
}

bool LineString::intersection(const LineString* line) const 
{
	double x1, y1, x2, y2,x3,y3,x4,y4;
	double A1, B1, A2, B2, A3, B3;
	for (size_t i = 0; i < points.size() - 1; i++) 
	{
		for (size_t j = 0; j < line->numPoints() - 1; j++) 
		{
			x1 = points[i].getX();
			y1 = points[i].getY();
			x2 = points[i+1].getX();
			y2 = points[i+1].getY();
			x3 = line->getPointN(j).getX();
			y3 = line->getPointN(j).getY();
			x4 = line->getPointN(j+1).getX();
			y4 = line->getPointN(j+1).getY();
			A1 = x2 - x1;
			B1 = y2 - y1;
			A2 = x3 - x1;
			B2 = y3 - y1;
			A3 = x4 - x1;
			B3 = y4 - y1;
			if ((A1*B2 - A2 * B1)*(A1*B3 - A3 * B1) <= 0)
				return TRUE;
		}
	}
	return FALSE;
}

double LineString::distance(const LineString* line) const
{
	if (intersection(line) == TRUE)
		return 0;
	double min = points[0].distance(line);
	double dist;
	for (size_t i = 0; i < points.size(); i++) 
	{
		dist = points[i].distance(line);
		if (dist < min)
			min = dist;
	}
	return min;	
}

double LineString::distance(const Polygon* polygon) const
{
	int flag=0;
	if (intersection(&polygon->getExteriorRing()) == TRUE)
		return 0;
	for (size_t i = 0; i < points.size(); i++) 
	{
		if (points[i].bInPolygon(polygon) == FALSE)
		{
			flag =1;
			break;
		}
	}
	if (flag == 0)
		return 0;
	LineString line = polygon->getExteriorRing();
	double min = points[0].distance(&line);
	double dist;
	for (size_t i = 0; i < points.size(); i++) 
	{
		dist = points[i].distance(&line);
		if (dist < min)
			min = dist;
	}
	return min;
}

typedef int OutCode;

const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

// Compute the bit code for a point (x, y) using the clip rectangle
// bounded diagonally by (xmin, ymin), and (xmax, ymax)
// ASSUME THAT xmax, xmin, ymax and ymin are global constants.
OutCode ComputeOutCode(double x, double y, double xmin, double xmax, double ymin, double ymax)
{
	OutCode code;

	code = INSIDE;          // initialised as being inside of [[clip window]]

	if (x < xmin)           // to the left of clip window
		code |= LEFT;
	else if (x > xmax)      // to the right of clip window
		code |= RIGHT;
	if (y < ymin)           // below the clip window
		code |= BOTTOM;
	else if (y > ymax)      // above the clip window
		code |= TOP;
	
	return code;
}

// Cohen�CSutherland clipping algorithm clips a line from
// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with 
// diagonal from (xmin, ymin) to (xmax, ymax).
bool intersectTest(double x0, double y0, double x1, double y1, double xmin, double xmax, double ymin, double ymax)
{
	// compute outcodes for P0, P1, and whatever point lies outside the clip rectangle
	OutCode outcode0 = ComputeOutCode(x0, y0, xmin, xmax, ymin, ymax);
	OutCode outcode1 = ComputeOutCode(x1, y1, xmin, xmax, ymin, ymax);
	bool accept = false;

	while (true) {
		if (!(outcode0 | outcode1)) {
			// bitwise OR is 0: both points inside window; trivially accept and exit loop
			accept = true;
			break;
		}
		else if (outcode0 & outcode1) {
			// bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, TOP,
			// or BOTTOM), so both must be outside window; exit loop (accept is false)
			break;
		}
		else {
			// failed both tests, so calculate the line segment to clip
			// from an outside point to an intersection with clip edge
			double x, y;

			// At least one endpoint is outside the clip rectangle; pick it.
			OutCode outcodeOut = outcode0 ? outcode0 : outcode1;

			// Now find the intersection point;
			// use formulas:
			//   slope = (y1 - y0) / (x1 - x0)
			//   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
			//   y = y0 + slope * (xm - x0), where xm is xmin or xmax
			// No need to worry about divide-by-zero because, in each case, the
			// outcode bit being tested guarantees the denominator is non-zero
			if (outcodeOut & TOP) {           // point is above the clip window
				x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
				y = ymax;
			}
			else if (outcodeOut & BOTTOM) { // point is below the clip window
				x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
				y = ymin;
			}
			else if (outcodeOut & RIGHT) {  // point is to the right of clip window
				y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
				x = xmax;
			}
			else if (outcodeOut & LEFT) {   // point is to the left of clip window
				y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
				x = xmin;
			}
			
			// Now we move outside point to intersection point to clip
			// and get ready for next pass.
			if (outcodeOut == outcode0) {
				x0 = x;
				y0 = y;
				outcode0 = ComputeOutCode(x0, y0, xmin, xmax, ymin, ymax);
			}
			else {
				x1 = x;
				y1 = y;
				outcode1 = ComputeOutCode(x1, y1, xmin, xmax, ymin, ymax);
			}
		}
	}
	return accept;
}

bool LineString::intersects(const Envelope& rect)  const
{
	double xmin = rect.getMinX();
	double xmax = rect.getMaxX();
	double ymin = rect.getMinY();
	double ymax = rect.getMaxY();

	for (size_t i = 1; i < points.size(); ++i)
		if (intersectTest(points[i - 1].getX(), points[i - 1].getY(), points[i].getX(), points[i].getY(), xmin, xmax, ymin, ymax))
			return true;
	return false;
}

void LineString::draw()  const
{
	glBegin(GL_LINE_STRIP);
	for (size_t i = 0; i < points.size(); ++i)
		glVertex2d(points[i].getX(), points[i].getY());
	glEnd();
}

void LineString::print() const
{
	cout << "LineString(";
	for (size_t i = 0; i < points.size(); ++i) {
		if (i != 0)
			cout << ", ";
		cout << points[i].getX() << " " << points[i].getY();
	}
	cout << ")";
}

/*
 * Polygon
 */
double Polygon::distance(const Polygon* polygon) const
{
	return min(exteriorRing.distance(polygon), polygon->getExteriorRing().distance(this));
}

bool Polygon::intersects(const Envelope& rect)  const
{
	//cout << "to be implemented: Polygon::intersects(const Envelope& box)\n";
	if (this->envelope.intersect(rect))
	{
		if(this->exteriorRing.intersects(rect))
			return true;
		//��������contains rect
		else
		{
			//��ȡenvelope��һ���жϣ������ڲ������ⲿ���ڶ�����ڲ�˵���ཻ
			double xmin = rect.getMinX();
			double ymin = rect.getMinY();
			
			Point p = Point(xmin, ymin);
			
			if (p.bInPolygon(this) == false)
				return false;
			else
				return true;
			

		}
	}
	return false;
}

void Polygon::draw() const
{
	exteriorRing.draw();
}

void Polygon::print() const
{
	cout << "Polygon(";
	for (size_t i = 0; i < exteriorRing.numPoints(); ++i) {
		if (i != 0)
			cout << ", ";
		Point p = exteriorRing.getPointN(i);
		cout << p.getX() << " " << p.getY();
	}
	cout << ")";
}
	
}
