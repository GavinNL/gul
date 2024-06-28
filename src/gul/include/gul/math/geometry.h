/*
 * MIT License
 *
 * Copyright (c) [year] [fullname]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef GUL_GEOMETRY_H
#define GUL_GEOMETRY_H

#include <glm/glm.hpp>
#include <glm/gtx/closest_point.hpp>

namespace gul
{

template<typename T>
class point_t
{
    public:
    typedef T                                  value_type;
    typedef glm::tvec3<value_type, glm::highp> glm_type;

    point_t()
    {
    }

    point_t(value_type _x) : point_t(_x,_x,_x){}

    explicit point_t(value_type _x, value_type _y, value_type _z) : x(_x), y(_y), z(_z)
    {

    }

    explicit point_t( glm_type const & p) : point_t( p.x,p.y,p.z)
    {

    }

    value_type & operator[](int i)
    {
        return (&x)[i];
    }
    value_type const & operator[](int i) const
    {
        return (&x)[i];
    }

    glm_type asVec() const
    {
        return glm_type(x,y,z);
    }
    value_type x=value_type(0);
    value_type y=value_type(0);
    value_type z=value_type(0);
};

/**
 * @brief displacement
 * @param p
 * @param L
 * @return
 *
 * Returns the vector which starts on p1 and ends on p2
 */
template<typename T>
inline glm::tvec3<T, glm::highp> displacement(point_t<T> const & p1, point_t<T> const & p2)
{
    return glm::tvec3<T, glm::highp>( p2.x-p1.x , p2.y-p1.y, p2.z-p1.z);
}

template<typename T>
inline point_t<T>  operator+(point_t<T> const & p1, glm::tvec3<T, glm::highp> const & v)
{
    return point_t<T>( p1.x+v.x , p1.y+v.y, p1.z+v.z);
}

template<typename T>
inline glm::tvec3<T, glm::highp>  operator-(point_t<T> const & p1, point_t<T> const & p2)
{
    return displacement(p1,p2);
}

/**
 * @brief The line_t class
 *
 * A line is defined by a point and a vector direction.
 */
template<typename T>
class line_t
{
    public:
    typedef T                                  value_type;
    typedef glm::tvec3<value_type, glm::highp> vec_type;
    typedef point_t<value_type>                point_type;

    line_t()
    {
    }

    /**
     * @brief line_t
     * @param _p0
     * @param _p1
     *
     * Construct a line from two points.
     */
    line_t( point_type const & _p0, point_type const & _p1) : p(_p0), v( displacement(_p1,_p0) )
    {

    }

    line_t( point_type const & _p0, vec_type const & v1) : p(_p0), v(v1)
    {

    }

    /**
     * @brief closestPoint
     * @param x
     * @return
     *
     * Returns the point on the line that is closest to x
     */
    point_type closestPoint(point_type const & x) const
    {
        return point_type( glm::closestPointOnLine( x.asVec(), p.asVec(), p.asVec()+v) );
        auto r = p - x;
        //auto r = vec3( L.p.x - p.x , L.p.y -p.y , L.p.z - p.z);
        auto n = glm::normalize(v);
        return x + (r - glm::dot(r,n)*n);
    }

    point_type p = point_type{0,0,0};
    vec_type   v = vec_type{1,0,0};

};


template<typename T>
T length( line_t<T> const & L)
{
    return glm::length( L.v );
}

/**
 * @brief intersectingLine
 * @param L0
 * @param L1
 * @return
 *
 * Finds the line/ray that starts on L0 and ends on L1
 */
template<typename T>
line_t<T> intersectingLine( line_t<T> const & L0, line_t<T> const & L1 )
{
    auto & d1 = L0.v;
    auto & d2 = L1.v;

    auto & p1 = L0.p;
    auto & p2 = L1.p;

    auto n  = glm::cross( d1, d2);

    auto n1 = glm::cross(d1, n);
    auto n2 = glm::cross(d2, n);

    auto t = -glm::dot(p2-p1, n2 ) / glm::dot(d1,n2);
    auto s = -glm::dot(p1-p2, n1 ) / glm::dot(d2,n1);

    return line_t<T>( p1+t*d1, p2+s*d2);
}

