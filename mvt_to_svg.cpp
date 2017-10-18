#include <iostream>
#include <fstream>
#include <sstream>
#include <mapbox/geometry/geometry.hpp>
#include <mapbox/geometry/algorithms/detail/boost_adapters.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
// vtzero
#include <vtzero/vector_tile.hpp>
//emscripten
#include <emscripten.h>
#include <emscripten/bind.h>
// MVT tools
#include "mvt_utils.hpp"

namespace {

template <typename Mapper, typename Colors>
struct geometry_to_svg
{
    using result_type = void;
    geometry_to_svg(Mapper & mapper, Colors const& colors)
        : mapper_(mapper),
          colors_(colors) {}

    template <typename T>
    void operator() (mapbox::geometry::point<T> const& pt)
    {
        mapper_.add(pt);
        mapper_.map(pt,"fill-opacity:1.0;fill:orange;stroke:red;stroke-width:0.5", 3.0);
    }

    template <typename T>
    void operator() (mapbox::geometry::multi_point<T> const& mpoint)
    {
        mapper_.add(mpoint);
        mapper_.map(mpoint,"fill-opacity:1.0;fill:orange;stroke:red;stroke-width:0.5", 3.0);
    }

    template <typename T>
    void operator() (mapbox::geometry::line_string<T> const& line)
    {
        mapper_.add(line);
        mapper_.map(line, "stroke:orange;stroke-width:2.0;stroke-opacity:1;comp-op:color-dodge");
    }

    template <typename T>
    void operator() (mapbox::geometry::multi_line_string<T> const& mline)
    {
        mapper_.add(mline);
        mapper_.map(mline, "stroke:orange;stroke-width:2.0;stroke-opacity:1;comp-op:color-dodge");
    }

    template <typename T>
    void operator() (mapbox::geometry::polygon<T> const& poly)
    {
        mapper_.add(poly);
        std::string style = "stroke:blue;stroke-width:1; fill-opacity:0.3;fill:" + colors_[count_++ % colors_.size()];
        mapper_.map(poly, style);
    }

    template <typename T>
    void operator() (mapbox::geometry::multi_polygon<T> const& mpoly)
    {
        mapper_.add(mpoly);
        std::string style = "stroke:blue;stroke-width:1; fill-opacity:0.3;fill:" + colors_[count_++ % colors_.size()];
        mapper_.map(mpoly, style);
    }

    template <typename T>
    void operator() (T const& ) {}

    std::size_t count() const { return count_; }
    Mapper & mapper_;
    Colors const& colors_;
    std::size_t count_ = 0;
};

}

#if 0
std::string decompress(std::string const& str)
{
    std::stringstream in(str), out;
    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
    inbuf.push(boost::iostreams::gzip_decompressor());
    inbuf.push(in);
    boost::iostreams::copy(inbuf, out);
    return out.str();
}
#endif

EMSCRIPTEN_KEEPALIVE
std::string  mvt_to_svg(std::string const& str)
{
    std::ostringstream output;
    try
    {
        //std::string decomp = decompress(str);
        vtzero::data_view view(str.data(), str.length());
        vtzero::vector_tile tile{view};
        std::array<std::string, 4> four_colors = {{"red", "blue", "green", "yellow"}};
        using mapper_type = boost::geometry::svg_mapper<mapbox::geometry::point<double>>;
        boost::geometry::svg_mapper<mapbox::geometry::point<double>> mapper(
            output,
            2048, 2048,
            "width=\"1024\" height=\"1024\" viewBox=\"0 0 4096 4096\"");
        // add tile boundary
        mapbox::geometry::line_string<double> tile_outline{{0, 0}, {0, 4096}, {4096, 4096}, {4096,0}, {0, 0}};
        mapper.add(tile_outline);
        mapper.map(tile_outline,"stroke:blue;stroke-width:1;stroke-dasharray:4,4;comp-op:multiply");
        geometry_to_svg<mapper_type, std::array<std::string, 4>> svg(mapper, four_colors);
        while (auto layer = tile.next_layer())
        {
            while (auto feature = layer.next_feature())
            {
                auto geom = detail::make_geometry<double>(feature);
                mapbox::util::apply_visitor(svg, geom);
            }
        }
        std::cerr << "Processed " << svg.count() << " features" << std::endl;
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
