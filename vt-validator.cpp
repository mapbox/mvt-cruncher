#include <iostream>
#include <fstream>
#include <mapbox/geometry/geometry.hpp>
#include <mapbox/geometry/algorithms/detail/boost_adapters.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/optional/optional.hpp>
// vtzero
#include <vtzero/vector_tile.hpp>

template <typename T>
void expand_to_include(mapbox::geometry::box<T> & bbox, mapbox::geometry::box<T> const& b)
{
    if (b.min.x < bbox.min.x) bbox.min.x = b.min.x;
    if (b.max.x > bbox.max.x) bbox.max.x = b.max.x;
    if (b.min.y < bbox.min.y) bbox.min.y = b.min.y;
    if (b.max.y > bbox.max.y) bbox.max.y = b.max.y;
}

struct point_processor
{
    point_processor(mapbox::geometry::multi_point<double> & mpoint)
        : mpoint_(mpoint) {}

    void points_begin(uint32_t count) const
    {
        if (count > 1) mpoint_.reserve(count);
    }

    void points_point(vtzero::point const& point) const
    {
        mpoint_.emplace_back(point.x, point.y);
    }

    void points_end() const
    {
        //
    }
    mapbox::geometry::multi_point<double> & mpoint_;
};

struct linestring_processor
{
    linestring_processor(mapbox::geometry::multi_line_string<double> & mline)
        : mline_(mline) {}

    void linestring_begin (std::uint32_t count)
    {
        mline_.emplace_back();
        mline_.back().reserve(count);
    }

    void linestring_point(vtzero::point const& point) const
    {
        mline_.back().emplace_back(point.x, point.y);
    }

    void linestring_end() const noexcept
    {
    }
    mapbox::geometry::multi_line_string<double> & mline_;
};

struct polygon_processor
{
    polygon_processor(mapbox::geometry::multi_polygon<double> & mpoly)
        : mpoly_(mpoly) {}

    void ring_begin(std::uint32_t count)
    {
        ring_.reserve(count);
    }

    void ring_point(vtzero::point const& point)
    {
        ring_.emplace_back(point.x, point.y);
    }

    void ring_end(bool is_outer)
    {
        if (is_outer) mpoly_.emplace_back();
        mpoly_.back().push_back(std::move(ring_));
        ring_.clear();
    }

    mapbox::geometry::multi_polygon<double> & mpoly_;
    mapbox::geometry::linear_ring<double> ring_;
};

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

    std::size_t count = 0;
    std::size_t fail_count = 0;
    //boost::geometry::validity_failure_type failure;
    std::string red = "\033[0;31m";
    std::string no_color = "\033[0m";
    std::string message;
    for (auto const& layer : tile)
    {
        for (auto const feature : layer)
        {
            switch (feature.type())
            {
            case vtzero::GeomType::POINT:
            {
                mapbox::geometry::multi_point<double> mpoint;
                point_processor proc_point(mpoint);
                vtzero::decode_point_geometry(feature.geometry(), false, proc_point);
                bool valid = boost::geometry::is_valid(mpoint, message);
                //bool simple = boost::geometry::is_simple(mpoint);
                if (!valid)
                {
                    std::cerr << boost::geometry::wkt(mpoint) << std::endl;
                    std::cerr << red << "Invalid geometry ^:" <<  message << no_color << std::endl;
                    ++fail_count;
                }
                ++count;
                break;
            }
            case vtzero::GeomType::LINESTRING:
            {
                mapbox::geometry::multi_line_string<double> mline;
                linestring_processor proc_line(mline);
                vtzero::decode_linestring_geometry(feature.geometry(), false, proc_line);
                bool valid = boost::geometry::is_valid(mline, message);
                //bool simple = boost::geometry::is_simple(mline);
                if (!valid) //|| !simple)
                {
                    std::cerr << boost::geometry::wkt(mline) << std::endl;
                    //throw std::runtime_error("Invalid  geometry ^:" + message);
                    std::cerr << red << "Invalid geometry ^:" <<  message  << no_color << std::endl;
                    ++fail_count;
                }
                ++count;

                break;
            }
            case vtzero::GeomType::POLYGON:
            {
                mapbox::geometry::multi_polygon<double> mpoly;
                polygon_processor proc_poly(mpoly);
                vtzero::decode_polygon_geometry(feature.geometry(), false, proc_poly);
                bool valid = boost::geometry::is_valid(mpoly, message);
                //bool simple = boost::geometry::is_simple(mpoly);
                if (!valid)// || !simple)
                {
                    std::cerr << boost::geometry::wkt(mpoly) << std::endl;
                    //throw std::runtime_error("Invalid  geometry ^:" + message);
                    std::cerr << red << "Invalid geometry ^:" <<  message << no_color << std::endl;
                    ++fail_count;
                }
                ++count;
                break;
            }
            default:
                std::cerr << "UNKNOWN GEOMETRY TYPE\n";
                break;
            }
        }

    }
    std::cerr << "Processed " << count << " features invalid geometries:" << fail_count << std::endl;
    return EXIT_SUCCESS;
}
