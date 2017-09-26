#include <iostream>
#include <fstream>
#include <mapbox/geometry/geometry.hpp>
#include <boost/geometry/geometry.hpp>
// vtzero
#include <vtzero/vector_tile.hpp>

int main(int argc, char** argv)
{
    std::cerr << "Vector Tile to SVG converter" << std::endl;
    if (argc != 2)
    {
        std::cerr << "Usage:" << argv[0] << " <path-to-vector-tile>" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream file(argv[1]);
    if (!file)
    {
        std::cerr << "Failed to open:" << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<char> buf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    vtzero::data_view view(buf.data(), buf.size());
    vtzero::vector_tile tile{view};

    for (auto const& layer : tile)
    {
        std::cerr << "LAYER:" << std::string(layer.name().data(),layer.name().data()+layer.name().size()) << std::endl;
    }
    return EXIT_SUCCESS;
}
