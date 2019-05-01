
#include <SFML/Graphics.hpp>
#include "voronoi.h"
#include <iostream>
#include "Fortunes/Data Structures/BinTree.h"
#include "Bowyer-Watson/Delunay.h"
#include "Bowyer-Watson/delaunay.hpp"
#include "Fortunes/Data Structures/DCEL.h"
#include <math.h> 
#include <chrono>

vector<sf::CircleShape> circumPoints;
static const int MINSIZE = 300; //55
static const int MAXSIZE = 400; //390
static const int SEEDS = 100;


class Polygon : public sf::Drawable, public sf::Transformable
{
public:
	Polygon()
	{
		m_center.setRadius(3);
	}
	void addTexture(sf::Texture texture)
	{
		this->m_texture = texture;
	}
	std::vector<sf::Vertex> m_vertices;
	sf::CircleShape m_center;
	sf::PrimitiveType m_type = sf::Triangles;
private:
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		// apply the entity's transform -- combine it with the one that was passed by the caller
		states.transform *= getTransform(); // getTransform() is defined by sf::Transformable

		// apply the texture
		//states.texture = &m_texture;

		// you may also override states.shader or states.blendMode if you want
		// draw the vertex array
		target.draw(m_vertices.data(), m_vertices.size(), m_type, states);

