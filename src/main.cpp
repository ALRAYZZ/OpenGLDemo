#include <tgaimage.h>
#include <numbers>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <geometry.h>
#include <model.h>
#include <algorithm>

constexpr int width = 800;
constexpr int height = 800;

// BGRA order
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

	int y = ay;
	int ierror = 0;
	for (int x = ax; x <= bx; x++)
	{
		if (steep)
		{
			frameBuffer.set(y, x, color);
		}
		else
		{
			frameBuffer.set(x, y, color);
		}

		ierror += 2 * std::abs(by - ay);
		if (ierror > bx - ax)
		{
			y += by > ay ? 1 : -1;
			ierror -= 2 * (bx - ax);
		}
	}
}

// Calculate signed area of triangle using determinant method
double signedTriangleArea(int ax, int ay, int bx, int by, int cx, int cy)
{
	return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) + (ay - cy) * (ax + cx));

}

void triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, TGAImage& zBuffer, TGAImage& frameBuffer, TGAColor color, const vec3& v0, const vec3& v1, const vec3& v2)
{
	// Backface culling: Calculating triangle normal
	vec3 edge1 = v1 - v0;
	vec3 edge2 = v2 - v0;
	vec3 normal = cross(edge1, edge2);
	
	// Camera direction assuming is at origin looking down the negative Z-axis
	vec3 cameraDir = { 0, 0, -1 };

	// If dot product is negative, triangle is facing away from camera
	// Based on the angle between the triangle normal and camera direction we can determine visibility
	if (normal * cameraDir >= 0)
	{
		return; // Backface culling: Skip triangle if facing away from camera
	}

	// Use bounding box approach to limit the area we need to scan
	int bbminx = std::max(0, std::min({ ax, bx, cx }));
	int bbminy = std::max(0, std::min({ ay, by, cy }));
	int bbmaxx = std::min(width - 1, std::max({ ax, bx, cx }));
	int bbmaxy = std::min(height - 1, std::max({ ay, by, cy }));

	// Calculate total triange area for barycentric coordinates
	double totalArea = signedTriangleArea(ax, ay, bx, by, cx, cy);
	if (std::abs(totalArea) < 1e-6) return; // Degenerate triangle, skip rendering


	for (int x = bbminx; x <= bbmaxx; x++)
	{
		for (int y = bbminy; y <= bbmaxy; y++)
		{
			// Calculate barycentric coordinates
			double alpha = signedTriangleArea(x, y, bx, by, cx, cy) / totalArea;
			double beta = signedTriangleArea(ax, ay, x, y, cx, cy) / totalArea;
			double gamma = signedTriangleArea(ax, ay, bx, by, x, y) / totalArea;

			// Check if pixel is inside triangle
			if (alpha < 0 || beta < 0 || gamma < 0) continue;

			// Interpolate Z-coordinate using barycentric coordinates
			double z = alpha * az + beta * bz + gamma * cz;

			// Z-buffer test (assuming higer Z values are closer to camera)
			TGAColor currentZ = zBuffer.get(x, y);
			unsigned char zValue = static_cast<unsigned char>(std::clamp(z, 0.0, 255.0));

			if (zValue > currentZ[0]) // Closer to camera
			{
				zBuffer.set(x, y, { zValue, zValue, zValue, 255 });
				frameBuffer.set(x, y, color);
			}
		}
	}

	// Scanline fill algorithm
	// Process each horizontal line from top vertex to bottom vertex
	// For each horizontal line, find intersections with triangle edges
  //  for (int y = ay; y <= cy; y++)
  //  {
		//// Collect intersections of the scanline with triangle edges
  //      std::vector<int> intersections;
  //      
		//// Define edges of the triangle
  //      int edges[3][4] = {
		//	{ax, ay, bx, by},
		//	{bx, by, cx, cy},
		//	{cx, cy, ax, ay}
		//};
  //      
  //      for (int i = 0; i < 3; i++)
  //      {
  //          int x1 = edges[i][0], y1 = edges[i][1];
  //          int x2 = edges[i][2], y2 = edges[i][3];
  //          
  //          // Check if scanline intersects this edge (avoid horizontal edges)
  //          if (y1 != y2 && ((y1 < y && y2 >= y) || (y2 < y && y1 >= y)))
  //          {
  //              int xIntersect = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
  //              intersections.push_back(xIntersect);
  //          }
  //      }
  //      
  //      // Sort intersections and fill between pairs
  //      if (intersections.size() >= 2)
  //      {
  //          std::sort(intersections.begin(), intersections.end());
  //          int xMin = intersections[0];
  //          int xMax = intersections[intersections.size() - 1];
  //          
  //          // Fill the scanline within screen bounds
  //          for (int x = std::max(0, xMin); x <= std::min(width - 1, xMax); x++)
  //          {
  //              frameBuffer.set(x, y, color);
  //          }
  //      }
  //  }
}
vec3 rot(vec3 vector)
{
	constexpr double angle = std::numbers::pi / 6.0; // 30 degrees
	double cosAngle = std::cos(angle);
	double singAngle = std::sin(angle);

	mat<3, 3> Ry;
	Ry[0][0] = cosAngle; Ry[0][1] = 0; Ry[0][2] = singAngle;
	Ry[1][0] = 0;        Ry[1][1] = 1; Ry[1][2] = 0;
	Ry[2][0] = -singAngle; Ry[2][1] = 0; Ry[2][2] = cosAngle;


	return Ry * vector; // Rotate around Y-axis
}

