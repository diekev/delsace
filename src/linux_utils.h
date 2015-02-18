#ifndef LINUX_UTILS_H
#define LINUX_UTILS_H

#include <string>
#include <vector>

namespace Linux {

auto execBuildImageList(const char *cmd) -> std::vector<std::string>;
auto execRemoveImage(const char *cmd) -> void;

}

#endif // LINUX_UTILS_H
