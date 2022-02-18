#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shape.h"

void Shape::OpenSTL(std::unique_ptr<std::string> stlPath)
{
    std::string line;
    std::string solidName;
    std::string strTemp;
    std::ifstream stream(*stlPath);
    vector<std::string> tokens;
    int vID;
    // Open file stream
    if (stream.is_open()) 
    {
        // Get name from first line
        std::getline(stream, line);
        std::istringstream sstream(line);
        sstream >> strTemp;
        sstream >> strTemp;
        solidName = strTemp;
        
        // Read in Facet data
        while (stream.eof() == false)
        {   
            std::getline(stream, line);
            tokens.clear();
            std::istringstream facetstream(line);
            while (facetstream)
            {
                facetstream >> strTemp;
                tokens.emplace_back(strTemp);
            }
            if (tokens[0] == "endsolid")
            {
                // End reading loop if first token in line = endsolid"
                break;
            }
            if (tokens[0] == "facet")
            {
                facetTemp.ijk[0] = StrToFloat(tokens[2]);
                facetTemp.ijk[1] = StrToFloat(tokens[3]);
                facetTemp.ijk[2] = StrToFloat(tokens[4]);
                vID = 0;
            }
            if (tokens[0] == "vertex")
            {
                if (vID == 0)
                {
                    facetTemp.vtx[0].x = StrToFloat(tokens[1]);
                    facetTemp.vtx[0].y = StrToFloat(tokens[2]);
                    facetTemp.vtx[0].z = StrToFloat(tokens[3]);
                }
                if (vID == 1)
                {
                    facetTemp.vtx[1].x = StrToFloat(tokens[1]);
                    facetTemp.vtx[1].y = StrToFloat(tokens[2]);
                    facetTemp.vtx[1].z = StrToFloat(tokens[3]);
                }
                if (vID == 2)
                {
                    facetTemp.vtx[2].x = StrToFloat(tokens[1]);
                    facetTemp.vtx[2].y = StrToFloat(tokens[2]);
                    facetTemp.vtx[2].z = StrToFloat(tokens[3]);
                }
                vID++;
            }
            if (tokens[0] == "endfacet")
            {
                facets.emplace_back(facetTemp);
            }
        }
        // Calculate geometric extent of solid
        minxyz = facets[0].vtx[0];
        maxxyz = facets[0].vtx[0];
        for (auto f : facets)
        {
            for (int p = 0; p < 3; p++)	//count points of vertex
            {
            	for (int i = 0; i < 3; i++)	//count x, y, z element
            	{
	                if (f.vtx[p][i] < minxyz[i]) minxyz[i] = f.vtx[p][i];
	                if (f.vtx[p][i] > maxxyz[i]) maxxyz[i] = f.vtx[p][i];
				}
			}
        }
        long a1 = facets.size();
        long a2 = a1;
    }
}

// Helper functions
int Shape::StrToInt(std::string strNum)
{
  return std::stoi(strNum);
}

long Shape::StrToLong(std::string strNum)
{
  const char *c = strNum.c_str();
  char *stopstring;
  return strtol(c, &stopstring, 10);
}

double Shape::StrToDouble(std::string strNum)
{
  const char *c = strNum.c_str();
  char *stopstring;
  return strtod(c, &stopstring);
}

float Shape::StrToFloat(std::string strNum)
{
  const char *c = strNum.c_str();
  char *stopstring;
  return strtof(c, &stopstring);
}
