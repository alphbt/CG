#pragma once
#ifndef  __IMAGE__
#define __IMAGE__

#include<fstream>
#include<vector>
#include<glm.hpp>

class Image
{

public:
	Image() {}
	Image(unsigned char* pixels, int A, int B) : nx(A), ny(B), data(pixels) {}

	glm::vec3 value(float u, float v) const;
	
	unsigned char* data;
	int nx, ny;
};

glm::vec3 Image::value(float u, float v) const {
	int i = (u)*nx;
	int j = (1 - v) * ny - 0.001;

	if (i < 0) i = 0;
	if (j < 0) j = 0;

	if (i > nx - 1) i = nx - 1;
	if (j > ny - 1) j = ny - 1;


	float r = int(data[3 * i + 3 * nx * j]) / 255.0;
	float g = int(data[3 * i + 3 * nx * j + 1]) / 255.0;
	float b = int(data[3 * i + 3 * nx * j + 2]) / 255.0;

	return glm::vec3(r, g, b);
}


#endif // ! __IMAGE__
