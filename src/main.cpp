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

// Project 3D coordinates to 2D screen space
std::tuple<int, int> project(const vec3& v)
{
	return { (v.x + 1) * width / 2,
			 (v.y + 1)* height / 2 };
}

int main(int argc, char** argv)
{
	auto start = std::chrono::high_resolution_clock::now();

	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
		return EXIT_FAILURE;
	}

	const std::string filename = argv[1];
	Model model(filename);

	if (model.nverts() == 0 || model.nfaces() == 0) // Check if model is empty/loaded correctly
	{
		std::cerr << "Error: Model failed to load or is empty.\n";
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