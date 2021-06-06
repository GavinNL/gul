/*
 * A Transform class represents a spatial position and an
 * orientation.
 *
 */
#ifndef GUL_MATH_TRANSFORM_H
#define GUL_MATH_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace gul
{

struct Transform;

/**
 * @brief operator *
 * @param ps
 * @param ls
 * @return
 *
 * Operator overload for * so that it acts similar to a matrix vector
 * product.
 *
 */
glm::vec3 operator * (const Transform & ps, const glm::vec3 & ls);

/**
 * @brief The Transform struct
 *
 * The Transform class is similar to a Matrix transform, but allows
 * you to provide the position,rotation and scaling factors instead
 * of setting up a full matrix.
 *
 */
struct Transform
{
    glm::vec3    position;
    glm::quat    rotation;
    glm::vec3    scale;

    constexpr Transform(glm::vec3 const & _position={0.f,0.f,0.f},
                        glm::quat const & _rotation = {1.f,0.f,0.f,0.f},
                        glm::vec3 const & _scale = {1.f,1.f,1.f}) : position(_position),
                                                                    rotation(_rotation),
                                                                    scale(_scale)
    {
    }

    /**
     * @brief identity
     * @return
     *
     * Returns the identity transform
     */
    static constexpr Transform identity()
    {
        return Transform();
    }

    /**
     * @brief translate
     * @param T
     * @return
     *
     * Translate the transform by some vector
     */
    Transform& translate(glm::vec3 const & T)
    {
        position += T;
        return *this;
    }

    /**
     * @brief translateLocal
     * @param direction
     * @return
     *
     * Translates the transform based on the rotation of the
     * current transform.
     */
    Transform& translateLocal(const glm::vec3 & direction)
    {
        return translate( rotation * direction);
    }

    /**
     * @brief rotateGlobal
     * @param axis
     * @param AngleRadians
     * @return
     *
     * Rotate the Transform around a global axis by some angle
     */
    Transform& rotateGlobal(const glm::vec3 & axis, float AngleRadians)
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
     * Rotate the the Transform around a vector relative to
     * the local rotation of the Transform.
     */
    Transform& rotateLocal(const glm::vec3 & axis, float AngleRadians)
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
    Transform& setEuler( const glm::vec3 & PitchYawRoll )
    {
        rotation = glm::quat(PitchYawRoll);
        return *this;
    }

    /**
     * @brief getMatrix
     * @return
     *
     * Returns the Transform as a Matrix
     */
    glm::mat4 getMatrix() const
    {
#if defined USE_ANGLE_AXIS
        const float angle    = glm::angle(rotation);
        const glm::vec3 axis = glm::axis(rotation);

        return glm::scale( glm::rotate( glm::translate(  glm::mat4(1.0f), position), angle, axis), scale);
#else
        //return glm::translate(position) * glm::mat4_cast(rotation) * glm::scale( glm::mat4(1.0), scale);
        return glm::translate(  glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale( glm::mat4(1.0), scale);
#endif
    }

    /**
     * @brief getViewMatrix
     * @return
     *
     * Returns the Transform as a view matrix. This is
     * used mostly for Computer Graphics, it is different
     * than getMatrix()
     *
     * The returned matrix is the camera matrix as if the
     * camera was looking down the +z axis of the Transform.
     */
    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt( position, position + rotation * glm::vec3(0,0,1), rotation * glm::vec3(0,1,0) );
    }

    /**
     * @brief reverse
     * @return
     *
     * Returns the reverse quaternion of the Transform's rotation
     */
    glm::quat reverse() const
    {
        return glm::quat(rotation.w, -rotation.x,  -rotation.y, -rotation.z);
    }

    /**
     * @brief lookat
     * @param at
     * @param up
     * @return
     *
     * Rotate the transform so it looks at a particular point.
     */
    Transform& lookat( glm::vec3 const & pointToLookAt, glm::vec3 const & up)
    {
#if 1
        rotation = glm::quatLookAt( glm::normalize(position-pointToLookAt) , up);
#else
        glm::vec3 z = -glm::normalize(position-pointToLookAt);
        glm::vec3 x = glm::normalize(glm::cross(up,z));
        glm::vec3 y = glm::cross(z,x);

        glm::mat3 R(x,y,z);
        rotation = glm::quat_cast(R);
#endif
        return *this;

        // ignore this: this is another method of calculating the rotation
        // not not working 100% the way I want.
        // rotation = glm::conjugate( glm::quat_cast( glm::lookAt( position, -at, up)  ) );
    }


    // return the x/y/z axies of the Transform.
    // ie: the direction the local direction of the x-axis
    glm::vec3 xAxis() const
    {
        return rotation * glm::vec3(1,0,0);
    }
    glm::vec3 yAxis() const
    {
        return rotation * glm::vec3(0,1,0);
    }
    glm::vec3 zAxis() const
    {
        return rotation * glm::vec3(0,0,1);
    }

    // returns various directions
    glm::vec3 forward() const
    {
        return rotation * glm::vec3(0,0,1);
    }
    glm::vec3 back() const
    {
        return -forward();
    }

    glm::vec3 left() const
    {
        return rotation * glm::vec3(1,0,0);
    }
    glm::vec3 right() const
    {
        return -left();
    }

    glm::vec3 up() const
    {
        return rotation * glm::vec3(0,1,0);
    }
    glm::vec3 down() const
    {
        return -up();
    }

    // A few constant transforms which provide
    // rotations around paricular axes
    static constexpr Transform R90x()
    {
        return Transform( {0,0,0}, glm::quat( { glm::half_pi<float>() ,0,0} ));
    }
    static constexpr Transform R180x()
    {
        return Transform( {0,0,0}, glm::quat( { glm::pi<float>() ,0,0} ));
    }
    static constexpr Transform R270x()
    {
        return Transform( {0,0,0}, glm::quat( { -glm::half_pi<float>() ,0,0} ));
    }
    static constexpr Transform R90y()
    {
        return  Transform( {0,0,0}, glm::quat( { 0,glm::half_pi<float>() ,0} ));
    }
    static constexpr Transform R180y()
    {
        return Transform( {0,0,0}, glm::quat( { 0,glm::pi<float>() ,0} ));
    }
    static constexpr Transform R270y()
    {
        return Transform( {0,0,0}, glm::quat( { 0,-glm::half_pi<float>() ,0} ));
    }
    static constexpr Transform R90z()
    {
        return  Transform( {0,0,0}, glm::quat( { 0,0,glm::half_pi<float>() } ));
    }
    static constexpr Transform R180z()
    {
        return Transform( {0,0,0}, glm::quat( { 0,0,glm::pi<float>() } ));
    }
    static constexpr Transform R270z()
    {
        return Transform( {0,0,0}, glm::quat( { 0,0,-glm::half_pi<float>() } ));
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
 * the transform from L to R
 */
inline Transform mix( const Transform & L, const Transform & R, float t)
{
    return Transform{
        glm::mix(L.position, R.position, t),
        glm::slerp(L.rotation, R.rotation, t),
        glm::mix(L.scale, R.scale,t)
    };
}

inline glm::vec3 operator * (const Transform & ps, const glm::vec3 & ls)
{
    return ps.position  + ps.rotation * (ps.scale * ls);
}


inline Transform operator * (const Transform & ps, const Transform & ls)
{
    return
    Transform(
                ps.position  + ps.rotation * (ps.scale * ls.position),
                ps.rotation * ls.rotation,
                ps.scale * ls.scale
    );
}

inline Transform& operator *= ( Transform & ps,  Transform const & ls)
{
    ps = ps * ls;

    return ps;

}

inline Transform operator/( Transform const & ws,  Transform const& ps)
{
    const glm::quat psConjugate = glm::conjugate(ps.rotation);

    return Transform  (
                         (psConjugate * (ws.position - ps.position)) / ps.scale,
                         psConjugate * ws.rotation,
                         psConjugate * (ws.scale / ps.scale)
                );
}

}

#endif // Transform_H

