#include <fstream>
#include <sstream>
#include <model.h>


Model::Model(const std::string filename)
{
	std::ifstream file(filename);

	if (!file)
	{
		std::cerr << "Error opening .obj file: " << filename << std::endl;
		return;
	}

	std::string line;
	while (std::getline(file, line)) // Best practice to read line by line 
	{
		std::istringstream iss(line); // String stream parses the lines and uses whitespace to separate words
		char prefix;
		if (line.starts_with("v ")) // We are not using the string stream, so we can check the whitespaces.
		{
			iss >> prefix;
			vec3 vertex;
			for (int i = 0; i < 3; i++) // 3 iterations because we expect 3 coordinates per vertex
			{
				iss >> vertex[i]; // Populating the struct vec3
			}
			verts.push_back(vertex);
		}
		else if (line.starts_with("f ")) // Face
		{
			iss >> prefix;
			int vertexIndex, textureIndex, normalIndex;
			int vertexCount = 0;

			while (iss >> vertexIndex >> prefix >> textureIndex >> prefix >> normalIndex)
			{
				if (vertexIndex <= 0)
				{
					std::cerr << "Error: Invalid vertex index in face.\n";
					return;
				}
				face_vert.push_back(vertexIndex - 1); // OBJ indices are 1-based, convert to 0-based
				vertexCount++;
			}

			if (vertexCount != 3)
			{
				std::cerr << "Error: Only triangulated .obj files are supported.\n";
				return;
			}
		}
	}

	std::cerr << "# v# " << nverts() << " f# " << nfaces() << '\n';
}

// Accessor methods
int Model::nverts() const
{
	return static_cast<int>(verts.size());
}
int Model::nfaces() const
{
	return static_cast<int>(face_vert.size() / 3);
}

vec3 Model::vert(const int i) const
{
	return verts.at(i);
}

vec3 Model::vert(const int iface, const int nthvert) const
{
	return verts.at(face_vert.at(iface * 3 + nthvert));
}