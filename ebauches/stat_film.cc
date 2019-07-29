#include <iostream>
#include <vector>

#include "../../repos/utils/io/io_utils.h"
#include "../../repos/utils/math/statistics.h"
#include "../../repos/utils/util/util_time.h"
#include "../../repos/utils/util/profiling.h"

void compute_stats(const std::string &filename)
{
	std::ifstream infile(filename);

	if (!infile.good()) {
		std::cerr << "Cannot read file: " << filename << "!\n";
		return;
	}

	using value_type = float;

	/* count the number of lines in the file. */
	auto lines = 0;
	io::foreach_line(infile, [&lines](const std::string &)
	{
		++lines;
	});

	std::vector<value_type> durees;
	durees.reserve(lines);

	/* return to the begining of the file */
	infile.clear();
	infile.seekg(0, std::ios::beg);

	io::foreach_line(infile, [&durees](const std::string &line)
	{
		if (line.size() < 11) {
			return;
		}

		const auto &temps = util::smpte_to_time<value_type>(line);

		/* exclude short films */
		if (temps < value_type(58.22)) {
			return;
		}

		durees.push_back(temps);
	});

	std::sort(durees.begin(), durees.end());

	auto mean = math::mean<value_type>(durees.begin(), durees.end());
	auto median = math::median<value_type>(durees.begin(), durees.end());
	auto dev = math::standard_deviation<value_type>(durees.begin(), durees.end(), mean);

	std::ostream &os = std::cout;
	os << "Moyenne: " << mean << " (" << util::time_to_smpte(mean) << ")\n";
	os << "Médiane: " << median << " (" << util::time_to_smpte(median) << ")\n";
	os << "Déviation: " << dev << " (" << util::time_to_smpte(dev) << ")\n";
}

int main(int argc, char **argv)
{
	Timer(__func__);

	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " filename\n";
		return 1;
	}

	compute_stats(argv[1]);
}
