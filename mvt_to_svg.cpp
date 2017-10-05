#include <iostream>
#include <fstream>
#include <sstream>
#include <mapbox/geometry/geometry.hpp>
#include <mapbox/geometry/algorithms/detail/boost_adapters.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/optional/optional.hpp>
// vtzero
#include <vtzero/vector_tile.hpp>

//emscripten
#include <emscripten.h>
#include <emscripten/bind.h>

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
        mpoint_.emplace_back(point.x, 4096.0 - point.y);
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
        mline_.back().emplace_back(point.x, 4096.0 - point.y);
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
        ring_.emplace_back(point.x, 4096.0 - point.y);
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

EMSCRIPTEN_KEEPALIVE
std::string  mvt_to_svg(std::string const& str)
{
    std::ostringstream output;
    try
    {
        vtzero::data_view view(str.data(), str.length());
        vtzero::vector_tile tile{view};

        std::array<std::string, 4> four_colors = {{"red", "blue", "green", "yellow"}};
        std::size_t count = 0;
        {
            boost::geometry::svg_mapper<mapbox::geometry::point<double>> mapper(
                output,
                2048, 2048,
                "width=\"1024\" height=\"1024\" viewBox=\"0 0 4096 4096\"");
            // add tile boundary
            mapbox::geometry::line_string<double> tile_outline{{0, 0},{0, 4096},{4096, 4096}, {4096,0}, {0, 0}};
            mapper.add(tile_outline);
            mapper.map(tile_outline,"stroke:blue;stroke-width:1;stroke-dasharray:4,4;comp-op:multiply");

            boost::optional<mapbox::geometry::box<double>> bbox;
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
                        mapbox::geometry::box<double> b{{0,0}, {0,0}};
                        boost::geometry::envelope(mpoint, b);
                        if (!bbox) bbox = b;
                        else expand_to_include(*bbox, b);
                        mapper.add(mpoint);
                        mapper.map(mpoint,"fill-opacity:1.0;fill:orange;stroke:red;stroke-width:0.5", 3.0);
                        ++count;
                        break;
                    }
                    case vtzero::GeomType::LINESTRING:
                    {
                        mapbox::geometry::multi_line_string<double> mline;
                        linestring_processor proc_line(mline);
                        vtzero::decode_linestring_geometry(feature.geometry(), false, proc_line);
                        mapbox::geometry::box<double> b{{0,0}, {0,0}};
                        boost::geometry::envelope(mline, b);
                        if (!bbox) bbox = b;
                        else expand_to_include(*bbox, b);
                        mapper.add(mline);
                        mapper.map(mline, "stroke:orange;stroke-width:2.0;stroke-opacity:1;comp-op:color-dodge");
                        ++count;
                        break;
                    }
                    case vtzero::GeomType::POLYGON:
                    {
                        mapbox::geometry::multi_polygon<double> mpoly;
                        polygon_processor proc_poly(mpoly);
                        vtzero::decode_polygon_geometry(feature.geometry(), false, proc_poly);
                        mapbox::geometry::box<double> b{{0,0}, {0,0}};
                        boost::geometry::envelope(mpoly, b);
                        if (!bbox) bbox = b;
                        else expand_to_include(*bbox, b);
                        mapper.add(mpoly);
                        std::string style = "stroke:blue;stroke-width:1; fill-opacity:0.3;fill:" + four_colors[count % four_colors.size()];
                        mapper.map(mpoly, style);
                        ++count;
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
            mapbox::geometry::line_string<double> extent{
                {bbox->min.x, bbox->min.y},
                {bbox->min.x, bbox->max.y},
                {bbox->max.x, bbox->max.y},
                {bbox->max.x, bbox->min.y},
                {bbox->min.x, bbox->min.y}};
            std::cerr << bbox->min.x << "," << bbox->min.y << "," << bbox->max.x << "," << bbox->max.y << std::endl;
            std::cerr << "Processed " << count << " features" << std::endl;
        }
    }
    catch (std::exception const& ex)
    {
        std::cerr << ex.what() << std::endl;
        return "FAIL";
    }
    output.flush();
    return output.str();

}

using namespace emscripten;
EMSCRIPTEN_BINDINGS(geometry) {
    function("mvt_to_svg", &mvt_to_svg);
}