// Project 3D coordinates to 2D screen space orthographic projection
std::tuple<int, int, int> project(const vec3& v)
{
	return {
		static_cast<int>((v.x + 1) * width / 2),
		static_cast<int>((v.y + 1) * height / 2),
		static_cast<int>((v.z + 1) * 255.0 / 2)
	};
}

// Check if the model is loaded correctly
bool checkModel(const Model& model, const char* filename)
{
	if (filename == nullptr)
	{
		std::cerr << "Error: No model file provided";
		return false;
	}
	if (model.nverts() == 0 || model.nfaces() == 0) // Check if model is empty/loaded correctly
	{
		std::cerr << "Error: Model failed to load or is empty. File: " << filename << "\n";
		return false;
	}
	return true;
}

int main(int argc, char** argv)
{
	auto start = std::chrono::high_resolution_clock::now();

	const std::string filename = argv[2];
	std::string_view argv1(argv[1]);
	if (argv1 == "--wireframe")
	{
		if (argc < 3)
		{
			std::cerr << "Usage: " << argv[0] << " --wireframe <model.obj>\n";
			return EXIT_FAILURE;
		}
		const std::string filename = argv[2];
		Model model(filename);
		if (!checkModel(model, filename.c_str())) // Check if model is empty/loaded correctly
		{
			return EXIT_FAILURE;
		}

		TGAImage frameBuffer(width, height, TGAImage::RGB);

		// Draw all triangles edges from faces
		for (int i = 0; i < model.nfaces(); i++)
		{
			auto [ax, ay, az] = project(model.vert(i, 0));
			auto [bx, by, bz] = project(model.vert(i, 1));
			auto [cx, cy, cz] = project(model.vert(i, 2));

			line(ax, ay, bx, by, frameBuffer, red);
			line(bx, by, cx, cy, frameBuffer, red);
			line(cx, cy, ax, ay, frameBuffer, red);
		}

		// Draw vertices as white dots
		for (int i = 0; i < model.nverts(); i++)
		{
			const vec3& vertex = model.vert(i);
			auto [x, y, z] = project(vertex);
			frameBuffer.set(x, y, white);
		}

		frameBuffer.write_tga_file("frameBufferOutput.tga");

		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "Rendered in " << elapsed.count() << " ms\n";

		return EXIT_SUCCESS;

	}
	else if (argv1 == "--faces")
	{
		if (argc < 3)
		{
			std::cerr << "Error: No model file provided. Usage: " << argv[0] << " --faces <model.obj>\n";
			return EXIT_FAILURE;
		}
		// Error handling
		const std::string filename = argv[2];
		Model model(filename);
		if (!checkModel(model, filename.c_str())) // Check if model is empty/loaded correctly
		{
			return EXIT_FAILURE;
		}

		TGAImage frameBuffer(width, height, TGAImage::RGB);
		TGAImage zBuffer(width, height, TGAImage::GRAYSCALE);

		for (int i = 0; i < model.nfaces(); i++) // Iterating through all faces
		{
			// Each face is a triangle defined by 3 vertices
			vec3 v0 = model.vert(i, 0);
			vec3 v1 = model.vert(i, 1);
			vec3 v2 = model.vert(i, 2);

			// Single call to project each vertex
			auto [ax, ay, az] = project(rot(model.vert(i, 0)));
			auto [bx, by, bz] = project(rot(model.vert(i, 1)));
			auto [cx, cy, cz] = project(rot(model.vert(i, 2)));

			TGAColor randomColor = { rand() % 256, rand() % 256, rand() % 256, 255 };
			triangle(ax, ay, az, bx, by, bz, cx, cy, cz, zBuffer, frameBuffer, randomColor, v0, v1, v2);
		}
		frameBuffer.write_tga_file("triangleOutput.tga");
		zBuffer.write_tga_file("zBufferOutput.tga");
		std::cout << "Image drawn.\n";

		return EXIT_SUCCESS;

	}
	else
	{
		std::cerr << "Unknown command: " << argv[1] << " Use '--obj' or '--triangle'.\n" << std::endl;
		return EXIT_FAILURE;
	}
}