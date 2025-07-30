#include <vector>
#include <geometry.h>

class Model
{
	std::vector<vec3> verts = {};
	std::vector<int> face_vert = {};

public:
	Model(const std::string filename);
	int nverts() const; // Number of vertices
	int nfaces() const; // Number of faces
	vec3 vert(const int i) const;
	vec3 vert(const int iface, const int nthvert) const;
};