		//target.draw(m_center,states);
		//target.draw(&m_site, 1, sf::Points, states);
	}
	sf::Transform m_transformation;
	sf::Texture m_texture;
};
pair<sf::Vector2f, float> normalize(sf::Vector2f vector);
void lineIntersection(sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, sf::Vector2f p4, sf::Vector2f& output);
int outsideSquare(sf::Vector2f& vector);
bool lineLine(sf::Vector2f& output, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
sf::Vector2f lineRect(sf::Vector2f v1, sf::Vector2f v2, float rx, float ry, float rw, float rh);

bool LiangBarsky(double edgeLeft, double edgeRight, double edgeBottom, double edgeTop,   // Define the x/y clipping values for the border.
	sf::Vector2f src0, sf::Vector2f src1,                 // Define the start and end points of the line.
	sf::Vector2f &clip0, sf::Vector2f &clip1);        // The output values, so declare these outside.

bool LiangBarsky(double edgeLeft, double edgeRight, double edgeBottom, double edgeTop,   // Define the x/y clipping values for the border.
	double x0src, double y0src, double x1src, double y1src,                 // Define the start and end points of the line.
	double &x0clip, double &y0clip, double &x1clip, double &y1clip);         // The output values, so declare these outside.



struct VoronoiEdge
{
	VoronoiEdge(sf::Vector2f _v1, sf::Vector2f _v2, sf::Vector2f* _site) : v1{ _v1 }, v2{ _v2 }, site{ _site }{};
	sf::Vector2f v1;
	sf::Vector2f v2;
	sf::Vector2f* site;
};

struct VoronoiCell
{
	sf::Vector2f* site;
	std::vector<VoronoiEdge*> edges;

	VoronoiCell(sf::Vector2f* sitePos) : site{ sitePos } {};
};

 sf::Vector2f CalculateCircleCenter(sf::Vector2f* p1, sf::Vector2f* p2, sf::Vector2f* p3)
{
	 sf::Vector2f center;

	 const float ax = p2->x - p1->x;
	 const float ay = p2->y - p1->y;
	 const float bx = p3->x - p1->x;
	 const float by = p3->y - p1->y;

	 const float m = p2->x * p2->x - p1->x * p1->x + p2->y * p2->y - p1->y * p1->y;
	 const float u = p3->x * p3->x - p1->x * p1->x + p3->y * p3->y - p1->y * p1->y;
	 const float s = 1. / (2. * (ax * by - ay * bx));

	center.x = ((p3->y - p1->y) * m + (p1->y - p2->y) * u) * s;
	center.y = ((p1->x - p3->x) * m + (p2->x - p1->x) * u) * s;

	 /*const float dx = v1->x - this->circle.x;
	 const float dy = v1->y - this->circle.y;
	 this->circle.radius = dx * dx + dy * dy;*/

	if (isinf(s))
	{
		int lul = 0;
	}

	/*float ma = (p2->y - p1->y) / (p2->x - p1->x);
	float mb = (p3->y - p2->y) / (p3->x - p2->x);

	center.x = (ma * mb * (p1->y - p3->y) + mb * (p1->x + p2->x) - ma * (p2->x + p3->x)) / (2 * (mb - ma));

	center.y = (-1 / ma) * (center.x - (p1->x + p2->x) / 2) + (p1->y + p2->y) / 2;
*/
	return center;
}

void addVoronoiEdge(HALF_EDGE::HE_Edge* e, sf::Vector2f center, std::vector<VoronoiEdge*>& voronoiEdges)
{
	//Ignore if this edge has no neighboring triangle
	if (e->twin == nullptr)
	{
		return;
	}

	//Calculate the circumcenter of the neighbor
	HALF_EDGE::HE_Edge* eNeighbor = e->twin;

	sf::Vector2f* v1 = eNeighbor->vert->point;
	sf::Vector2f* v2 = eNeighbor->next->vert->point;
	sf::Vector2f* v3 = eNeighbor->next->next->vert->point;

	sf::Vector2f voronoiVertexNeighbor = eNeighbor->face->circumCenter;
	//if (outsideSquare(voronoiVertexNeighbor) != 0 && outsideSquare(center) != 0)
	//{
	//	return;
	//}

	//if (outsideSquare(center) != 0)
	//{
	//	return;
	//	sf::Vector2f result = lineRect(center, voronoiVertexNeighbor, float(MINSIZE), float(MINSIZE),
	//		float(MAXSIZE + MINSIZE), float(MAXSIZE + MINSIZE));
	//	if (result.x != 0 || result.y != 0)
	//	{
	//		center = result;
	//		sf::CircleShape point;
	//		point.setRadius(4);
	//		point.setPosition(result - sf::Vector2f(4,4));
	//		point.setFillColor({ 0,0,255 });
	//		circumPoints.push_back(point);
	//	}
	//}
	//
	//if (outsideSquare(voronoiVertexNeighbor))
	//{
	//	return;
	//	sf::Vector2f result = lineRect(center, voronoiVertexNeighbor, float(MINSIZE), float(MINSIZE),
	//		float(MAXSIZE + MINSIZE), float(MAXSIZE + MINSIZE));
	//	if (result.x != 0 || result.y != 0)
	//	{
	//		voronoiVertexNeighbor = result;
	//		sf::CircleShape point;
	//		point.setRadius(4);
	//		point.setPosition(result - sf::Vector2f(4, 4));
	//		point.setFillColor({ 0,0,255 });
	//		circumPoints.push_back(point);
	//	}
	//}
	//Create a new voronoi edge between the voronoi vertices
	voronoiEdges.push_back(new 
		VoronoiEdge(center, voronoiVertexNeighbor, e->vert->point));
}

int main()
{
	Voronoi* vdg;
	vector<VoronoiPoint*> ver;
	vector<VEdge> edges;

	//INITIALIZATION
	for (vector<VoronoiPoint*>::iterator i = ver.begin(); i != ver.end(); i++)
		delete((*i));
	ver.clear();
	edges.clear();

	float center = float(MINSIZE + (MAXSIZE / 2));
	//Create Seed Points
	vector<DVertex*> BWpoints;
	{
		for (size_t i = 0; i < SEEDS; i++)
		{
			float x = float(rand() % MAXSIZE + MINSIZE);
			float y = float(rand() % MAXSIZE + MINSIZE);
			BWpoints.push_back(new DVertex(x, y, i));
			ver.push_back(new VoronoiPoint(x, y));
		}

		{
			float minMax = center;
			sf::Vector2f starTop = { center, center - minMax };
			sf::Vector2f starLeft = { center - minMax, center };
			sf::Vector2f starRight = { center + minMax, center };
			sf::Vector2f starBottom = { center, center + minMax };

			BWpoints.push_back(new DVertex(starTop, BWpoints.size()));
			BWpoints.push_back(new DVertex(starLeft, BWpoints.size()));
			BWpoints.push_back(new DVertex(starRight, BWpoints.size()));
			BWpoints.push_back(new DVertex(starBottom, BWpoints.size()));

			BWpoints.push_back(new DVertex(MINSIZE, MINSIZE, BWpoints.size()));
			BWpoints.push_back(new DVertex(MAXSIZE + MINSIZE, MINSIZE, BWpoints.size()));
			BWpoints.push_back(new DVertex(MAXSIZE + MINSIZE, MAXSIZE + MINSIZE, BWpoints.size()));
			BWpoints.push_back(new DVertex(MINSIZE, MAXSIZE + MINSIZE, BWpoints.size()));

			ver.push_back(new VoronoiPoint(double(MINSIZE), double(MINSIZE)));
			ver.push_back(new VoronoiPoint(double(MAXSIZE + MINSIZE), double(MINSIZE)));
			ver.push_back(new VoronoiPoint(double(MAXSIZE + MINSIZE), double(MAXSIZE + MINSIZE)));
			ver.push_back(new VoronoiPoint(double(MINSIZE), double(MAXSIZE + MINSIZE)));

			ver.push_back(new VoronoiPoint(starTop.x, starTop.y));
			ver.push_back(new VoronoiPoint(starLeft.x, starLeft.y));
			ver.push_back(new VoronoiPoint(starRight.x, starRight.y));
			ver.push_back(new VoronoiPoint(starBottom.x, starBottom.y));
		}
	}

	/////////FORTUNES ALGORITHM///////////////////
	vdg = new Voronoi();
	std::cout << "FORTUNES ALGORITHM\n";
	auto start = std::chrono::system_clock::now();

	edges = vdg->ComputeVoronoiGraph(ver, MINSIZE, MINSIZE + MAXSIZE);

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
	delete vdg;
	/////////////////////////////////////////////
	std::cout << "\nBOWYER-WATSON TRIANGULATION ALGORITHM\n";
	start = std::chrono::system_clock::now();
	Delunay triangulation;
	const std::vector<Triangle> triangles = triangulation.Triangulate(BWpoints);
	end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
	std::cout << triangles.size() << " triangles generated\n";

	start = std::chrono::system_clock::now();

	//for (auto& vert : triangulation.getSuperTriangle())
	//{
	//	vert->arrayIndex = BWpoints.size();
	//	BWpoints.push_back(vert);
	//}

	std::vector<pair<unsigned int, unsigned int>> EdgeIndex;
	
	for (const Triangle &triangle : triangles)
	{
		EdgeIndex.push_back(make_pair(triangle.v1->arrayIndex, triangle.v2->arrayIndex));
		EdgeIndex.push_back(make_pair(triangle.v2->arrayIndex, triangle.v3->arrayIndex));
		EdgeIndex.push_back(make_pair(triangle.v3->arrayIndex, triangle.v1->arrayIndex));
	}

	map< pair<unsigned int, unsigned int>, HALF_EDGE::HE_Edge* > Edges;
	std::vector<HALF_EDGE::HE_Vertex*> vertexList;
	for (auto &point : BWpoints)
	{
		HALF_EDGE::HE_Vertex* vertex = new HALF_EDGE::HE_Vertex();
		vertex->point = point;
		vertexList.push_back(vertex);
	}
	std::vector<HALF_EDGE::HE_Face*> faceList;

	for (int i = 0; i < triangles.size(); i++)
	{
		const int offset = i * 3;
		HALF_EDGE::HE_Face* tempF = new HALF_EDGE::HE_Face();
		tempF->circumCenter = sf::Vector2f(triangles[i].circle.x, triangles[i].circle.y);
		tempF->radius = triangles[i].circle.radius;
		for (size_t k = 0; k < 3; k++)
		{
			const int location = offset + k;

			pair<unsigned int, unsigned int> memEdges(EdgeIndex[location]);
			Edges[memEdges] = new HALF_EDGE::HE_Edge();
			Edges[memEdges]->face = tempF;

			Edges[memEdges]->vert = vertexList[memEdges.first];
			Edges[memEdges]->vert->edge = Edges[EdgeIndex[offset]];
		}

		tempF->edge = Edges[EdgeIndex[offset]];

		for (size_t k = 0; k < 3; k++)
		{
			const int location = offset + k;
			Edges[EdgeIndex[location]]->setNext(
				Edges[EdgeIndex[offset + (k + 1) % 3]]);
			
			pair<unsigned int, unsigned int> otherPair = make_pair(EdgeIndex[location].second, EdgeIndex[location].first);
			if (Edges.find(otherPair) != Edges.end())
			{
				Edges[EdgeIndex[location]]->setTwin(Edges[otherPair]);
			}

		}
		faceList.push_back(tempF);
	}
	
	std::vector <HALF_EDGE::HE_Vertex*> faceEdgePoints;
	for (auto &face : faceList)
	{
		HALF_EDGE::getFaceVertices(faceEdgePoints, face);
	}

	std::vector <HALF_EDGE::HE_Edge*> faceEdges;
	for (auto &face : faceList)
	{
		HALF_EDGE::getFaceEdges(faceEdges, face);
	}

	std::vector<VoronoiEdge*> allVoronoiEdges;
	for (size_t i = 0; i < faceList.size(); i++)
	{
		HALF_EDGE::HE_Edge* e1 = faceList[i]->edge;
		HALF_EDGE::HE_Edge* e2 = e1->next;
		HALF_EDGE::HE_Edge* e3 = e2->next;
 
		sf::Vector2f* v1 = e1->vert->point;
		sf::Vector2f* v2 = e2->vert->point;
		sf::Vector2f* v3 = e3->vert->point;

		sf::Vector2f voronoiVertex = faceList[i]->circumCenter;

		addVoronoiEdge(e1, voronoiVertex, allVoronoiEdges);
		addVoronoiEdge(e2, voronoiVertex, allVoronoiEdges);
		addVoronoiEdge(e3, voronoiVertex, allVoronoiEdges);
	}	

	end = std::chrono::system_clock::now();
	std::cout << "\nTRIANGULATION & VORONOI GENERATION\n";
	elapsed_seconds = end - start + elapsed_seconds;
	std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

	std::vector<VoronoiCell> voronoiCells;
	for (int i = 0; i < allVoronoiEdges.size(); i++)
	{
		VoronoiEdge* e = allVoronoiEdges[i];

		int index = -1;
		//Find the position in the list of all cells that includes this site
		for (int k = 0; k < voronoiCells.size(); k++)
		{
			if (e->site == voronoiCells[k].site)
			{
				index = k;
				break;
			}
		}

		int cellPos = index;
		//No cell was found so we need to create a new cell
		if (cellPos == -1)
		{
			VoronoiCell newCell(e->site);

			voronoiCells.push_back(newCell);

			voronoiCells.back().edges.push_back(e);
		}
		else
		{
			voronoiCells[cellPos].edges.push_back(e);
		}
	}

	//RENDERING

	//VORONOI CELLS
	//std::vector<sf::Vertex*> polygons;
	std::vector<Polygon> polygons;
	//polygons.resize(voronoiCells.size());
	int count = 0;
	bool pST = false;
	vector<sf::CircleShape> seedPoints;
	for (auto &cell : voronoiCells)
	{
		
		sf::Color color(rand() % 200, rand() % 200, rand() % 200);
		pST = false;
		
		#if 0
		for (int i = 0; i < cell.edges.size(); i++)
		{
			if (!(cell.site.x < MAXSIZE+MINSIZE && cell.site.x > MINSIZE) &&
				!(cell.site.y < MAXSIZE + MINSIZE && cell.site.y > MINSIZE))
			{
				pST = true;
				break;
			}
			if (!(cell.edges[i]->site.x < MAXSIZE + MINSIZE && cell.edges[i]->site.x > MINSIZE) &&
				!(cell.edges[i]->site.y < MAXSIZE + MINSIZE && cell.edges[i]->site.y > MINSIZE))
			{
				pST = true;
				break;
			}

			if (!(cell.edges[i]->v1.x < MAXSIZE + MINSIZE && cell.edges[i]->v1.x > MINSIZE) &&
				!(cell.edges[i]->v1.y < MAXSIZE + MINSIZE && cell.edges[i]->v1.y > MINSIZE))
			{
				pST = true;
				break;
			}

			if (!(cell.edges[i]->v2.x < MAXSIZE + MINSIZE && cell.edges[i]->v2.x > MINSIZE) &&
				!(cell.edges[i]->v2.y < MAXSIZE + MINSIZE && cell.edges[i]->v2.y > MINSIZE))
			{
				pST = true;
				break;
			}
		}
		if (pST)
			continue;
		#endif

		Polygon polygon;
		sf::Vector2f polyCenter;
		int vertCount = 0;
		for (int i = 0; i < cell.edges.size(); i++)
		{
			sf::Vector2f p3 = cell.edges[i]->v1;
			sf::Vector2f p2 = cell.edges[i]->v2;
			sf::Vertex* test = new sf::Vertex[3];
			test[0] = *cell.site;
			test[0].color = color;
			test[1] = p2;
			test[1].color = color;
			test[2] = p3;
			test[2].color = color;

			sf::Vector2f v1Result;
			sf::Vector2f v2Result;
			bool draw = true;
			float halfwidth = float(MAXSIZE / 2)+1.0f;
			sf::Vector2f boundries[4];
			boundries[0] = { center - halfwidth, center - halfwidth };
			boundries[1] = { center - halfwidth, center + halfwidth };
			boundries[2] = { center + halfwidth, center + halfwidth };
			boundries[3] = { center + halfwidth, center - halfwidth };

			#if 0
			for (int k = 0; k < 4; k++)
			{
				sf::Vector2f result;
				lineIntersection(test[0].position, test[1].position, boundries[k], boundries[(k + 1) % 4], result);
				if (result.x != 0 && result.y != 0)
				{
					test[1].position = result;
					break;
				}
			}
			for (int k = 0; k < 4; k++)
			{
				sf::Vector2f result;
				lineIntersection(test[0].position, test[2].position, boundries[k], boundries[(k + 1) % 4], result);
				if (result.x != 0 && result.y != 0)
				{
					test[2].position = result;
					break;
				}
			}
			#endif	

			#if 1
			//for (int k = 1; k < 3; k++)
			{
				/*if (outsideSquare(test[1].position) && outsideSquare(test[2].position))
					continue;*/


				if (LiangBarsky(center-halfwidth, center+halfwidth,
					center+halfwidth, center-halfwidth,
					test[1].position, test[2].position, v1Result, v2Result))
				{

					if (v1Result != test[1].position)
					{
						sf::Vector2f v1ResultN;
						sf::Vector2f v2ResultN;
						if (LiangBarsky(center - halfwidth, center + halfwidth,
							center + halfwidth, center - halfwidth,
							test[0].position, test[1].position, v1ResultN, v2ResultN))
						{
							sf::Vertex nVertex;
							nVertex = *cell.site;
							nVertex.color = color;
							polygon.m_vertices.push_back(nVertex);

							nVertex = v1Result;
							nVertex.color = color;
							polygon.m_vertices.push_back(nVertex);

							nVertex = v2ResultN;
							nVertex.color = color;
							polygon.m_vertices.push_back(nVertex);

							polyCenter += v1Result;
							polyCenter += v2ResultN;
							vertCount += 2;
						}

						
					}
					if (v2Result != test[2].position)
					{
						sf::Vector2f v1ResultN;
						sf::Vector2f v2ResultN;
						if (LiangBarsky(center - halfwidth, center + halfwidth,
							center + halfwidth, center - halfwidth,
							test[0].position, test[2].position, v1ResultN, v2ResultN))
						{
							sf::Vertex nVertex;
							nVertex = *cell.site;
							nVertex.color = color;
							polygon.m_vertices.push_back(nVertex);

							nVertex = v2Result;
							nVertex.color = color;
							polygon.m_vertices.push_back(nVertex);

							nVertex = v2ResultN;
							nVertex.color = color;
							polygon.m_vertices.push_back(nVertex);

							polyCenter += v2Result;
							polyCenter += v2ResultN;
							vertCount += 2;
						}

						
					}
					

					test[1].position = v1Result;
					test[2].position = v2Result;
					/*if (outsideSquare(v1Result))
						test[k].position = v2Result;*/
				}
				else
				{
					bool passed = false;
					if (LiangBarsky(center - halfwidth, center + halfwidth,
						center + halfwidth, center - halfwidth,
						test[0].position, test[2].position, v1Result, v2Result))
					{
						test[0].position = v1Result;
						test[2].position = v2Result;
						passed = true;
					}
					if (LiangBarsky(center - halfwidth, center + halfwidth,
						center + halfwidth, center - halfwidth,
						test[0].position, test[1].position, v1Result, v2Result))
					{
						test[0].position = v1Result;
						test[1].position = v2Result;
						passed = true;
					}
					if (!passed)
						draw = false;
					/*test[1].position = cell.edges[(i-1)% cell.edges.size()]->v2;
					test[2].position = cell.edges[(i+1) % cell.edges.size()]->v1;*/
					//draw = false;
					
				}

			}
			#endif
			if (draw)
			{
				polyCenter += test[1].position;
				polyCenter += test[2].position;
				vertCount += 2;
				polygon.m_vertices.push_back(test[0]);
				polygon.m_vertices.push_back(test[1]);
				polygon.m_vertices.push_back(test[2]);
			}
		}
		polyCenter.x /= vertCount;
		polyCenter.y /= vertCount;
		polygon.m_center.setPosition(polyCenter);
		polygon.setOrigin(polygon.m_center.getPosition());
		polygon.setPosition(polygon.m_center.getPosition());

		polygons.push_back(polygon);
		count++;
		/*break;*/
	}

	
	std::vector<sf::Vertex*> lines;
	{

	
	//TRIANGULATION
	/*int stride = 3;
	bool once = true;
	for (size_t i = 0; i < triangles.size(); i++)
	{
		int offset = i * stride;
		for (size_t k = 0; k < 3; k++)
		{
			sf::Vector2f* start = faceEdgePoints[offset + k]->point;
			sf::Vector2f* end = faceEdgePoints[offset + (k + 1) % 3]->point;

			sf::Vertex* test = new sf::Vertex[2];
			test[0] = sf::Vector2f(start->x, start->y);
			test[1] = sf::Vector2f(end->x, end->y);
			lines.push_back(test);
			lines.back()[0].color = sf::Color(0, 255, 0);
			lines.back()[1].color = sf::Color(0, 255, 0);
		}
	}*/

	//FORTUNES VORONOI EDGES
	/*for (int i = 0; i < edges.size(); i++)
	{
		sf::Vertex* test = new sf::Vertex[2];
		test[0] = sf::Vector2f(edges[i].VertexA.x, edges[i].VertexA.y);
		test[1] = sf::Vector2f(edges[i].VertexB.x, edges[i].VertexB.y);
		lines.push_back(test);
	}*/


	
	//for (size_t i = 0; i < faceList.size(); i++)
	//{
	//	//if (faceList[i]->radius < MAXSIZE)
	//	{
	//		/*sf::CircleShape point;
	//		point.setRadius(faceList[i]->radius);
	//		point.setPosition(faceList[i]->circumCenter - sf::Vector2f(faceList[i]->radius, faceList[i]->radius));
	//		point.setOutlineThickness(1.0f);
	//		point.setOutlineColor({ 255,0,255 });
	//		point.setFillColor(sf::Color::Transparent);
	//		circumPoints.push_back(point);*/

	//		sf::CircleShape point;
	//		point.setRadius(2);
	//		point.setPosition(faceList[i]->circumCenter - sf::Vector2f(2, 2));
	//		point.setFillColor({ 0,0,255 });
	//		circumPoints.push_back(point);
	//	}
	//}

	}

	
	sf::RectangleShape rect;
	rect.setFillColor(sf::Color::Transparent);
	rect.setOutlineThickness(2);
	rect.setOutlineColor({ 255,255,255 });
	rect.setSize({ float(MAXSIZE),float(MAXSIZE) });
	rect.setPosition({ float(MINSIZE), float(MINSIZE)  });

	std::vector<sf::Vector2f> distances;
	for (auto &polygon : polygons)
	{

		distances.push_back(normalize(polygon.m_center.getPosition() - sf::Vector2f(center, center)).first);
	}

	/*for (auto seeds : BWpoints)
	{
		sf::CircleShape point;
				point.setRadius(2);
				point.setPosition(seeds->getCoordinates() - sf::Vector2f(2, 2));
				point.setFillColor({ 0,0,255 });
				seedPoints.push_back(point);
	}*/

	double dt = 1/60;
	double timer = 0;
	bool seperate = false;
	sf::RenderWindow window(sf::VideoMode(1000, 1000), "SFML works!");
	while (window.isOpen())
	{
		start = std::chrono::system_clock::now();
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
				case sf::Event::Closed:
					window.close();
					break;
				case sf::Event::KeyPressed:
					if (event.key.code == sf::Keyboard::Escape)
						window.close();
					break;
			}
		}

		window.clear();
		
		if(seperate)
			for (int i = 0; i < polygons.size(); i++)
			{
				polygons[i].move(distances[i] * 0.01f);
			}

		for(int i = 0; i < polygons.size(); i++)
		{
			window.draw(polygons[i]);
		}

		
		for(auto line : lines)
			window.draw(line, 2, sf::Lines);

		for (auto point : seedPoints)
			window.draw(point);
		
		/*for (auto point : circumPoints)
		{
			window.draw(point);
		}*/
		if (!seperate)
			window.draw(rect);
		window.display();

		std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
		dt = elapsed_seconds.count();
		timer += dt;
		(timer > 4) ? seperate = true : NULL;
			
	}

	return 0; 
}



