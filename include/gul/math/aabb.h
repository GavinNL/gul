#pragma once
#ifndef GUL_AABB_H
#define GUL_AABB_H

#include <cstdint>
#include <stdexcept>
#include <glm/glm.hpp>

namespace gul
{

template< typename _T, size_t _dim>
class aabb_t
{
public:
    using value_type = _T;
    using vec_type   = glm::vec< static_cast<int>(_dim), value_type, glm::defaultp>;// array_type;

    /// Lower bound of AABB in each dimension.
    vec_type lowerBound;

    /// Upper bound of AABB in each dimension.
    vec_type upperBound;

    aabb_t()
    {
    }

    static constexpr size_t dimensions()
    {
       return _dim;
    }

    aabb_t(const vec_type& lowerBound_,
           const vec_type& upperBound_) : lowerBound(lowerBound_),
                                          upperBound(upperBound_)
    {
        // Validate that the upper bounds exceed the lower bounds.
        const int32_t dim = static_cast<int32_t>(dimensions());
        for (int32_t i=0;i < dim ;i++)
        {
            // Validate the bound.
            if (lowerBound[i] > upperBound[i])
            {
                throw std::invalid_argument("[ERROR]: AABB lower bound is greater than the upper bound!");
            }
        }
    }

    value_type computeSurfaceArea() const
    {
        // Sum of "area" of all the sides.
        value_type sum = 0;

        // General formula for one side: hold one dimension constant
        // and multiply by all the other ones.
        const int32_t dim = static_cast<int32_t>(dimensions());
        for (int32_t d1 = 0; d1 < dim; d1++)
        {
            // "Area" of current side.
            value_type product = 1;

            for (int32_t d2 = 0; d2 < dim; d2++)
            {
                if (d1 == d2)
                    continue;

                value_type dx = upperBound[d2] - lowerBound[d2];
                product *= dx;
            }

            // Update the sum.
            sum += product;
        }

        return 2 * sum;
    }

    void translate(vec_type const & v)
    {
        lowerBound += v;
        upperBound += v;
    }
//    static aabb_t merge(const aabb_t & aabb1, const aabb_t & aabb2)
//    {
//        aabb_t out;
//        for (uint32_t i=0;i<dimensions();i++)
//        {
//            out.lowerBound[i] = std::min(aabb1.lowerBound[i], aabb2.lowerBound[i]);
//            out.upperBound[i] = std::max(aabb1.upperBound[i], aabb2.upperBound[i]);
//        }
//        return out;
//    }

    bool contains(const aabb_t & aabb) const
    {
        int d = static_cast<int>(dimensions());
        for (int i=0;i<d;i++)
        {
            if (aabb.lowerBound[i] < lowerBound[i]) return false;
            if (aabb.upperBound[i] > upperBound[i]) return false;
        }

        return true;
    }

    bool contains(const vec_type & p) const
    {
        for (uint32_t i=0;i<p.length();i++)
        {
            if (p[i] < lowerBound[i]) return false;
            if (p[i] > upperBound[i]) return false;
        }

        return true;
    }

    void expand(const vec_type & p)
    {
        lowerBound = glm::min(lowerBound, p);
        upperBound = glm::max(upperBound, p);
    }

    void expand(const aabb_t & p)
    {
        expand( p.lowerBound );
        expand( p.upperBound);
    }

    void scale(vec_type const & p)
    {
        auto W = (upperBound-lowerBound) * 0.5f;
        auto M = computeCentre();
        W *= p;
        upperBound = M + W;
        lowerBound = M - W;
    }

    bool overlaps(const aabb_t & aabb, bool touchIsOverlap) const
    {
        bool rv = true;
        const int32_t dim = static_cast<int32_t>(dimensions());
        if (touchIsOverlap)
        {
            for (int32_t i = 0; i < dim ; ++i)
            {
                if (aabb.upperBound[i] < lowerBound[i] || aabb.lowerBound[i] > upperBound[i])
                {
                    rv = false;
                    break;
                }
            }
        }
        else
        {
            for (int32_t i = 0; i < dim; ++i)
            {
                if (aabb.upperBound[i] <= lowerBound[i] || aabb.lowerBound[i] >= upperBound[i])
                {
                    rv = false;
                    break;
                }
            }
        }

        return rv;
    }

