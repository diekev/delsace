#include <iostream>

#include "../utils/io/file.h"

enum {
	WHITE = 0,
	BLACK = 1
};

enum {
	FACE_UP = 0,
	FACE_DOWN,
	FACE_LEFT,
	FACE_RIGHT
};

class Ant {
	int m_x, m_y, m_facing;

public:
	Ant(const int x, const int y, const int facing)
		: m_x(x)
		, m_y(y)
		, m_facing(facing)
	{}

	~Ant() = default;

	void turn_right()
	{
		switch (m_facing) {
			case FACE_UP:
				m_x = m_x + 1;
				m_facing = FACE_RIGHT;
				break;
			case FACE_DOWN:
				m_x = m_x - 1;
				m_facing = FACE_LEFT;
				break;
			case FACE_LEFT:
				m_y = m_y - 1;
				m_facing = FACE_UP;
				break;
			case FACE_RIGHT:
				m_y = m_y + 1;
				m_facing = FACE_DOWN;
				break;
		}
	}

	void turn_left()
	{
		switch (m_facing) {
			case FACE_UP:
				m_x = m_x - 1;
				m_facing = FACE_LEFT;
				break;
			case FACE_DOWN:
				m_x = m_x + 1;
				m_facing = FACE_RIGHT;
				break;
			case FACE_LEFT:
				m_y = m_y + 1;
				m_facing = FACE_DOWN;
				break;
			case FACE_RIGHT:
				m_y = m_y - 1;
				m_facing = FACE_UP;
				break;
		}
	}

	void clamp_pos(const int rx, const int ry)
	{
		if (m_x >= rx) {
			m_x = rx - 1;
		}

		if (m_y >= ry) {
			m_y = ry - 1;
		}
	}

	int get_index(const int rx) const
	{
		return m_x + rx * m_y;
	}
};

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " outfile.\n";
		return 1;
	}

	const int res_x = 800, res_y = 800;
	int grid[res_x * res_y] = {0};

	Ant ant(400, 400, FACE_DOWN);

	int i = 25000;

	while (i--) {
		auto ant_index = ant.get_index(res_x);

		if (grid[ant_index] == WHITE) {
			grid[ant_index] = BLACK;
			ant.turn_right();
		}
		else {
			grid[ant_index] = WHITE;
			ant.turn_left();
		}

		ant.clamp_pos(res_x, res_y);
	}

	io::File f(argv[1], "wb");
	f.print("P6\n%d %d\n255\n", res_x, res_y);

	int index = 0;
	for (int j(0); j < res_y; ++j) {
		for (int i = 0; i < res_x; ++i, ++index) {
			auto col = (unsigned char)(1 - grid[index]) * 255;

			f.print("%c%c%c", col, col, col);
		}
	}
}
