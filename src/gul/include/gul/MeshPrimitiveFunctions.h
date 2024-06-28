#ifndef GUL_MESH_PRIMITIVE_FUNCTIONS_H
#define GUL_MESH_PRIMITIVE_FUNCTIONS_H

#include "MeshPrimitive2.h"
#include <glm/glm.hpp>

namespace gul
{


/**
 * @brief convertAttribute_t
 * @param V
 * @param C
 *
 * Converts the data from inType to outType using the callable function C,
 *
 * This does not do any type checking, it simply byte copies the data
 * runs it through the callable, and byte-writes the new data.
 *
 * Note: inType and outType must be fundamental type or have inType::value_type as a fundamental type
 */
template<typename inType, typename outType, typename Callable_t>
inline bool convertAttribute_t(VertexAttribute & V, Callable_t && C)
{

    // if the output type is going to be smaller
    // then the input type, then we can
    // forward iterate because we wont overwrite any
    // data that is currently being read
    if constexpr ( sizeof(outType) <= sizeof(inType))
    {
        auto attrCount = V.attributeCount();

        auto byteSizeIn  =  attrCount * V.getAttributeSize();
        auto byteSizeOut =  attrCount * sizeof(outType);
        auto newSize     =  byteSizeIn / sizeof(outType);

        (void)byteSizeIn;
        (void)byteSizeOut;
        (void)newSize;

        for(uint32_t i=0;i<attrCount;i++)
        {
            auto v = C(V.get<inType>(i));
            V.set<outType>(i, v);
        }

        V.init( type_to_component<outType>(), type_to_type<outType>() );
        V.resize(attrCount);
        assert( V.attributeCount() == attrCount);
    }
    else
    {
        // if the output size is larger, we have to do the conversion starting
        // from the end and work backwards
        auto attrCount = V.attributeCount();

        auto attrSize    = V.getAttributeSize();
        if(attrSize == 0)
            return false;
        auto byteSizeIn  =  attrCount * attrSize;
        auto byteSizeOut =  attrCount * sizeof(outType);
        auto newSize     =  (byteSizeOut) / (attrSize);
        (void)byteSizeIn;
        (void)byteSizeOut;
        (void)newSize;

        if constexpr ( sizeof(outType)  % sizeof(inType) != 0)
        {
            V.resize(newSize+1);
        }
        else
        {
            V.resize(newSize);
        }

        for(uint32_t i=0;i<attrCount;i++)
        {
            auto j = attrCount-i-1;

            auto v = C(V.get<inType>(j));
            V.set<outType>(j, v);
        }

        V.init( type_to_component<outType>(), type_to_type<outType>());
        V.resize(attrCount);
    }
    return true;

}

inline glm::uvec2 packHalf4x32(glm::vec4 const &v)
{
    return { glm::packHalf2x16(glm::vec2(v[0],v[1]) ), glm::packHalf2x16(glm::vec2(v[2],v[3]) ) };
}

inline uint32_t packSnorm3x10(glm::vec3 const & n)
{
    auto ot = glm::uvec3( glm::mix( glm::vec3(0.0f), glm::vec3(1023), (n+1.0f)*0.5f) ) << glm::uvec3(0,10,20);
    return ot.x | ot.y | ot.z;
}

inline glm::vec3 unpackSnorm3x10(uint32_t n)
{
    return ( glm::vec3( (glm::uvec3(n) >> glm::uvec3(0,10,20) ) & glm::uvec3(1023) ) - 511.0f ) / 511.0f;
}

/**
 * @brief packMesh
 * @param M
 * @return
 *
 * Packs the mesh using some known schemes:
 *
 * Attribute     Input Type      Output Type                     GLSL Function to unpack
 * ---------------------------------------------------------------------------------------
 * POSITION       vec3           vec3
 * NORMAL         vec3           uint                            see function above: unpackSnorm3x10
 * TEXCOORD_N     vec2           uint                            unpackHalf2x16(uint)
 * COLOR_N        vec4           uint                            unpackUnorm4x8(uint)
 * JOINTS_0       uvec4
 * WEIGHTS_N      vec4           2 x uint                        vec4( unpackHalf2x16(u[0]), unpackHalf2x16(u[1]) )
 *
 */
inline uint64_t packMesh(MeshPrimitive & M)
{
    if(M.NORMAL.getComponentType() == eComponentType::FLOAT && M.NORMAL.getType() == eType::VEC3)
    {
        convertAttribute_t<glm::vec3, uint32_t>(M.NORMAL, packSnorm3x10);
        M.NORMAL.setType(eType::SCALAR);
        M.NORMAL.setComponent(eComponentType::UNSIGNED_INT);
    }
    if(M.TEXCOORD_0.getComponentType() == eComponentType::FLOAT && M.TEXCOORD_0.getType() == eType::VEC2)
    {
        convertAttribute_t<glm::vec2, uint32_t>(M.TEXCOORD_0, glm::packHalf2x16);
        M.NORMAL.setType(eType::SCALAR);
        M.NORMAL.setComponent(eComponentType::UNSIGNED_INT);
    }
    if(M.TEXCOORD_1.getComponentType() == eComponentType::FLOAT && M.TEXCOORD_1.getType() == eType::VEC2)
    {
        convertAttribute_t<glm::vec2, uint32_t>(M.TEXCOORD_1, glm::packHalf2x16);
        M.NORMAL.setType(eType::SCALAR);
        M.NORMAL.setComponent(eComponentType::UNSIGNED_INT);
    }
    if(M.TANGENT.getComponentType() == eComponentType::FLOAT && M.TANGENT.getType() == eType::VEC4)
    {
        convertAttribute_t<glm::vec4, uint32_t>(M.TANGENT, glm::packSnorm4x8);
        M.TANGENT.setType(eType::SCALAR);
        M.TANGENT.setComponent(eComponentType::UNSIGNED_INT);
    }
    if(M.COLOR_0.getComponentType() == eComponentType::FLOAT && M.COLOR_0.getType() == eType::VEC4)
    {
        convertAttribute_t<glm::vec4, uint32_t>(M.COLOR_0, glm::packUnorm4x8);
        M.NORMAL.setType(eType::SCALAR);
        M.NORMAL.setComponent(eComponentType::UNSIGNED_INT);
    }
    if(M.WEIGHTS_0.getComponentType() == eComponentType::FLOAT && M.WEIGHTS_0.getType() == eType::VEC4)
    {
        convertAttribute_t<glm::vec4, glm::uvec2>(M.WEIGHTS_0, packHalf4x32);
        M.WEIGHTS_0.setType(eType::VEC2);
        M.NORMAL.setComponent(eComponentType::UNSIGNED_INT);
    }
    return M.calculateDeviceSize();
}

}

#endif

