#ifndef KYLIN_EXPLORER_UTILS_H
#define KYLIN_EXPLORER_UTILS_H

#include <cstdint>
#include <string>

void FormatSize(char *output, size_t size, uint64_t bytes);
void FormatModifiedTime(char *output, size_t size, uint64_t timestamp);
bool IsMounted(const char *path);
std::string JoinPath(const std::string &directory, const std::string &name);
std::string ParentPath(const std::string &path);
bool IsPathWithin(const std::string &path, const std::string &root);
uint64_t GetFileSize(const std::string &path);

#endif
