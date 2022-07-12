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

#ifndef GUL_MATH_FRUSTRUM_H
#define GUL_MATH_FRUSTRUM_H

#include "geometry.h"
#include "aabb.h"

namespace gul
{
/**
 * @brief The frustum_t struct
 *
 * A class which represents a frustum in 3d space. It can be used to check clipping
 * with bounding boxes
 */
template<typename T>
struct frustum_t
{
    using value_type = T;
    using vec_type   = glm::tvec3<value_type, glm::highp>;
    using vec4_type  = glm::tvec4<value_type, glm::highp>;
    using mat_type   = glm::tmat4x4<value_type, glm::highp>;
    using point_type = point_t<value_type>;
    using line_type  = line_t<value_type>;
    using plane_type = plane_t<value_type>;
    using aabb_type  = aabb_t<value_type, 3>;

    frustum_t()
    {

    }

    /**
     * @brief Frustum
     * @param proj
     *
     * Constructs the frustrum from a perspective projection matrix.
     * The Frustum will have a default position of (0,0,0) point down the
     * negative Z axis
     *
     */
    frustum_t(const mat_type & proj)
    {
        const auto & m = proj;
        left.v.x = -(m[0][3] + m[0][0]);
        left.v.y = -(m[1][3] + m[1][0]);
        left.v.z = -(m[2][3] + m[2][0]);
        left.p   = point_type(glm::vec3(0.0f));
        left.v   = glm::normalize(left.v);

        // right
        right.v.x = -(m[0][3] - m[0][0]);
        right.v.y = -(m[1][3] - m[1][0]);
        right.v.z = -(m[2][3] - m[2][0]);
        right.p = point_type(glm::vec3(0.0f));
        right.v = glm::normalize(right.v);

        // bottom
        bottom.v.x = -(m[0][3] + m[0][1]);
        bottom.v.y = -(m[1][3] + m[1][1]);
        bottom.v.z = -(m[2][3] + m[2][1]);
        bottom.p = point_type(glm::vec3(0.0f));
        bottom.v = glm::normalize(bottom.v);

        // top
        top.v.x = -(m[0][3] - m[0][1]);
        top.v.y = -(m[1][3] - m[1][1]);
        top.v.z = -(m[2][3] - m[2][1]);
        top.p   = point_type(glm::vec3(0.0f));
        top.v = glm::normalize(top.v);

        // near
        near.v.x = m[0][3] + m[0][2];
        near.v.y = m[1][3] + m[1][2];
        near.v.z = m[2][3] + m[2][2];
        near.p =  point_type( glm::vec3(0,0, -(m[3][3] + m[3][2])/near.v.z) );
        near.v = -glm::normalize(near.v);

        // far
        far.v.x = m[0][3] - m[0][2];
        far.v.y = m[1][3] - m[1][2];
        far.v.z = m[2][3] - m[2][2];
        far.p = point_type( glm::vec3(0,0, -(m[3][3] - m[3][2]) / far.v.z) );
        far.v = -glm::normalize(far.v);
    }

    // Transform the fustrum using a matrix
    // The matrix M needs to be unitary
    void transform(const mat_type & ViewMatrix)
    {
        line_type * L = &top;

        for(int i=0;i<6;i++)
        {
            auto p4 = ViewMatrix * vec4_type( L[i].p.x, L[i].p.y, L[i].p.z, 1.0f);
            auto v4 = ViewMatrix * vec4_type( L[i].v.x, L[i].v.y, L[i].v.z, 0.0f);

            L[i] = line_type( point_type(p4.x,p4.y,p4.z), vec_type(v4.x,v4.y,v4.z));
        }

        auto p4 = ViewMatrix * vec4_type( p.x, p.y, p.z, 1.0f);
        p = point_type(vec_type(p4.x,p4.y,p4.z));
    }


    /**
     * @brief Intersects
     * @param B
     * @return
     *
     * Determines if a bounding box intersects the fustrum
     */
    bool intersects(aabb_type const & B) const
    {
        // for each plane, check if the corners of the bounding box are on the same side of the plane
        line_type const * L = &top;

        const vec_type P[] =
        {
            vec_type( B.lowerBound.x, B.lowerBound.y, B.lowerBound.z),
            vec_type( B.lowerBound.x, B.lowerBound.y, B.upperBound.z),
            vec_type( B.lowerBound.x, B.upperBound.y, B.lowerBound.z),
            vec_type( B.lowerBound.x, B.upperBound.y, B.upperBound.z),
            vec_type( B.upperBound.x, B.lowerBound.y, B.lowerBound.z),
            vec_type( B.upperBound.x, B.lowerBound.y, B.upperBound.z),
            vec_type( B.upperBound.x, B.upperBound.y, B.lowerBound.z),
            vec_type( B.upperBound.x, B.upperBound.y, B.upperBound.z)
        };

        // for each plane
        for(int i=0 ; i<6 ; i++)
        {
            int c = 0;
            auto oneOverL = 1.0f / glm::length(L[i].v);
            auto Lip = L[i].p.asVec();
            c += (glm::dot( P[0]-Lip, L[i].v) * oneOverL ) > 0;
            c += (glm::dot( P[1]-Lip, L[i].v) * oneOverL ) > 0;
            c += (glm::dot( P[2]-Lip, L[i].v) * oneOverL ) > 0;
            c += (glm::dot( P[3]-Lip, L[i].v) * oneOverL ) > 0;
            c += (glm::dot( P[4]-Lip, L[i].v) * oneOverL ) > 0;
            c += (glm::dot( P[5]-Lip, L[i].v) * oneOverL ) > 0;
            c += (glm::dot( P[6]-Lip, L[i].v) * oneOverL ) > 0;
            c += (glm::dot( P[7]-Lip, L[i].v) * oneOverL ) > 0;

            if(c==8) return false;
        }
        return true;
    }


    /**
     * @brief Intersects
     * @param point
     * @return true if the point is within the furstrum
     */
    bool intersects(const glm::vec3 & P ) const
    {
        line_type const * L = &top;

        for(int i=0;i<6;i++)
        {
            const float d = glm::dot( P-L[i].p, L[i].v) / glm::length(L[i].v);
            if( d > 0) return false;
        }
        return true;
    }



    const glm::vec3 & get_position() const
    {
        return p;
    }

    plane_type getNear() const
    {
        return plane_type( near.p, near.v);
    }
    plane_type getFar() const
    {
        return plane_type( point_type(2.0f*p.asVec() - far.p.asVec()), far.v);
    }
    plane_type getTop() const
    {
        return plane_type( point_type(p), top.v);
    }
    plane_type getRight() const
    {
        return plane_type( point_type(p), right.v);
    }
    plane_type getBottom() const
    {
        return plane_type( point_type(p), bottom.v);
    }
    plane_type getLeft() const
    {
        return plane_type( point_type(p), left.v);
    }

public:
    point_type p; // position of the camera in 3d space. The position of the point of the pyrimad

    // the 6 planes that define the fustrum
    // these planes are in real coordinate space. not normalized.
    line_type top;    // this plane passes through the point, p,
    line_type right;  // this plane passes through the point, p,
    line_type bottom; // this plane passes through the point, p,
    line_type left;   // this plane passes through the point, p,
    line_type near;
    line_type far;

};

using frustum = frustum_t<float>;

template<typename T>
bool intersects(frustum_t<T> const & F, typename frustum_t<T>::aabb_type const & bb)
{
    return F.intersects(bb);
}
template<typename T>
bool intersects(typename frustum_t<T>::aabb_type const & bb, frustum_t<T> const & F)
{
    return F.intersects(bb);
}

}

#endif

