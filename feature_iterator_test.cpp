#include <iostream>
#include <fstream>
#include <sstream>

#include <mapbox/geometry/geometry.hpp>
#include <mapbox/geometry/feature.hpp>
#include <mapbox/geometry/algorithms/detail/boost_adapters.hpp>
#include <mapbox/geometry/algorithms/closest_point_impl.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/optional/optional.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
// vtzero
#include <vtzero/vector_tile.hpp>
// utils
#include "mvt_utils.hpp"

template <typename Out>
struct print_value
{
    print_value(Out & out)
        : out_(out) {}

    void operator() (mapbox::geometry::null_value_t) const
    {
        out_ << "null";
    }
    void operator() (mapbox::geometry::value const& val) const
    {
        mapbox::util::apply_visitor(*this, val);
    }
    void operator() (std::vector<mapbox::geometry::value> const& val) const
    {
        for (auto const& v : val)
        {
            out_ << "    ";
            operator()(v);
        }
    }
    void operator() (std::unordered_map<std::string,
                     mapbox::geometry::value> const& val) const
    {
        for (auto const& kv : val)
        {
            out_ << "    " << kv.first << ":";
            operator()(kv.second);
        }
    }
    template <typename T>
    void operator() (T const& val) const
    {
        out_ << val;
    }

    Out & out_;
};

int main(int argc, char** argv)
{
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
    std::size_t count = 0;
    while (auto layer = tile.next_layer())
    {
        mapbox::geometry::point<double> pt{100,100};
        while (auto feature = layer.next_feature())
        {
            auto geom = detail::make_geometry<double>(feature);
            //auto props = detail::make_properties(feature);
            auto  props = vtzero::create_properties_map<mapbox::geometry::property_map>(feature);
            auto info = mapbox::geometry::algorithms::closest_point(geom, pt);
            std::cerr << info.x << "," << info.y << " distance:" << info.distance << std::endl;
            for (auto const& kv : props)
            {
                std::cerr << kv.first << ":" ;
                mapbox::util::apply_visitor(print_value<std::ostream>(std::cout), kv.second);
                std::cerr << std::endl;
            }
            ++count;
        }
    }
    std::cerr << "NUM FEATURES:" << count << std::endl;
    std::cerr << "======================================================" << std::endl;
    std::cerr << "FEATURE size:" << sizeof(mapbox::geometry::feature<double>) << std::endl;
    std::cerr << "PROPERTY_MAP size:" << sizeof(mapbox::geometry::property_map) << std::endl;
    std::cerr << "ID size:" << sizeof(mapbox::geometry::identifier) << std::endl;
    std::cerr << "optional ID size:" << sizeof(std::experimental::optional<mapbox::geometry::identifier>) << std::endl;
    std::cerr << "GEOMETRY size:" << sizeof(mapbox::geometry::geometry<double>) << std::endl;
    return EXIT_SUCCESS;
}
