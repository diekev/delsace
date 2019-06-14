#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

bool check_points(const std::vector<float> &points)
{
	auto num = static_cast<float>(points.size());

	for (auto i(0ul); i < points.size(); ++i) {
		auto bound = static_cast<float>(i);

		if ((bound / num <= points[i]) && (points[i] < (bound + 1.0f) / num)) {
			continue;
		}

		return false;
	}

	return true;
}

int main()
{
	std::vector<float> points;
	std::mt19937 rng(19937);
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	auto max_points(0ul);
	auto i(0);

	while (true) {
		std::cout << "Iteration: " << ++i << "\n";
		points.clear();
//		points.push_back(dist(rng));

		do {
			points.push_back(dist(rng));
			std::sort(points.begin(), points.end());
		} while (!check_points(points));

		if (max_points < points.size()) {
			max_points = points.size();
			continue;
		}

		break;
	}

	std::cout << "Max points: " << max_points << "\n";
	std::cout << "Nombre d'iteration: " << i;
}
