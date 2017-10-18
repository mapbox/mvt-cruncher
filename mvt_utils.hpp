#pragma once

#include <mapbox/geometry/geometry.hpp>

namespace detail {

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
         mpoint_.emplace_back(point.x, 4096 -  point.y);
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
        mline_.back().emplace_back(point.x, 4096 - point.y);
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
        ring_.emplace_back(point.x, 4096 - point.y);
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


template <typename T>
mapbox::geometry::geometry<T> make_geometry(vtzero::feature const& f)
{
    using coordinate_type = T;
    mapbox::geometry::geometry<coordinate_type> geom = mapbox::geometry::point<coordinate_type>();
    switch (f.geometry_type())
    {
    case vtzero::GeomType::POINT:
    {
        mapbox::geometry::multi_point<coordinate_type> mpoint;
        point_processor proc_point(mpoint);
        vtzero::decode_point_geometry(f.geometry(), false, proc_point);
        if (mpoint.size() == 1) geom = std::move(mpoint.front());
        else geom = std::move(mpoint);
        break;
    }
    case vtzero::GeomType::LINESTRING:
    {
        mapbox::geometry::multi_line_string<coordinate_type> mline;
        linestring_processor proc_line(mline);
        vtzero::decode_linestring_geometry(f.geometry(), false, proc_line);
        if (mline.size() == 1) geom = std::move(mline.front());
        else geom = std::move(mline);
        break;
    }
    case vtzero::GeomType::POLYGON:
    {
        mapbox::geometry::multi_polygon<coordinate_type> mpoly;
        polygon_processor proc_poly(mpoly);
        vtzero::decode_polygon_geometry(f.geometry(), false, proc_poly);
        if (mpoly.size() == 1 ) geom = std::move(mpoly.front());
        else geom = std::move(mpoly);
        break;
    }
    default:
        break;
    }
    return geom;
}

struct assign_value
{
    assign_value(mapbox::geometry::value & val)
        : val_(val) {}

    template <typename T>
    void operator() (T && v)
    {
        val_ = v;
    }

    mapbox::geometry::value & val_;
};
#if 0
mapbox::geometry::property_map make_properties(vtzero::feature const& f)
{
    mapbox::geometry::property_map props;
    while (auto property = f.next_property())
    {
        std::string key {property.key().data(), property.key().size()};
        mapbox::geometry::value val;
        auto value_view = property.value();
        switch (value_view.type())
        {
        case vtzero::property_value_type::string_value:
        {
            std::string str(value_view.string_value().data(), value_view.string_value().size());
            val = std::move(str);
            break;
        }
        case vtzero::property_value_type::float_value:
            val = std::move(double(value_view.float_value()));
            break;
        case vtzero::property_value_type::double_value:
            val = std::move(value_view.double_value());
            break;
        case vtzero::property_value_type::int_value:
            val = std::move(value_view.int_value());
            break;
        case vtzero::property_value_type::uint_value:
            val = std::move(value_view.uint_value());
            break;
        case vtzero::property_value_type::sint_value:
            val = std::move(value_view.sint_value());
            break;
        default: // case vtzero::property_value_type::bool_value:
            val = std::move(value_view.bool_value());
            break;
        }
        props.emplace(std::move(key), std::move(val));
    }
    return props;
}

template <typename T>
struct feature_iterator : public boost::iterator_facade<feature_iterator<T>,
                                                        mapbox::geometry::feature<T>,
                                                        boost::forward_traversal_tag,
                                                        mapbox::geometry::feature<T>>
{
    using coordinate_type = T;
    using value_type = mapbox::geometry::feature<coordinate_type>;
    feature_iterator(vtzero::layer const& layer, bool begin = true)
        : itr_(begin ? layer.begin() : layer.end()) {}
private:
    friend class boost::iterator_core_access;
    void increment() { ++itr_;}
    void decrement() { --itr_;}
    void advance(typename boost::iterator_difference<feature_iterator<coordinate_type>>::type) {} // no-op
    bool equal(feature_iterator<coordinate_type> const& other) const
    {
        return (itr_ == other.itr_);
    }

    value_type dereference() const
    {
        auto geom = make_geometry<coordinate_type>(*itr_);
        auto props = make_properties(*itr_);
        return mapbox::geometry::feature<coordinate_type>(std::move(geom), std::move(props));
    }
    vtzero::layer_iterator itr_;
};
#endif
}
