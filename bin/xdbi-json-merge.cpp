#include <cxxopts/cxxopts.hpp>
#include "JsonMerge.hpp"
#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <boost/filesystem.hpp>
    namespace fs = boost::filesystem;
#endif
#include <fstream>

int main(int argc, char *argv[])
{
    cxxopts::Options options("xdbi-json-merge", "3-way merge of JSON files");
    options.add_options()
        ("original", "The common ancestor of the modified versions", cxxopts::value<fs::path>())
        ("ours", "The local version. Will be overwritten by result of merge", cxxopts::value<fs::path>())
        ("theirs", "The remote version", cxxopts::value<fs::path>())
        ("help", "Print usage");
    options.parse_positional({"original", "ours", "theirs"});
    options.show_positional_help();
    auto parsed_options = options.parse(argc, argv);
    if (parsed_options.count("help"))
    {
        std::cout << options.help() << std::endl;
        return EXIT_SUCCESS;
    }   

    // Open and parse the three given files
    std::ifstream original_file(parsed_options["original"].as<fs::path>());
    std::ifstream our_file(parsed_options["ours"].as<fs::path>());
    std::ifstream their_file(parsed_options["theirs"].as<fs::path>());

    // Call 3-way merge algorithm
    nl::json result;
    bool have_conflict = three_way_merge(nl::json::parse(original_file), nl::json::parse(our_file), nl::json::parse(their_file), result);
    original_file.close();
    our_file.close();
    their_file.close();

    // Write the result back to the local files
    std::ofstream new_file(parsed_options["ours"].as<fs::path>());
    new_file << result.dump(4);
    new_file.close();

    return have_conflict ? 1 : 0;
}
