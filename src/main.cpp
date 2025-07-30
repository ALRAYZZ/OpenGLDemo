#include <tgaimage.h>
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
void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& frameBuffer, TGAColor color)
{
	// We need vertices sorted from top to bottom
	// This ensures our scanline algorithm processes triangles correctly
    if (ay > by) std::swap(ax, bx), std::swap(ay, by);
    if (ay > cy) std::swap(ax, cx), std::swap(ay, cy);
    if (by > cy) std::swap(bx, cx), std::swap(by, cy);

    // Draw triangle edges (wireframe)
    line(ax, ay, bx, by, frameBuffer, color);
    line(bx, by, cx, cy, frameBuffer, color);
    line(cx, cy, ax, ay, frameBuffer, color);

	// Scanline fill algorithm
	// Process each horizontal line from top vertex to bottom vertex
	// For each horizontal line, find intersections with triangle edges
    for (int y = ay; y <= cy; y++)
    {
		// Collect intersections of the scanline with triangle edges
        std::vector<int> intersections;
        
		// Define edges of the triangle
        int edges[3][4] = {
			{ax, ay, bx, by},
			{bx, by, cx, cy},
			{cx, cy, ax, ay}
		};
        
        for (int i = 0; i < 3; i++)
        {
            int x1 = edges[i][0], y1 = edges[i][1];
            int x2 = edges[i][2], y2 = edges[i][3];
            
            // Check if scanline intersects this edge (avoid horizontal edges)
            if (y1 != y2 && ((y1 < y && y2 >= y) || (y2 < y && y1 >= y)))
            {
                int xIntersect = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
                intersections.push_back(xIntersect);
            }
        }
        
        // Sort intersections and fill between pairs
        if (intersections.size() >= 2)
        {
            std::sort(intersections.begin(), intersections.end());
            int xMin = intersections[0];
            int xMax = intersections[intersections.size() - 1];
            
            // Fill the scanline within screen bounds
            for (int x = std::max(0, xMin); x <= std::min(width - 1, xMax); x++)
            {
                frameBuffer.set(x, y, color);
            }
        }
    }
}

// Project 3D coordinates to 2D screen space orthographic projection
std::tuple<int, int> project(const vec3& v)
{
	return { (v.x + 1) * width / 2,
			 (v.y + 1)* height / 2 };
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
			auto [ax, ay] = project(model.vert(i, 0));
			auto [bx, by] = project(model.vert(i, 1));
			auto [cx, cy] = project(model.vert(i, 2));

			line(ax, ay, bx, by, frameBuffer, red);
			line(bx, by, cx, cy, frameBuffer, red);
			line(cx, cy, ax, ay, frameBuffer, red);
		}

		// Draw vertices as white dots
		for (int i = 0; i < model.nverts(); i++)
		{
			const vec3& vertex = model.vert(i);
			auto [x, y] = project(vertex);
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

		for (int i = 0; i < model.nfaces(); i++)
		{
			auto [ax, ay] = project(model.vert(i, 0));
			auto [bx, by] = project(model.vert(i, 1));
			auto [cx, cy] = project(model.vert(i, 2));
			TGAColor randomColor(rand() % 255, rand() % 255, rand() % 255, 255);
			triangle(ax, ay, bx, by, cx, cy, frameBuffer, randomColor);
		}
		frameBuffer.write_tga_file("triangleOutput.tga");
		std::cout << "Triangle drawn.\n";

		return EXIT_SUCCESS;

	}
	else
	{
		std::cerr << "Unknown command: " << argv[1] << " Use '--obj' or '--triangle'.\n" << std::endl;
		return EXIT_FAILURE;
	}
}