/**
 * @brief distance
 * @param L0
 * @param L1
 * @return
 *
 * Returns the distance between two lines
 */
template<typename T>
T distance( line_t<T> const & L0, line_t<T> const & L1 )
{
    auto n = glm::cross(L0.v, L1.v);
    return glm::dot( L1.p - L0.p, n) / glm::length(n);
}


/**
 * @brief distance
 * @param L
 * @param p
 * @return
 *
 * Calculates the distance between a line and a point.
 */
template<typename T>
inline T distance(line_t<T> const & L, point_t<T> const & p)
{
    return glm::length( glm::cross( L.v, vec3(L.p.x - p.x, L.p.y-p.y,L.p.z-p.z)) ) / glm::length(L.v);
}




/**
 * @brief displacement
 * @param p
 * @param L
 * @return
 *
 * Returns a vector which when added to the point, p, will bring the point to line, L
 */
template<typename T>
inline glm::tvec3<T, glm::highp> displacement(point_t<T> const & p, line_t<T> const & L)
{
    auto r = vec3( L.p.x - p.x , L.p.y -p.y , L.p.z - p.z);
    auto n = glm::normalize(L.v);
    return r - glm::dot(r,n)*n;


}



/**
 * @brief displacement
 * @param L
 * @param p
 * @return
 *
 * Return the vector which is perpendicular to the Line L and points to the direction of p
 */
template<typename T>
inline glm::tvec3<T, glm::highp> displacement(line_t<T> const & L, point_t<T> const & p)
{
    return -displacement(p,L);
}



template<typename T>
class plane_t
{
    public:
    typedef T                                  value_type;
    typedef glm::tvec3<value_type, glm::highp> vec_type;
    typedef point_t<value_type>                point_type;

    plane_t()
    {
    }

    /**
     * @brief plane_t
     * @param _p0
     * @param _n
     *
     * Construct a plane from a point and a normal
     */
    plane_t( point_type const & _p0, vec_type const & _n) : n( glm::normalize(_n)), d( -glm::dot(n, vec3(_p0.x,_p0.y,_p0.z)))
    {

    }

    /**
     * @brief plane_t
     * @param a
     * @param b
     * @param c
     *
     * Construct a plane from 3 points.
     */
    plane_t( point_type const & a,
             point_type const & b,
             point_type const & c) : plane_t( a,  glm::cross( displacement(b,a), displacement(c,a)) )
    {

    }

    /**
     * @brief closestPoint
     * @param p
     * @return
     *
     * Returns the point on the plane that is closest to x
     */
    point_type closestPoint(point_type const & x) const
    {
        //auto const & n = p.n;
        //auto const & d = p.d;

        return x + (( d + glm::dot(n, vec3(x.x,x.y,x.z) ) )/glm::dot(n,n)) * n;
    }
    vec_type   n = vec_type{0,1,0};
    value_type d = 0;
};






/**
 * @brief displacement
 * @param p
 * @param P
 * @return
 *
 * Returns a vector when added to point x, will bring the point to plane P
 */
template<typename T>
inline glm::tvec3<T, glm::highp> displacement(point_t<T> const & x, plane_t<T> const & P)
{
    auto const & n = P.n;
    auto const & d = P.d;

    return (( d + glm::dot(n, vec3(x.x,x.y,x.z) ) )/glm::dot(n,n)) * n;
}

template<typename T>
inline glm::tvec3<T, glm::highp> displacement(plane_t<T> const & P, point_t<T> const & x)
{
    return -displacement(x,P);
}



/**
 * @brief distance
 * @param x
 * @param P
 * @return
 *
 * Returns the distance between the point x and the plane P
 */
template<typename T>
inline T distance(point_t<T> const & x, plane_t<T> const & P)
{
    auto const & n = P.n;
    auto const & d = P.d;

    return -(( d + glm::dot(n, vec3(x.x,x.y,x.z) ) )/glm::dot(n,n));
}

template<typename T>
inline T distance(plane_t<T> const & P, point_t<T> const & x)
{
    return distance(x,P);
}





/**
 * @brief intersection
 * @param P
 * @param L
 * @return
 *
 * Returns the intersection point of plane P and line L
 */
