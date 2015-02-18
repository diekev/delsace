#include <algorithm>

#include "linux_utils.h"

namespace Linux {

auto execBuildImageList(const char *cmd) -> std::vector<std::string>
{
	FILE *pipe = popen(cmd, "r");

	char buffer[128];
	auto result = std::vector<std::string>{};

	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != nullptr) {
			auto str = std::string{buffer};
			str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
			result.push_back(str);
		}
	}

	pclose(pipe);

	return result;
}

auto execRemoveImage(const char *cmd) -> void
{
	FILE *pipe = popen(cmd, "r");
	pclose(pipe);
}

} /* namespace Linux */
