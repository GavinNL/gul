/*
 * A Transform_t class represents a spatial position and an
 * orientation.
 *
 */
#ifndef GUL_MATH_Transform_t_H
#define GUL_MATH_Transform_t_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace gul
{

/**
 * @brief The Transform_t struct
 *
 * The Transform_t class is similar to a Matrix Transform_t, but allows
 * you to provide the position,rotation and scaling factors instead
 * of setting up a full matrix.
 *
 */
template<typename _T=float>
struct Transform_t
{
    using value_type = _T;
    using vec_type  = glm::vec<3, value_type, glm::defaultp>;
    using quat_type = glm::qua<value_type, glm::defaultp>;
    using mat4_type = glm::mat<4, 4, value_type, glm::defaultp>;

    vec_type  position;
    quat_type rotation;
    vec_type  scale;


    explicit constexpr Transform_t() : position({0,0,0}),
                                                                 rotation({1.f,0.f,0.f,0.f}),
                                                                 scale({1,1,1})
    {
    }


    template<typename _t1>
    explicit constexpr Transform_t(glm::vec<3, _t1, glm::defaultp> const & _position) : position(_position),
                                                                 rotation({1.f,0.f,0.f,0.f}),
                                                                 scale({1,1,1})
    {
    }

    template<typename _t1>
    explicit constexpr Transform_t(glm::qua<_t1, glm::defaultp> const & _rotation) : position({0,0,0}),
                                                                 rotation(_rotation),
                                                                 scale({1,1,1})
    {
    }

    template<typename _t1>
    explicit constexpr Transform_t(glm::vec<3, _t1, glm::defaultp> const & _position,
                                   glm::qua<_t1, glm::defaultp> const & _rotation,
                                   glm::vec<3, _t1, glm::defaultp> const & _scale) : position(_position),
                                                                 rotation(_rotation),
                                                                 scale(_scale)
    {
    }

    template<typename _P>
    constexpr Transform_t(Transform_t<_P> const & P) : position(P.position),
                                                       rotation(P.rotation),
                                                       scale(P.scale)
    {
    }


    Transform_t(mat4_type const & M)
    {
        vec_type skew;
        typename mat4_type::col_type perspective;
        glm::decompose(M, scale, rotation, position, skew,perspective);
    }

    /**
     * @brief identity
     * @return
     *
     * Returns the identity Transform_t
     */
    static constexpr Transform_t identity()
    {
        return Transform_t();
    }

    /**
     * @brief translate
     * @param T
     * @return
     *
     * Translate the Transform_t by some vector
     */
    Transform_t& translate(vec_type const & T)
    {
        position += T;
        return *this;
    }

    /**
     * @brief translateLocal
     * @param direction
     * @return
     *
     * Translates the Transform_t based on the rotation of the
     * current Transform_t.
     */
    Transform_t& translateLocal(const vec_type & direction)
    {
        return translate( rotation * direction);
    }

    /**
     * @brief rotateGlobal
     * @param axis
     * @param AngleRadians
     * @return
     *
     * Rotate the Transform_t around a global axis by some angle
     */
    Transform_t& rotateGlobal(const vec_type & axis, value_type AngleRadians)
    {
        return rotateLocal(glm::conjugate(rotation) * axis, AngleRadians);
        return *this;
    }

    /**
     * @brief rotateLocal
     * @param axis
     * @param AngleRadians
     * @return
     *
     * Rotate the the Transform_t around a vector relative to
     * the local rotation of the Transform_t.
     */
    Transform_t& rotateLocal(const vec_type & axis, value_type AngleRadians)
    {
        rotation = glm::rotate( rotation, AngleRadians, axis );
        return *this;
    }

    /**
     * @brief setEuler
     * @param PitchYawRoll
     * @return
     *
     * Set the rotation using the euler angles
     */
    Transform_t& setEuler( const vec_type & PitchYawRoll )
    {
        rotation = quat_type(PitchYawRoll);
        return *this;
    }

    /**
     * @brief getMatrix
     * @return
     *
     * Returns the Transform_t as a Matrix
     */
    mat4_type getMatrix() const
    {
    #if 1
        auto M = mat4_cast(rotation);
        M[0] *= scale[0];
        M[1] *= scale[1];
        M[2] *= scale[2];
        M[3] = typename mat4_type::col_type(position,1.0f);
        return M;
    #else
        //return glm::translate(position) * mat4_type_cast(rotation) * glm::scale( mat4_type(1.0), scale);
        return glm::translate(  mat4_type(1.0f), position) * mat4_type_cast(rotation) * glm::scale( mat4_type(1.0), scale);
    #endif     
    }

    /**
     * @brief getViewMatrix
     * @return
     *
     * Returns the Transform_t as a view matrix. This is
     * used mostly for Computer Graphics, it is different
     * than getMatrix()
     *
     * The returned matrix is the camera matrix as if the
     * camera was looking down the +z axis of the Transform_t.
     */
    mat4_type getViewMatrix() const
    {
        return glm::lookAt( position, position + rotation * vec_type(0,0,1), rotation * vec_type(0,1,0) );
    }

    /**
     * @brief reverse
     * @return
     *
     * Returns the reverse quaternion of the Transform_t's rotation
     */
    quat_type reverse() const
    {
        return quat_type(rotation.w, -rotation.x,  -rotation.y, -rotation.z);
    }

    /**
     * @brief lookat
     * @param at
     * @param up
     * @return
     *
     * Rotate the Transform_t so it looks at a particular point.
     */
    Transform_t& lookat( vec_type const & pointToLookAt, vec_type const & up)
    {
#if 1
        rotation = glm::quatLookAt( glm::normalize(position-pointToLookAt) , up);
#else
        vec_type z = -glm::normalize(position-pointToLookAt);
        vec_type x = glm::normalize(glm::cross(up,z));
        vec_type y = glm::cross(z,x);

        glm::mat3 R(x,y,z);
        rotation = quat_type_cast(R);
#endif
        return *this;
    }


    // return the x/y/z axies of the Transform_t.
    // ie: the direction the local direction of the x-axis
    vec_type xAxis() const
    {
        return rotation * vec_type(1,0,0);
    }
    vec_type yAxis() const
    {
        return rotation * vec_type(0,1,0);
    }
    vec_type zAxis() const
    {
        return rotation * vec_type(0,0,1);
    }

    // returns various directions
    vec_type forward() const
    {
        return zAxis();
    }
    vec_type back() const
    {
        return -forward();
    }

    vec_type left() const
    {
        return xAxis();
    }
    vec_type right() const
    {
        return -left();
    }

    vec_type up() const
    {
        return yAxis();
    }
    vec_type down() const
    {
        return -up();
    }

    // A few constant Transform_ts which provide
    // rotations around paricular axes
    static constexpr Transform_t<value_type> R90x()
    {
        return Transform_t<value_type>( quat_type( { glm::half_pi<value_type>() ,0,0} ));
    }
    static constexpr Transform_t<value_type> R180x()
    {
        return Transform_t<value_type>(  quat_type( { glm::pi<value_type>() ,0,0} ));
    }
    static constexpr Transform_t<value_type> R270x()
    {
        return Transform_t<value_type>(  quat_type( { -glm::half_pi<value_type>() ,0,0} ));
    }
    static constexpr Transform_t<value_type> R90y()
    {
        return  Transform_t<value_type>(  quat_type( { 0,glm::half_pi<value_type>() ,0} ));
    }
    static constexpr Transform_t<value_type> R180y()
    {
        return Transform_t<value_type>(  quat_type( { 0,glm::pi<value_type>() ,0} ));
    }
    static constexpr Transform_t<value_type> R270y()
    {
        return Transform_t<value_type>(  quat_type( { 0,-glm::half_pi<value_type>() ,0} ));
    }
    static constexpr Transform_t<value_type> R90z()
    {
        return  Transform_t<value_type>(  quat_type( { 0,0,glm::half_pi<value_type>() } ));
    }
    static constexpr Transform_t<value_type> R180z()
    {
        return Transform_t<value_type>(  quat_type( { 0,0,glm::pi<value_type>() } ));
    }
    static constexpr Transform_t<value_type> R270z()
    {
        return Transform_t( quat_type( { 0,0,-glm::half_pi<value_type>() } ));
    }
};


/**
 * @brief mix
 * @param L
 * @param R
 * @param t
 * @return
 *
 * performs the equivelant of glm::mix(  ), smoothly interpolates
 * the Transform_t from L to R
 */
template<typename T>
inline Transform_t<T> mix( const Transform_t<T> & L, const Transform_t<T> & R, typename Transform_t<T>::value_type t)
{
    return Transform_t<T>{
        glm::mix(L.position, R.position, t),
        glm::slerp(L.rotation, R.rotation, t),
        glm::mix(L.scale, R.scale,t)
    };
}


/**
 * @brief operator *
 * @param ps
 * @param ls
 * @return
 *
 * Transform a vector
 *
 * If you need to perform the same transformation on multiple vectors, it would be faster to
 * first get the matrix representation using getMatrix() then multiply the matrix by the vector.
 *
 */
template<typename T>
inline typename Transform_t<T>::vec_type operator * (const Transform_t<T> & ps, const typename  Transform_t<T>::vec_type &ls)
{
    return ps.position + glm::rotate(ps.rotation, ps.scale*ls);
}

template<typename T>
inline Transform_t<T> operator * (const Transform_t<T> & ps, const Transform_t<T> & ls)
{
    return
    Transform_t<T>(
                ps.position  + ps.rotation * (ps.scale * ls.position),
                ps.rotation * ls.rotation,
                ps.scale * ls.scale
    );
}

template<typename T>
inline Transform_t<T>& operator *= ( Transform_t<T> & ps,  Transform_t<T> const & ls)
{
    ps = ps * ls;

    return ps;

}

using Transform = Transform_t<float>;

using fTransform = Transform_t<float>;
using dTransform = Transform_t<double>;

}

#endif // Transform_t_H