    vec_type computeCentre() const
    {
        vec_type position;

        int32_t dim = static_cast<int32_t>(dimensions());
        for ( int32_t i=0;i<dim;i++)
            position[i] = static_cast<value_type>(0.5) * (lowerBound[i] + upperBound[i]);

        return position;
    }

    aabb_t<_T, _dim> transform(const glm::mat4 & M) const
    {
        glm::vec4 p[] =
        {
            M*glm::vec4(lowerBound.x, lowerBound.y, lowerBound.z,1.0f),
            M*glm::vec4(lowerBound.x, lowerBound.y, upperBound.z,1.0f),
            M*glm::vec4(lowerBound.x, upperBound.y, lowerBound.z,1.0f),
            M*glm::vec4(lowerBound.x, upperBound.y, upperBound.z,1.0f),
            M*glm::vec4(upperBound.x, lowerBound.y, lowerBound.z,1.0f),
            M*glm::vec4(upperBound.x, lowerBound.y, upperBound.z,1.0f),
            M*glm::vec4(upperBound.x, upperBound.y, lowerBound.z,1.0f),
            M*glm::vec4(upperBound.x, upperBound.y, upperBound.z,1.0f)
        };

        aabb_t<_T, _dim> out;
        out.lowerBound = p[0];
        out.upperBound = p[0];

        for(int i=0 ; i < 7 ; i++)
        {
            out.lowerBound = glm::min( out.lowerBound, vec_type(p[i])  );
            out.upperBound = glm::max( out.upperBound, vec_type(p[i])  );
        }

        return out;
    }
};

template<typename _T, size_t _dim>
inline aabb_t<_T,_dim> merge(const aabb_t<_T,_dim> & aabb1, const aabb_t<_T,_dim> & aabb2)
{
    aabb_t<_T,_dim> out;
    constexpr int d = static_cast<int>(_dim);
    for (int i=0;i<d;i++)
    {
        out.lowerBound[i] = std::min(aabb1.lowerBound[i], aabb2.lowerBound[i]);
        out.upperBound[i] = std::max(aabb1.upperBound[i], aabb2.upperBound[i]);
    }
    return out;
}

template<typename _T, size_t _dim>
inline aabb_t<_T,_dim> contains(const aabb_t<_T,_dim> & a1, const aabb_t<_T,_dim> & a2)
{
    for (uint32_t i=0;i<_dim;i++)
    {
        if (a2.lowerBound[i] < a1.lowerBound[i]) return false;
        if (a2.upperBound[i] > a1.upperBound[i]) return false;
    }

    return true;
}

template<typename _T, size_t _dim>
inline aabb_t<_T,_dim> overlaps(const aabb_t<_T,_dim> & a1, const aabb_t<_T,_dim> & a2, bool touchIsOverlap)
{
    bool rv = true;

    if (touchIsOverlap)
    {
        for (uint32_t i = 0; i < _dim ; ++i)
        {
            if (a2.upperBound[i] < a1.lowerBound[i] || a2.lowerBound[i] > a1.upperBound[i])
            {
                rv = false;
                break;
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < _dim; ++i)
        {
            if (a2.upperBound[i] <= a1.lowerBound[i] || a2.lowerBound[i] >= a1.upperBound[i])
            {
                rv = false;
                break;
            }
        }
    }

    return rv;
}

using AABB = aabb_t<float, 3>;

using bb3f = aabb_t<float,3>;
using bb2f = aabb_t<float,2>;

template<typename T, size_t _dim>
bool intersects(aabb_t<T,_dim> const & A, aabb_t<T,_dim> const & B)
{
    return A.overlaps(B,true);
}

}

#endif