void lineIntersection(sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, sf::Vector2f p4, sf::Vector2f& output)
{
	double epsilon = std::numeric_limits<double>::epsilon();
	// Store the values for fast access and easy
	// equations-to-code conversion
	float x1 = p1.x, x2 = p2.x, x3 = p3.x, x4 = p4.x;
	float y1 = p1.y, y2 = p2.y, y3 = p3.y, y4 = p4.y;

	float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	// If d is zero, there is no intersection
	if (d == 0) return;

	// Get the x and y
	float pre = (x1*y2 - y1 * x2), post = (x3*y4 - y3 * x4);
	float x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
	float y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

	// Check if the x and y coordinates are within both lines
	if (x < (min(x1, x2) - epsilon) || x >(max(x1, x2) + epsilon) ||
		x < (min(x3, x4) - epsilon) || x >(max(x3, x4) + epsilon))
		return;
	if (y < (min(y1, y2) - epsilon) || y >(max(y1, y2) + epsilon) ||
		y < (min(y3, y4) - epsilon) || y >(max(y3, y4) + epsilon))
		return;

	// Return the point of intersection
	output = sf::Vector2f(x,y);
}

pair<sf::Vector2f, float> normalize(sf::Vector2f vector)
{
	float mx = vector.x;
	float my = vector.y;

	float multiplier = 1.0f / sqrt(mx*mx + my * my);
	mx *= multiplier;
	my *= multiplier;

	return make_pair(sf::Vector2f(mx, my),multiplier);
}

