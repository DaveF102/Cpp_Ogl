#ifndef SOLID_H
#define SOLID_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using std::vector;

// A Facet consists of 3 points in space and a normal vector.  These are read directly
//   from the .stl file
class Facet
{
public:
    // Constructor
	Facet() :
		fID { -1 },
		vtx { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f) },
		ijk { glm::vec3(0.0f, 0.0f, 1.0f) }
		{}
	
    // Getters and setters
	void SetfID(long id);

	//Public member variables
    long fID;
    glm::vec3 vtx[3];	//vertex data of triangular facet
    glm::vec3 ijk;		//normal vector of triangular facet

};

// A Shape is the exterior surface of a solid, consisting of Facets that are read from an stl file.
//   .stl (STereoLithography) files define facets per:
class Shape
{
public:
    // Constructor
    Shape() :
		minxyz { glm::vec3(0.0f, 0.0f, 0.0f) },
		maxxyz { glm::vec3(0.0f, 0.0f, 0.0f) }
		{}

    // Other functions
	void OpenSTL(std::unique_ptr<std::string> stlPath);
	int StrToInt(std::string strNum);
	long StrToLong(std::string strNum);
	double StrToDouble(std::string strNum);
	float StrToFloat(std::string strNum);
    
    // Public variablesms;
	Facet facetTemp;
	vector<Facet> facets;
	glm::vec3 minxyz;
	glm::vec3 maxxyz;
};

#endif