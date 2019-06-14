#include <iostream>
#include <random>
#include <sstream>
#include <tbb/parallel_for.h>

#include "../utils/io/file.h"

enum {
	DEAD  = 0,
	ALIVE = 1,
};

int get_index(int x, int res_x, int y)
{
	return x + y * res_x;
}

void step(int res_x, int res_y, int *grid)
{
	tbb::parallel_for(tbb::blocked_range<int>(1, res_y - 1),
		[&](tbb::blocked_range<int> &r)
		{
			for (int y = r.begin(), ye = r.end(); y != ye; ++y) {
				for (int x = 1; x < res_x - 1; ++x) {
					auto count = grid[get_index(x    , res_x, y - 1)] +
						grid[get_index(x - 1, res_x, y - 1)] +
						grid[get_index(x - 1, res_x, y)] +
						grid[get_index(x - 1, res_x, y + 1)] +
						grid[get_index(x    , res_x, y + 1)] +
						grid[get_index(x + 1, res_x, y - 1)] +
						grid[get_index(x + 1, res_x, y)] +
						grid[get_index(x + 1, res_x, y + 1)];

					if (count < 2 || count > 3) {
						grid[get_index(x, res_x, y)] = DEAD;
					}
					else if (count == 3) {
						grid[get_index(x, res_x, y)] = ALIVE;
					}
				}
			}
		});
}

std::string get_name_frame(const std::string &basename, int frame)
{
	std::string str(basename);
	std::stringstream ss;
	ss << frame;
	std::string frame_str(ss.str());

	frame_str.insert(0, 4 - frame_str.size(), '0');

	return str + frame_str + ".ppm";
}

void write_ppm(const std::string &name, int res_x, int res_y, int *grid)
{
	io::File f(name.c_str(), "wb");
	f.print("P6\n%d %d\n255\n", res_x, res_y);

	for (int y = 0; y < res_y; ++y) {
		for (int x = 0; x < res_x; ++x) {
			auto col = (unsigned char)(1 - grid[x + y * res_x]) * 255;
			f.print("%c%c%c", col, col, col);
		}
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " outfile.\n";
		return 1;
	}

	const int res_x = 500, res_y = 500;
	int grid[res_x * res_y] = { ALIVE };

	std::mt19937 rng(199337);
	std::uniform_int_distribution<int> dist(0, 100);

	for (int y = 200; y < res_y - 200; ++y) {
		for (int x = 200; x < res_x - 200; ++x) {
			grid[x + y * res_x] = dist(rng) % 2;
		}
	}

	auto name = get_name_frame(argv[1], 0);
	write_ppm(name, res_x, res_y, grid);

	for (int i = 0; i < 100; ++i) {
		step(res_x, res_y, grid);
		auto name = get_name_frame(argv[1], i + 1);
		write_ppm(name, res_x, res_y, grid);
	}
}