int outsideSquare(sf::Vector2f& vector)
{
	int extra = 0;
	int result = 0;
	if (!(vector.x < MAXSIZE+ MINSIZE+ extra && vector.y < MAXSIZE+ MINSIZE+ extra))
		result = 1;
	else if (!(vector.x > MINSIZE- extra && vector.y > MINSIZE- extra))
		result = -1;

	return result;
}


// LINE/LINE
bool lineLine(sf::Vector2f& output, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) {

	// calculate the direction of the lines
	float uA = ((x4 - x3)*(y1 - y3) - (y4 - y3)*(x1 - x3)) / ((y4 - y3)*(x2 - x1) - (x4 - x3)*(y2 - y1));
	float uB = ((x2 - x1)*(y1 - y3) - (y2 - y1)*(x1 - x3)) / ((y4 - y3)*(x2 - x1) - (x4 - x3)*(y2 - y1));

	// if uA and uB are between 0-1, lines are colliding
	if (uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1) {

		// optionally, draw a circle where the lines meet
		float intersectionX = x1 + (uA * (x2 - x1));
		float intersectionY = y1 + (uA * (y2 - y1));
		output.x = intersectionX;
		output.y = intersectionY;
		return true;
	}
	return false;
}

// LINE/RECTANGLE
sf::Vector2f lineRect(sf::Vector2f v1, sf::Vector2f v2, float rx, float ry, float rw, float rh) {

	sf::Vector2f rectStart = { rx, ry };
	sf::Vector2f size = { rw, rh };
	sf::Vector2f result;
	// check if the line has hit any of the rectangle's sides
	// uses the Line/Line function below
	bool left = lineLine(result, v1.x, v1.y, v2.x, v2.y , rx, ry, rx, ry + rh);
	if (left)
		return result;

	bool right = lineLine(result,v1.x, v1.y, v2.x, v2.y, rx + rw, ry, rx + rw, ry + rh);
	if (right)
		return result;

	bool top = lineLine(result, v1.x, v1.y, v2.x, v2.y, rx, ry, rx + rw, ry);
	if (top)
		return result;

	bool bottom = lineLine(result, v1.x, v1.y, v2.x, v2.y, rx, ry + rh, rx + rw, ry + rh);
	if (bottom)
		return result;


	return sf::Vector2f();
}