template<typename T>
inline point_t<T> intersection(plane_t<T> const & P, line_t<T> const & L)
{
    auto t = (-P.d - glm::dot( P.n, L.p.asVec())) / glm::dot(P.n,L.v);
    return point_t<T>( L.p.asVec() + t*L.v );
}



using point = point_t<float>;
using plane = plane_t<float>;
using line  = line_t<float>;

using point3 = point_t<float>;
using plane3 = plane_t<float>;
using line3  = line_t<float>;









template<typename T>
class box_t
{
    public:
    typedef T                                  value_type;
    typedef glm::tvec3<value_type, glm::highp> vec_type;
    typedef point_t<value_type>                point_type;

    box_t() : box_t( vec_type(0.5,0.5,0.5) )
    {
    }

    explicit box_t( vec_type const & _halfExtents) : half_extents(_halfExtents)
    {
    }

    /**
     * @brief plane_t
     * @param _p0
     * @param _n
     *
     * Construct a plane from a point and a normal
     */
    box_t( point_type const & _centre, vec_type const & _halfExtents) : centre(_centre), half_extents(_halfExtents)
    {

    }

    float operator()( vec_type const &p ) const
    {
        auto & b = half_extents;
        auto q = glm::abs(p) - b;
        return glm::length(glm::max(q,glm::vec3(0.0))) + glm::min(glm::max(q.x,glm::max(q.y,q.z)),0.0f);
    }

    point_type centre;
    vec_type   half_extents;
};

/**
 * @brief distance
 * @param L
 * @param p
 * @return
 *
 * Calculates the distance between a line and a point.
 */
template<typename T>
inline T distance(box_t<T> const & L, point_t<T> const & p)
{
    auto q = glm::abs(glm::vec3(p.x-L.centre.x,p.y-L.centre.y,p.z-L.centre.z) ) - L.half_extents;

    return glm::length( glm::max(q, static_cast<T>(0.0) )) + glm::min( glm::max( q.x, glm::max(q.y,q.z)), static_cast<T>(0.0) );
}



template<typename T>
inline bool intersects(box_t<T> const & b, line_t<T> const & r)
{
    T tmin = -INFINITY, tmax = INFINITY;

    auto b_min = b.centre.asVec() - b.half_extents;
    auto b_max = b.centre.asVec() + b.half_extents;

    if ( std::fabs(r.v.x) > static_cast<T>(0.001) )
    {
        auto tx1 = (b_min.x - r.p.x)/r.v.x;
        auto tx2 = (b_max.x - r.p.x)/r.v.x;

        tmin = glm::max(tmin, glm::min(tx1, tx2));
        tmax = glm::min(tmax, glm::max(tx1, tx2));
    }

    if ( std::fabs(r.v.y) > static_cast<T>(0.001) )
    {
        auto ty1 = (b_min.y - r.p.y)/r.v.y;
        auto ty2 = (b_max.y - r.p.y)/r.v.y;

        tmin = glm::max(tmin, glm::min(ty1, ty2));
        tmax = glm::min(tmax, glm::max(ty1, ty2));
    }

    if ( std::fabs(r.v.z) > static_cast<T>(0.001) )
    {
        auto ty1 = (b_min.z - r.p.z)/r.v.z;
        auto ty2 = (b_max.z - r.p.z)/r.v.z;

        tmin = glm::max(tmin, glm::min(ty1, ty2));
        tmax = glm::min(tmax, glm::max(ty1, ty2));
    }

    return tmax >= tmin;
}



template<typename T>
class sphere_t
{
    public:
    typedef T                                  value_type;
    typedef glm::tvec3<value_type, glm::highp> vec_type;
    typedef point_t<value_type>                point_type;

    sphere_t() : sphere_t( value_type(1) )
    {
    }

    explicit sphere_t( value_type _radius) : radius(_radius)
    {
    }

    /**
     * @brief plane_t
     * @param _p0
     * @param _n
     *
     * Construct a plane from a point and a normal
     */
    sphere_t( point_type const & _centre, value_type _radius) : centre(_centre), radius(_radius)
    {

    }

    point_type centre;
    value_type radius;
};

template<typename T>
inline bool intersects(sphere_t<T> const & b, line_t<T> const & r)
{
    return distance( r, point_t<T>(b.centre) ) <= b.radius;
}






}

#endif
