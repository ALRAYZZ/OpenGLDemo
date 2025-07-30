#include <tgaimage.h>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>


constexpr TGAColor white = { 255, 255, 255, 255 };
constexpr TGAColor green = { 0, 255, 0, 255 };
constexpr TGAColor red = { 0, 0, 255, 255 };
constexpr TGAColor blue = { 255, 128, 64, 255 };
constexpr TGAColor yellow = { 0, 200, 255, 255 };

void line(int ax, int ay, int bx, int by, TGAImage& frameBuffer, TGAColor color)
{
	bool steep = std::abs(ax - bx) < std::abs(ay - by);
	if (steep) // if the line is steep, we transpose the coordinates
	{
		std::swap(ax, ay);
		std::swap(bx, by);
	}

	if (ax > bx)
	{
		std::swap(ax, bx);
		std::swap(ay, by);
	}

	float y = ay;
	float ierror = 0;
	for (int x = ax; x <= bx; x++)
	{
		float t = (x - ax) / static_cast<float>(bx - ax);
		int y = std::round(ay + (by - ay) * t);
		if (steep)
		{
			frameBuffer.set(y, x, color); // if steep, we swap x and y
		}
		else
		{
			frameBuffer.set(x, y, color);
		}
		ierror += 2 * std::abs(by - ay); // calculate the error based on the slope
		if (ierror > bx - ax)
		{
			y += by > ay ? 1 : -1;
			ierror -= 2 * (bx - ax);
		}
	}
}

struct Vec3
{
	float x, y, z;
};

bool load_obj(const std::string& filename, std::vector<Vec3>& vertices, std::vector<std::vector<int>>& faces)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cerr << "Error opening .obj file: " << filename << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream iss(line); // String stream parses the lines and uses whitespace to separate words
		std::string type;
		iss >> type; // Extracting the first word to determine the type of line

		if (type == "v") // vertex
		{
			Vec3 vertex;
			iss >> vertex.x >> vertex.y >> vertex.z; // Populating the struct Vec3
			vertices.push_back(vertex);
		}
		else if (type == "f") // face
		{
			std::vector<int> face;
			std::string vertex;
			while (iss >> vertex)
			{
				// Parse the vertex index (before first slash)
				int vertexIndex = std::stoi(vertex.substr(0, vertex.find('/'))) - 1; // OBJ indices are 1-based, convert to 0-based
				face.push_back(vertexIndex);
			}
			if (face.size() >= 3) // Only process faces with at least 3 vertices
			{
				faces.push_back(face);
			}
		}
	}
	file.close();
	return true;
}

int main(int argc, char** argv)
{
	auto start = std::chrono::high_resolution_clock::now();

	constexpr int width = 64;
	constexpr int height = 64;
	TGAImage frameBuffer(width, height, TGAImage::RGB);

	// Load the OBJ file
	std::vector<Vec3> vertices;


	int ax = 7, ay = 3;
	int bx = 12, by = 37;
	int cx = 62, cy = 53;

	std::srand(std::time({}));
	for (int i = 0; i < (1 << 24); i++)
	{
		int ax = rand() % width, ay = rand() % height;
		int bx = rand() % width, by = rand() % height;
		line(ax, ay, bx, by, frameBuffer, {
			static_cast<std::uint8_t>(rand() % 255),
			static_cast<std::uint8_t>(rand() % 255),
			static_cast<std::uint8_t>(rand() % 255),
			static_cast<std::uint8_t>(rand() % 255),
			static_cast<std::uint8_t>(rand() % 255) 
		});
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;

	std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;


	frameBuffer.write_tga_file("frameBuffer.tga");

	return 0;
}