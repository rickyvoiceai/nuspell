#include "finder.hxx"

namespace nuspell {
namespace v5 {

auto append_default_dir_paths(std::vector<path>&) -> void {}
auto append_libreoffice_dir_paths(std::vector<path>&) -> void {}
auto search_dirs_for_one_dict(const std::vector<path>&,
                              const path&) -> path
{
	return path();
}
auto search_dirs_for_dicts(const std::vector<path>&,
                           std::vector<path>&) -> void {}
auto search_default_dirs_for_dicts() -> std::vector<path>
{
	return {};
}
auto append_default_dir_paths(std::vector<std::string>&) -> void {}
auto append_libreoffice_dir_paths(std::vector<std::string>&) -> void {}
auto search_dir_for_dicts(
    const std::string&,
    std::vector<std::pair<std::string, std::string>>&) -> void
{
}
auto search_dirs_for_dicts(
    const std::vector<std::string>&,
    std::vector<std::pair<std::string, std::string>>&) -> void
{
}
auto search_default_dirs_for_dicts(
    std::vector<std::pair<std::string, std::string>>&) -> void
{
}
auto find_dictionary(
    const std::vector<std::pair<std::string, std::string>>&,
    const std::string&)
    -> std::vector<std::pair<std::string, std::string>>::const_iterator
{
	return {};
}

Dict_Finder_For_CLI_Tool::Dict_Finder_For_CLI_Tool() = default;
auto Dict_Finder_For_CLI_Tool::get_dictionary_path(
    const std::string&) const -> std::string
{
	return {};
}

Dict_Finder_For_CLI_Tool_2::Dict_Finder_For_CLI_Tool_2() = default;
auto Dict_Finder_For_CLI_Tool_2::get_dictionary_path(
    const path&) const -> path
{
	return {};
}

}
}