#if 0

int x_intersect(int x1, int y1, int x2, int y2,
	int x3, int y3, int x4, int y4)
{
	int num = (x1*y2 - y1 * x2) * (x3 - x4) -
		(x1 - x2) * (x3*y4 - y3 * x4);
	int den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	return num / den;
}

// Returns y-value of point of intersectipn of 
// two lines 
int y_intersect(int x1, int y1, int x2, int y2,
	int x3, int y3, int x4, int y4)
{
	int num = (x1*y2 - y1 * x2) * (y3 - y4) -
		(y1 - y2) * (x3*y4 - y3 * x4);
	int den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	return num / den;
}

void clip(int poly_points[][2], int &poly_size,
	int x1, int y1, int x2, int y2)
{
	int new_points[MAX_POINTS][2], new_poly_size = 0;

	// (ix,iy),(kx,ky) are the co-ordinate values of 
	// the points 
	for (int i = 0; i < poly_size; i++)
	{
		// i and k form a line in polygon 
		int k = (i + 1) % poly_size;
		int ix = poly_points[i][0], iy = poly_points[i][1];
		int kx = poly_points[k][0], ky = poly_points[k][1];

		// Calculating position of first point 
		// w.r.t. clipper line 
		int i_pos = (x2 - x1) * (iy - y1) - (y2 - y1) * (ix - x1);

		// Calculating position of second point 
		// w.r.t. clipper line 
		int k_pos = (x2 - x1) * (ky - y1) - (y2 - y1) * (kx - x1);

		// Case 1 : When both points are inside 
		if (i_pos < 0 && k_pos < 0)
		{
			//Only second point is added 
			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 2: When only first point is outside 
		else if (i_pos >= 0 && k_pos < 0)
		{
			// Point of intersection with edge 
			// and the second point is added 
			new_points[new_poly_size][0] = x_intersect(x1,
				y1, x2, y2, ix, iy, kx, ky);
			new_points[new_poly_size][1] = y_intersect(x1,
				y1, x2, y2, ix, iy, kx, ky);
			new_poly_size++;

			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 3: When only second point is outside 
		else if (i_pos < 0 && k_pos >= 0)
		{
			//Only point of intersection with edge is added 
			new_points[new_poly_size][0] = x_intersect(x1,
				y1, x2, y2, ix, iy, kx, ky);
			new_points[new_poly_size][1] = y_intersect(x1,
				y1, x2, y2, ix, iy, kx, ky);
			new_poly_size++;
		}

		// Case 4: When both points are outside 
		else
		{
			//No points are added 
		}
	}

	// Copying new points into original array 
	// and changing the no. of vertices 
	poly_size = new_poly_size;
	for (int i = 0; i < poly_size; i++)
	{
		poly_points[i][0] = new_points[i][0];
		poly_points[i][1] = new_points[i][1];
	}
}

// Implements Sutherland�Hodgman algorithm 
void suthHodgClip(int poly_points[][2], int poly_size,
	int clipper_points[][2], int clipper_size)
{
	//i and k are two consecutive indexes 
	for (int i = 0; i < clipper_size; i++)
	{
		int k = (i + 1) % clipper_size;

		// We pass the current array of vertices, it's size 
		// and the end points of the selected clipper line 
		clip(poly_points, poly_size, clipper_points[i][0],
			clipper_points[i][1], clipper_points[k][0],
			clipper_points[k][1]);
	}

	// Printing vertices of clipped polygon 
	for (int i = 0; i < poly_size; i++)
		cout << '(' << poly_points[i][0] <<
		", " << poly_points[i][1] << ") ";
}

#endif

// Liang-Barsky function by Daniel White @ http://www.skytopia.com/project/articles/compsci/clipping.html
// This function inputs 8 numbers, and outputs 4 new numbers (plus a boolean value to say whether the clipped line is drawn at all).
//
bool LiangBarsky(double edgeLeft, double edgeRight, double edgeBottom, double edgeTop,   // Define the x/y clipping values for the border.
	sf::Vector2f src0, sf::Vector2f src1,                 // Define the start and end points of the line.
	sf::Vector2f &clip0, sf::Vector2f &clip1)         // The output values, so declare these outside.
{

	double t0 = 0.0;    double t1 = 1.0;
	double xdelta = src1.x - src0.x;
	double ydelta = src1.y - src0.y;
	double p, q, r;

	for (int edge = 0; edge < 4; edge++) {   // Traverse through left, right, bottom, top edges.
		if (edge == 0) { p = -xdelta;    q = -(edgeLeft - src0.x); }
		if (edge == 1) { p = xdelta;     q = (edgeRight - src0.x); }
		if (edge == 2) { p = ydelta;    q = (edgeBottom - src0.y); }
		if (edge == 3) { p = -ydelta;     q = -(edgeTop - src0.y); }
		r = q / p;
		if (p == 0 && q < 0) return false;   // Don't draw line at all. (parallel line outside)

		if (p < 0) {
			if (r > t1) return false;         // Don't draw line at all.
			else if (r > t0) t0 = r;            // Line is clipped!
		}
		else if (p > 0) {
			if (r < t0) return false;      // Don't draw line at all.
			else if (r < t1) t1 = r;         // Line is clipped!
		}
	}

	clip0 = { src0.x + float(t0 * xdelta) , src0.y + float(t0 * ydelta) };
	clip1 = { src0.x + float(t1 * xdelta) , src0.y + float(t1 * ydelta) };

	return true;        // (clipped) line is drawn
}


bool LiangBarsky(double edgeLeft, double edgeRight, double edgeBottom, double edgeTop,   // Define the x/y clipping values for the border.
	double x0src, double y0src, double x1src, double y1src,                 // Define the start and end points of the line.
	double &x0clip, double &y0clip, double &x1clip, double &y1clip)         // The output values, so declare these outside.
{

	double t0 = 0.0;    double t1 = 1.0;
	double xdelta = x1src - x0src;
	double ydelta = y1src - y0src;
	double p, q, r;

	for (int edge = 0; edge < 4; edge++) {   // Traverse through left, right, bottom, top edges.
		if (edge == 0) { p = -xdelta;    q = -(edgeLeft - x0src); }
		if (edge == 1) { p = xdelta;     q = (edgeRight - x0src); }
		if (edge == 2) { p = -ydelta;    q = (edgeBottom - y0src); }
		if (edge == 3) { p = ydelta;     q = -(edgeTop - y0src); }
		r = q / p;
		if (p == 0 && q < 0) return false;   // Don't draw line at all. (parallel line outside)

		if (p < 0) {
			if (r > t1) return false;         // Don't draw line at all.
			else if (r > t0) t0 = r;            // Line is clipped!
		}
		else if (p > 0) {
			if (r < t0) return false;      // Don't draw line at all.
			else if (r < t1) t1 = r;         // Line is clipped!
		}
	}

	x0clip = x0src + t0 * xdelta;
	y0clip = y0src + t0 * ydelta;
	x1clip = x0src + t1 * xdelta;
	y1clip = y0src + t1 * ydelta;

	return true;        // (clipped) line is drawn
}