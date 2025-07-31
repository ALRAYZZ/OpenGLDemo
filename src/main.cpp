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

struct Camera
{
	vec3 eye = { -1, 0, 2 }; // Camera position in 3D space
	vec3 center = { 0, 0, 0 }; // Point the camera is looking at
	vec3 up = { 0, 1, 0 }; // Up direction of the camera
	double fov = std::numbers::pi / 4; // Field of view in radians
};

Camera camera;
mat<4, 4> ModelView, Perspective, Viewport;

void lookAt(const vec3 eye, const vec3 center, const vec3 up)
{
	vec3 n = normalized(eye - center);
	vec3 l = normalized(cross(up, n));
	vec3 m = normalized(cross(n, l));
	ModelView = mat<4, 4>{{{l.x, l.y, l.z, 0}, {m.x, m.y, m.z, 0}, {n.x, n.y, n.z, 0}, {0, 0, 0, 1}}} *
				mat<4, 4>{{{1, 0, 0, -center.x}, { 0, 1, 0, -center.y }, { 0, 0, 1, -center.z }, { 0, 0, 0, 1 }}};
}

void perspective(const double f)
{
	Perspective = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, -1/f, 1}}};
}

void viewport(const int x, const int y, const int w, const int h)
{
	Viewport = { {{w / 2., 0, 0, x + w / 2.}, {0, h / 2., 0, y + h / 2.}, {0, 0, 1, 0}, {0, 0, 0, 1}} };
}


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
	vec3 triangleCenter = (v0 + v1 + v2) / 3.0;
	vec3 cameraDir = normalized(camera.eye - triangleCenter);

	// If dot product is negative, triangle is facing away from camera
	// Based on the angle between the triangle normal and camera direction we can determine visibility
	if (normal * cameraDir <= 0)
	{
		return; // Backface culling: Skip triangle if facing away from camera
	}

	// Use bounding box approach to limit the area we need to scan
	int bbminx = std::max(0, std::min({ ax, bx, cx }));
	int bbminy = std::max(0, std::min({ ay, by, cy }));
	int bbmaxx = std::min(width - 1, std::max({ ax, bx, cx }));
	int bbmaxy = std::min(height - 1, std::max({ ay, by, cy }));

	// Calculate total triange area for barycentric coordinates
	vec3 v0v1 = { bx - ax, by - ay, 0 };
	vec3 v0v2 = { cx - ax, cy - ay, 0 };
	double totalArea = 0.5 * std::abs(cross(v0v1, v0v2).z);
	if (totalArea < 1e-6) return; // Degenerate triangle, skip rendering

	for (int x = bbminx; x <= bbmaxx; x++)
	{
		for (int y = bbminy; y <= bbmaxy; y++)
		{
			// Calculate barycentric coordinates
			vec3 pv1 = { bx - x, by - y, 0 };
			vec3 pv2 = { cx - x, cy - y, 0 };
			double alpha = signedTriangleArea(x, y, bx, by, cx, cy) / totalArea;

			vec3 v0p = { x - ax, y - ay, 0 };
			vec3 v0v2Area = { cx - x, cy - y, 0 };
			double beta = 0.5 * std::abs(cross(v0p, v0v2Area).z / totalArea);
			double gamma = 1.0 - alpha - beta;

			// Check if pixel is inside triangle
			if (alpha < 0 || beta < 0 || gamma < 0) continue;

			// Interpolate Z-coordinate using barycentric coordinates
			double z = alpha * az + beta * bz + gamma * cz;

			// Z-buffer test (assuming higer Z values are closer to camera)
			TGAColor currentZ = zBuffer.get(x, y);
			double zNorm = (z + 1) / 2; // Normalize z to [0, 1] range
			unsigned char zValue = static_cast<unsigned char>(std::clamp(zNorm * 255, 0.0, 255.0));

			if (zValue > currentZ[0]) // Closer to camera
			{
				// Store depth in grayscale z-buffer
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
// Rotate vector around Y-axis by 60 degrees. Making the model spin around Y-axis
//vec3 rot(vec3 vector)
//{
//	constexpr double angle = std::numbers::pi / 6; // Rotation angle in radians
//	double cosAngle = std::cos(angle);
//	double singAngle = std::sin(angle);
//
//	mat<3, 3> Ry;
//	Ry[0][0] = cosAngle; Ry[0][1] = 0; Ry[0][2] = singAngle;
//	Ry[1][0] = 0;        Ry[1][1] = 1; Ry[1][2] = 0;
//	Ry[2][0] = -singAngle; Ry[2][1] = 0; Ry[2][2] = cosAngle;
//
//
//	return Ry * vector; // Rotate around Y-axis
//}


// Project 3D coordinates to 2D screen space orthographic projection
std::tuple<int, int, double> project(const vec4& vector)
{
	vec4 ndc = vector / vector.w; // Prespective divide
	vec4 screen = Viewport * ndc; // Screen space coordinates
	return { static_cast<int>(screen.x), static_cast<int>(screen.y), ndc.z }; // Return NDC z for depth testing
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
	srand(time(nullptr)); // Seed random number generator

	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " --wireframe <model.obj> or --faces <model.obj>\n";
		return EXIT_FAILURE;
	}

	// Initialize camera and projection matrices
	lookAt(camera.eye, camera.center, camera.up);
	perspective(1.0 / std::tan(camera.fov / 2.0)); // Perspective projection matrix
	viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // Viewport transformation matrix

	const std::string filename = argv[2];
	std::string_view argv1(argv[1]);

	if (argv1 == "--wireframe")
	{
		Model model(filename);
		if (!checkModel(model, filename.c_str())) // Check if model is empty/loaded correctly
		{
			return EXIT_FAILURE;
		}

		TGAImage frameBuffer(width, height, TGAImage::RGB);

		// Draw all triangles edges from faces
		for (int i = 0; i < model.nfaces(); i++)
		{
			vec4 clip[3];
			for (int d = 0; d < 3; d++)
			{
				vec3 v = model.vert(i, d);
				clip[d] = Perspective * ModelView * vec4{ v.x, v.y, v.z, 1.0 };
			}
			auto [ax, ay, az] = project(clip[0]);
			auto [bx, by, bz] = project(clip[1]);
			auto [cx, cy, cz] = project(clip[2]);

			// Draw edges of the triangle
			line(ax, ay, bx, by, frameBuffer, red);
			line(bx, by, cx, cy, frameBuffer, red);
			line(cx, cy, ax, ay, frameBuffer, red);
		}

		// Draw vertices as white dots
		for (int i = 0; i < model.nverts(); i++)
		{
			vec3 v = model.vert(i);
			vec4 clip = Perspective * ModelView * vec4{ v.x, v.y, v.z , 1.0 };
			auto [x, y, z] = project(clip);
			frameBuffer.set(x, y, white); // Draw vertex as white dot
		}

		frameBuffer.write_tga_file("frameBufferOutput.tga");

		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "Rendered in " << elapsed.count() << " ms\n";

		return EXIT_SUCCESS;

	}
	else if (argv1 == "--faces")
	{
		Model model(filename);
		if (!checkModel(model, filename.c_str())) // Check if model is empty/loaded correctly
		{
			return EXIT_FAILURE;
		}

		TGAImage frameBuffer(width, height, TGAImage::RGB);
		TGAImage zBuffer(width, height, TGAImage::GRAYSCALE);

		for (int i = 0; i < model.nfaces(); i++) // Iterating through all faces
		{
			vec4 clip[3];
			vec3 world[3];
			for (int d = 0; d < 3; d++)
			{
				world[d] = model.vert(i, d);
				clip[d] = Perspective * ModelView * vec4{ world[d].x, world[d].y, world[d].z, 1.0 };
			}

			// Project the vertices to 2D screen space
			auto [ax, ay, az] = project(clip[0]);
			auto [bx, by, bz] = project(clip[1]);
			auto [cx, cy, cz] = project(clip[2]);

			TGAColor randomColor = { rand() % 256, rand() % 256, rand() % 256, 255 };
			// Draw triangle using barycentric coordinates
			triangle(ax, ay, az, bx, by, bz, cx, cy, cz, zBuffer, frameBuffer, randomColor, world[0], world[1], world[2]);
		}
		frameBuffer.write_tga_file("triangleOutput.tga");
		zBuffer.write_tga_file("zBufferOutput.tga");
		std::cout << "Image drawn.\n";

		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "Rendered in " << elapsed.count() << " ms\n";
		return EXIT_SUCCESS;

	}
	else
	{
		std::cerr << "Unknown command: " << argv[1] << " Use '--obj' or '--triangle'.\n" << std::endl;
		return EXIT_FAILURE;
	}
}