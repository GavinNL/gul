#ifndef GUL_MESH_PRIMITIVE_2_H
#define GUL_MESH_PRIMITIVE_2_H

#include <cmath>
#include <vector>
#include <array>
#include <cstring>
#include <tuple>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>
#include <map>

namespace gul
{

enum class eComponentType : uint32_t
{
    UNKNOWN        = 0,
    BYTE           = 5120,
    UNSIGNED_BYTE  = 5121,
    SHORT          = 5122,
    UNSIGNED_SHORT = 5123,
    INT            = 5124,
    UNSIGNED_INT   = 5125,
    FLOAT          = 5126,
    DOUBLE         = 5130
};

enum class eType : uint32_t
{
    //           rows         columns
    UNKNOWN = 0x00000000 | 0x00000000,
    SCALAR  = 0x00000100 | 0x00000001,
    VEC2    = 0x00000100 | 0x00000002,
    VEC3    = 0x00000100 | 0x00000003,
    VEC4    = 0x00000100 | 0x00000004,
    MAT2    = 0x00000200 | 0x00000002,
    MAT3    = 0x00000300 | 0x00000003,
    MAT4    = 0x00000400 | 0x00000004
};

constexpr const char* to_string(eComponentType t)
{
    switch(t)
    {
        default:
        case eComponentType::UNKNOWN       : return "UNKNOWN";
        case eComponentType::BYTE          : return "BYTE";
        case eComponentType::UNSIGNED_BYTE : return "UNSIGNED_BYTE";
        case eComponentType::SHORT         : return "SHORT";
        case eComponentType::UNSIGNED_SHORT: return "UNSIGNED_SHORT";
        case eComponentType::INT           : return "INT";
        case eComponentType::UNSIGNED_INT  : return "UNSIGNED_INT";
        case eComponentType::FLOAT         : return "FLOAT";
        case eComponentType::DOUBLE        : return "DOUBLE";
    }
}

constexpr const char* to_string(eType t)
{
    switch(t)
    {
        default:
        case eType::UNKNOWN: return "UNKNOWN";
        case eType::SCALAR : return "SCALAR";
        case eType::VEC2   : return "VEC2";
        case eType::VEC3   : return "VEC3";
        case eType::VEC4   : return "VEC4";
        case eType::MAT2   : return "MAT2";
        case eType::MAT3   : return "MAT3";
        case eType::MAT4   : return "MAT4";
    }
}

/**
 * @brief row_type
 * @param c
 * @return
 *
 * Returns the row type
 */
constexpr eType row_type(eType c)
{
    switch ( c )
    {
        case eType::UNKNOWN: return eType::UNKNOWN;
        case eType::SCALAR : return eType::UNKNOWN;
        case eType::VEC2   : return eType::SCALAR;
        case eType::VEC3   : return eType::SCALAR;
        case eType::VEC4   : return eType::SCALAR;
        case eType::MAT2   : return eType::VEC2;
        case eType::MAT3   : return eType::VEC3;
        case eType::MAT4   : return eType::VEC4;
    }
    return eType::UNKNOWN;
}

constexpr uint32_t component_row_count(eType c)
{
    return (static_cast<uint32_t>(c) & 0x000000FF);
}

constexpr uint32_t component_column_count(eType c)
{
    return ( (static_cast<uint32_t>(c) >> 8) & 0x000000FF);
}

constexpr uint32_t component_count(eType c)
{
    return component_row_count(c) * component_column_count(c);
}

constexpr uint32_t component_size(eComponentType c)
{
    switch(c)
    {
        case gul::eComponentType::BYTE:
        case gul::eComponentType::UNSIGNED_BYTE: return 1;
        case gul::eComponentType::SHORT:
        case gul::eComponentType::UNSIGNED_SHORT: return 2;
        case gul::eComponentType::INT:
        case gul::eComponentType::UNSIGNED_INT:
        case gul::eComponentType::FLOAT: return 4;
        case gul::eComponentType::DOUBLE: return 8;
        default:
            return 0;
    }
}

/**
 * @brief type_to_component
 * @return
 *
 * Given a datatype, either fundamental, array<T>, or glm type
 * return the base component type. If it is non-fundamental type
 * T::value_type must exist
 */
template<typename T>
constexpr eComponentType type_to_component()
{
    if constexpr ( std::is_arithmetic_v<T> )
    {
        if constexpr (std::is_same_v<T, int8_t>) return eComponentType::BYTE;
        else if constexpr (std::is_same_v<T, uint8_t>) return eComponentType::UNSIGNED_BYTE;
        else if constexpr (std::is_same_v<T, int16_t>) return eComponentType::SHORT;
        else if constexpr (std::is_same_v<T, uint16_t>) return eComponentType::UNSIGNED_SHORT;
        else if constexpr (std::is_same_v<T, int32_t>) return eComponentType::INT;
        else if constexpr (std::is_same_v<T, uint32_t>) return eComponentType::UNSIGNED_INT;
        else if constexpr (std::is_same_v<T, float>) return eComponentType::FLOAT;
        else if constexpr (std::is_same_v<T, double>) return eComponentType::DOUBLE;
        else
        {
            return eComponentType::UNKNOWN;
        }
    }
    else
    {
        return type_to_component<typename T::value_type>();
    }
}

/**
 * @brief type_to_type
 * @return
 *
 * Converts a C++ datatype into a eType
 */
template<typename T>
constexpr eType type_to_type()
{
    if constexpr ( std::is_arithmetic_v<T>) // integer or float types
    {
        return eType::SCALAR;
    }
    else
    {
        constexpr auto component = type_to_component<T>();
        constexpr auto size      = component_size(component);
        constexpr auto componentCount = sizeof(T) / size;

        static_assert(componentCount == 1 ||
                      componentCount == 2 ||
                      componentCount == 3 ||
                      componentCount == 4 ||
                      componentCount == 9 ||
                      componentCount == 16);

        if constexpr ( componentCount == 4 )
        {
            // either a vec4 or a mat2
            // so we need to determine whether V[0] is an arithmetic type
            // or a

            struct S
            {
                auto operator()()
                {
                    return T()[0];
                }
            };

            //std::invoke_result
            if constexpr( std::is_arithmetic_v< typename std::invoke_result<S>::type> )
            {
                // T[0]  is  arithmetic so its vec4
                return eType::VEC4;
            }
            else
            {
                // T[0] is likely a vec[2] so its a matrix
                return eType::MAT2;
            }
        }
        else
        {

            // in all other cases we can determine the type by the total number
            // of components
            switch ( sizeof(T) / size)
            {
                case 1:
                    return eType::SCALAR;
                case 2:
                    return eType::VEC2;
                case 3:
                    return eType::VEC3;
                case 4:
                    return eType::VEC4;
                case 9:
                    return eType::MAT3;
                case 16:
                    return eType::MAT4;
                default:
                    return eType::UNKNOWN;
            }
        }

    }
}

/**
 * @brief The VertexAttribute struct
 *
 * The vertex attribute class is essentially a vector of data
 * for a single attribute. This is NOT meant for compound vertices: eg
 * struct Vertex
 * {
 *    vec3 position;
 *    vec2 uv;
 * }
 *
 *
 */
struct VertexAttribute
{
    VertexAttribute()
    {

    }
    VertexAttribute(eComponentType c, eType t)
    {
        m_componentType = c;
        m_type = t;
    }

    template<typename T>
    VertexAttribute( std::vector<T> const & V)
    {
        using container_type = std::decay_t<decltype(V) >;
        using attribute_type = typename container_type::value_type;

        m_componentType = type_to_component<attribute_type>();
        m_type = type_to_type<attribute_type>();
        m_data.resize( getAttributeSize() * V.size() );
        std::memcpy(m_data.data(), V.data(), m_data.size());
    }


    void dump(std::ostream & out, std::string name)
    {
        out.write(name.data(), static_cast<std::streamsize>(name.size()));
        out.write(reinterpret_cast<const char*>(&m_componentType), sizeof(m_componentType));
        out.write(reinterpret_cast<const char*>(&m_type), sizeof(m_type));
        out.write(reinterpret_cast<const char*>(m_data.data()), static_cast<std::streamsize>(m_data.size()));
    }

    /**
     * @brief init
     * @param c
     * @param t
     *
     * Initialize the vertex attribute based on its base component type and
     * its attribute type
     */
    void init(eComponentType c, eType t)
    {
        m_componentType = c;
        m_type = t;
    }

    template<typename T>
    VertexAttribute& operator=(std::vector<T> const & V)
    {
        using container_type = std::decay_t<decltype(V) >;
        using attribute_type = typename container_type::value_type;

        m_componentType = type_to_component<attribute_type>();
        m_type = type_to_type<attribute_type>();
        m_data.resize( getAttributeSize() * V.size() );
        std::memcpy(m_data.data(), V.data(), m_data.size());
        return *this;
    }

    template<typename T>
    std::vector<T> toVector() const
    {
        std::vector<T> data( m_data.size() / sizeof(T));
        std::memcpy(data.data(), m_data.data(), sizeof(T)*data.size());
        return data;
    }
    /**
     * @brief at
     * @param i
     * @return
     *
     * Returns the value of a component
     */
    template<typename T>
    T at(size_t index, size_t componentIndex=0) const
    {
        T v;
        std::memcpy(&v, m_data.data() + index * getAttributeSize() + componentIndex * component_size(m_componentType), sizeof(T));
        return v;
    }

    /**
     * @brief get
     * @param index
     * @return
     *
     * Treats the VertexAttribute as a vector<T> and returns
     * the index into that vector
     */
    template<typename T>
    T get(size_t index) const
    {
        T v;
        std::memcpy(&v, m_data.data() + index * sizeof(T), sizeof(T));
        return v;
    }


    /**
     * @brief getAttributeAs
     * @param index
     * @return
     *
     * different from get(), return's the attribute specified by index,
     * the a
     */
    template<typename T>
    T getAttributeAs(size_t index) const
    {
        T v;
        std::memcpy(&v, m_data.data() + index * getAttributeSize(), sizeof(T));
        return v;
    }

    /**
     * @brief set
     * @param index
     * @param v
     *
     * Treats the VertexAttribute as a vector<T> and sets
     * vertexAttribute[index] = v
     */
    template<typename T>
    void set(size_t index, T const &v)
    {
        std::memcpy(m_data.data() + index*sizeof(T), &v, sizeof(T));
    }

    /**
     * @brief size
     * @return
     *
     * Returns the total number of components in the attribute array.
     *
     * If ther are 4 vertices and each vertex has xyz components, the
     * return value will be 12
     */
    size_t size() const
    {
        return attributeCount() * getNumComponents();
    }

    /**
     * @brief push_back
     * @param v
     *
     * Pushes data to the end of the vector
     */
    template<typename T>
    void push_back(T const & v)
    {
        auto m = m_data.size();
        m_data.resize( m + sizeof(v));
        std::memcpy( &m_data[m], &v, sizeof(v));
    }

    bool empty() const
    {
        return m_data.empty();
    }

    /**
     * @brief getType
     * @return
     *
     */
    eType getType() const
    {
        return m_type;
    }
    eComponentType getComponentType() const
    {
        return m_componentType;
    }
    uint32_t getNumComponents() const
    {
        return component_count(m_type);
    }
    std::array<uint32_t,2> getShape() const
    {
        return { static_cast<uint32_t>(attributeCount()), getNumComponents()};
    }

    /**
     * @brief convertTo32BitInteger
     *
     * Used only for uint and ints. Converts a lower
     * bit value into the 32 bit equivelant
     */
    bool convertTo32BitInteger()
    {
        auto totalComponents = attributeCount() * getNumComponents();

        if(getComponentType() == eComponentType::UNSIGNED_BYTE)
        {
            VertexAttribute newData(eComponentType::UNSIGNED_INT, getType());
            for(uint32_t i=0;i<totalComponents;i++)
            {
                newData.push_back<uint32_t>(get<uint8_t>(i));
            }
            *this = std::move(newData);
            return true;
        }
        if(getComponentType() == eComponentType::UNSIGNED_SHORT)
        {
            VertexAttribute newData(eComponentType::UNSIGNED_INT, getType());
            for(uint32_t i=0;i<totalComponents;i++)
            {
                newData.push_back<uint32_t>(get<uint16_t>(i));
            }
            *this = std::move(newData);
            return true;
        }
        if(getComponentType() == eComponentType::BYTE)
        {
            VertexAttribute newData(eComponentType::INT, getType());
            for(uint32_t i=0;i<totalComponents;i++)
            {
                newData.push_back<int32_t>(get<int8_t>(i));
            }
            *this = std::move(newData);
            return true;
        }
        if(getComponentType() == eComponentType::SHORT)
        {
            VertexAttribute newData(eComponentType::INT, getType());
            for(uint32_t i=0;i<totalComponents;i++)
            {
                newData.push_back<int32_t>(get<int16_t>(i));
            }
            *this = std::move(newData);
            return true;
        }
        return false;
    }

    void* data()
    {
        return m_data.data();
    }

    void const* data() const
    {
        return m_data.data();
    }

    /**
     * @brief getAttributeSize
     * @return
     *
     * Returns the size of the attribute. If it returns 0 it means that
     * the attribute type has not been set
     */
    uint32_t getAttributeSize() const
    {
        return component_size(m_componentType) * component_count(m_type);
    }

    uint64_t getByteSize() const
    {
        return m_data.size();
    }

    /**
     * @brief attributeCount
     * @return
     *
     * Returns the total number of attributes in the buffer.
     */
    uint64_t attributeCount() const
    {
        auto s = getAttributeSize();
        return s == 0 ? 0 : m_data.size() / s;
    }

    /**
     * @brief resize
     * @param attrCount
     *
     * Resize the attribute vector to be able to hold attrCount attributes
     */
    void resize(size_t attrCount)
    {
        m_data.resize( attrCount * getAttributeSize() );
    }
    /**
     * @brief canMerge
     * @param B
     * @return
     *
     * Returns wither you can merge this vertex attribute with another.
     * You can only merge the two if the componentType and the Type are the same
     */
    bool canMerge(VertexAttribute const & B) const
    {
        return m_componentType == B.m_componentType && m_type == B.m_type;
    }

    /**
     * @brief merge
     * @param B
     * @return
     *
     * Merge B to the end of the attribute vector and return the byte offset
     * at which the data was merged.
     */
    uint64_t merge(VertexAttribute const& B)
    {
        auto s = m_data.size();
        m_data.insert(m_data.end(), B.m_data.begin(), B.m_data.end());
        return s;
    }

    /**
     * @brief strideCopy
     * @param data
     * @param stride
     *
     * Copy the vertex attribute data into the memory location.
     *
     * if V = [p0,p1,p2,p3]
     *
     * Then a stride copy of strideCopy(data, 2*sizeof(p0)) will copy data as follows
     *
     * data = [p0|  |p1|  |p2| |p3]
     *
     * This is used to interleave multiple attribute. eg:
     *
     * positionAttribute.strideCopy(data, sizeof(vec3) )
     * uvAttribute.strideCopy( data+sizeof(vec3), sizeof(vec2) )
     */
    void strideCopy(void * data, uint64_t stride) const
    {
        auto c = attributeCount();
        auto s = getAttributeSize();

        auto d_in  = m_data.data();
        auto d_out = static_cast<uint8_t*>(data);
        for(uint64_t i=0;i<c;i++)
        {
            std::memcpy(d_out, d_in, s);
            d_out += stride;
            d_in  += s;
        }
    }


    /**
     * @brief strideCopyOffset
     * @param dstData - the start of the destination to copy to
     * @param dstByteStride - how many bytes to skip after copying each attribute
     * @param dstByteOffset - the offset from the start of dstData to start copying to
     *
     * @param srcStartAttributeIndex - which index in the source attribute to start copying from
     * @param attributeCountToCopy - number of attributes to copy
     * @return
     *
     * Copies the attribute data to dstData+dstByteOffset
     */
    uint64_t strideCopyOffset(void * dstData,
                              uint64_t dstByteStride,
                              uint64_t dstByteOffset,

                              uint64_t srcStartAttributeIndex,
                              uint64_t attributeCountToCopy = std::numeric_limits<uint64_t>::max()) const
    {
        auto c = std::min(attributeCount(), attributeCountToCopy);
        auto srcAttrSize = getAttributeSize();

        auto d_in  = m_data.data() + srcStartAttributeIndex * srcAttrSize;
        auto d_in_end = std::min(d_in + srcAttrSize * attributeCountToCopy, &m_data.back()+1);

        auto d_out = static_cast<uint8_t*>(dstData) + dstByteOffset;

        while(d_in < d_in_end)
        {
            std::memcpy(d_out, d_in, srcAttrSize);

            d_out += dstByteStride;
            d_in  += srcAttrSize;
        }
        return c;
    }

    /**
     * @brief strideCopy
     * @param data
     * @param n
     * @param offset
     * @param stride
     * @param num
     *
     * Copies num attributes into memory starting at data+offset with a specific stride.
     *
     * For example
     *
     *  <-offset-> <--stride-->
     * [          | A1 |       | A2 |        | A3 |       ]
     *  ^--data
     */
    [[deprecated]] uint64_t strideCopy(void * data, uint64_t stride, uint64_t offset, uint64_t num = std::numeric_limits<uint64_t>::max()) const
    {
        auto c = std::min(attributeCount(), num);
        auto s = getAttributeSize();

        auto d_in  = m_data.data();
        auto d_out = static_cast<uint8_t*>(data)+offset;

        for(uint64_t i=0;i<c;i++)
        {
            std::memcpy(d_out, d_in, s);
            d_out += stride;
            d_in  += s;
        }
        return c;
    }

    void clear()
    {
        m_data.clear();
    }

    void setType(eType t)
    {
        m_type = t;
    }
    void setComponent(eComponentType c)
    {
        m_componentType = c;
    }

    /**
     * @brief getMinMax
     * @return
     *
     * Returns the min and max values for each component.
     */
    template<typename T>
    std::pair< std::vector<T>, std::vector<T>> getMinMax() const
    {
        auto & V = *this;

        static_assert( std::is_arithmetic_v<T>, "T must be an arithmetic type");
        using value_type = T;

        std::vector<value_type> _min(component_count(V.getType()), std::numeric_limits<value_type>::max() );
        std::vector<value_type> _max(component_count(V.getType()), std::numeric_limits<value_type>::lowest() );

        auto attrCount = attributeCount();
        for(uint32_t j=0;j<attrCount;j++)
        {
            for(size_t i=0;i<_min.size();i++)
            {
                _min[i] = std::min( V.at<value_type>(j, i), _min[i] );
                _max[i] = std::max( V.at<value_type>(j, i), _max[i] );
            }
        }
        return {_min, _max};
    }

protected:
    friend struct MeshPrimitive;
    std::vector<uint8_t> m_data;
    eComponentType       m_componentType = eComponentType::UNKNOWN;
    eType                m_type = eType::UNKNOWN;
};

//===========================================================================================================
/**
 * @brief calculateInterleavedStride
 * @param attrs
 * @return
 *
 * Calculates the sum of each attr[i].attributeSize() skipping any attributes that dont have
 * items
 */
inline uint64_t calculateInterleavedStride(std::vector<VertexAttribute const*> const &attrs)
{
    uint64_t stride=0;
    for(auto * v :  attrs)
    {
        if(v->size() > 0)
            stride += v->getAttributeSize();
    }
    return stride;
}

/**
 * @brief calculateInterleavedBytes
 * @param attrs
 * @return
 *
 * Returns the total number of bytes required to store all attributes in
 * interleaved format, attributes with zero attributeCount() are not included
 */
inline uint64_t calculateInterleavedBytes(std::vector<VertexAttribute const*> const &attrs)
{
    auto stride = calculateInterleavedStride(attrs);

    uint64_t vCount = 9999999999999;
    for(auto * v :  attrs)
    {
        if(v->size() > 0)
            vCount = std::min(vCount,v->attributeCount());
    }
    return stride*vCount;
}


//===========================================================================================================

enum class Topology
{
    POINT_LIST                    = 0,
    LINE_LIST                     = 1,
    LINE_STRIP                    = 2,
    TRIANGLE_LIST                 = 3,
    TRIANGLE_STRIP                = 4,
    TRIANGLE_FAN                  = 5,
    LINE_LIST_WITH_ADJACENCY      = 6,
    LINE_STRIP_WITH_ADJACENCY     = 7,
    TRIANGLE_LIST_WITH_ADJACENCY  = 8,
    TRIANGLE_STRIP_WITH_ADJACENCY = 9,
    PATCH_LIST                    = 10,
};

struct DrawCall
{
    uint32_t indexCount   = 0;
    uint32_t vertexCount  = 0;
    int32_t  vertexOffset = 0;
    int32_t  indexOffset  = 0;
    Topology topology     = Topology::TRIANGLE_LIST;
};

using Primitive = DrawCall;

/**
 * @brief forEachVertexIndex
 * @param _INDEX
 * @param p
 * @param C
 *
 * Given an index buffer and a primitive, call the callable, C for each VertexIndex in the primitive.
 *
 */
template<typename Callable_t>
inline void forEachVertexIndex(VertexAttribute const & _INDEX, Primitive const & p, Callable_t && C)
{
    if( _INDEX.getComponentType() == eComponentType::UNSIGNED_INT)
    {
        uint32_t vertexOffset = static_cast<uint32_t>(p.vertexOffset);
        using IndexComponentType = uint32_t;

        for(uint32_t i=0;i < p.indexCount ; i++)
        {
            uint32_t vertexIndex =  _INDEX.at<IndexComponentType>(i + static_cast<uint32_t>(p.indexOffset))
                                   + vertexOffset;
            C(vertexIndex);
        }
    }
}

/**
 * @brief The MeshPrimitive struct
 *
 * A Mesh Primitive is a class which allows
 * you to represent a triangular mesh
 *
 */
struct MeshPrimitive
{
    using attribute_type = VertexAttribute;

    // list of common attributes in the order specified by the GLTF specification
    // initialized using the most common types
    attribute_type POSITION   = attribute_type(eComponentType::FLOAT, eType::VEC3);
    attribute_type NORMAL     = attribute_type(eComponentType::FLOAT, eType::VEC3);
    attribute_type TANGENT    = attribute_type(eComponentType::FLOAT, eType::VEC4);
    attribute_type TEXCOORD_0 = attribute_type(eComponentType::FLOAT, eType::VEC2);
    attribute_type TEXCOORD_1 = attribute_type(eComponentType::FLOAT, eType::VEC2);
    attribute_type COLOR_0    = attribute_type(eComponentType::UNSIGNED_BYTE, eType::VEC4);
    attribute_type JOINTS_0   = attribute_type(eComponentType::UNSIGNED_SHORT, eType::VEC4);
    attribute_type WEIGHTS_0  = attribute_type(eComponentType::FLOAT, eType::VEC4);

    // The index buffer
    attribute_type INDEX      = attribute_type(eComponentType::UNSIGNED_INT, eType::SCALAR);

    Topology       topology   = Topology::TRIANGLE_LIST;

    // a vector of primitives
    // each primitive is a sub component of the mesh and
    // contains the draw call to draw it
    std::vector<Primitive> primitives;

    void clear()
    {
        for(auto * attr : {&POSITION  ,
                           &NORMAL    ,
                           &TANGENT   ,
                           &TEXCOORD_0,
                           &TEXCOORD_1,
                           &COLOR_0   ,
                           &JOINTS_0  ,
                           &WEIGHTS_0 ,
                           &INDEX})
        {
            attr->clear();
        }
    }


    /**
     * @brief dump
     * @param out
     *
     * [experimental]
     * Dump the entire mesh to a simple binary file
     */
    void dump(std::ostream & out)
    {
        auto attrs = {&POSITION  ,
                                   &NORMAL    ,
                                   &TANGENT   ,
                                   &TEXCOORD_0,
                                   &TEXCOORD_1,
                                   &COLOR_0   ,
                                   &JOINTS_0  ,
                                   &WEIGHTS_0 ,
                                   &INDEX};
        struct header_t
        {
            uint64_t magic = 5496876546618;
            uint32_t byteSize=0;
            uint32_t numAttributes=0;
        };

        header_t h;
        h.byteSize = 0;
        for(auto * attr : attrs)
        {
            if(attr->size())
            {
                h.numAttributes++;
            }
        }

        #define DUMP_ATTR(NAME ) if(NAME.size() > 0) NAME.dump(out, #NAME)

        out.write(reinterpret_cast<char const *>(&h), sizeof(h));
        DUMP_ATTR(NORMAL    );
        DUMP_ATTR(TANGENT   );
        DUMP_ATTR(TEXCOORD_0);
        DUMP_ATTR(TEXCOORD_1);
        DUMP_ATTR(COLOR_0   );
        DUMP_ATTR(JOINTS_0  );
        DUMP_ATTR(WEIGHTS_0 );
        DUMP_ATTR(INDEX     );

    }

    /**
     * @brief calculateDeviceSize
     * @return
     *
     * Calculate the amount of bytes this mesh takes on the
     * the GPU if all vertices were placed one after the
     * other.
     *
     * This also includes the index size!
     */
    uint64_t calculateDeviceSize() const
    {
        uint64_t size = 0;

        size += POSITION  .getByteSize();
        size += NORMAL    .getByteSize();
        size += TANGENT   .getByteSize();
        size += TEXCOORD_0.getByteSize();
        size += TEXCOORD_1.getByteSize();
        size += COLOR_0   .getByteSize();
        size += JOINTS_0  .getByteSize();
        size += WEIGHTS_0 .getByteSize();
        size += INDEX.getByteSize();

        return size;
    }

    /**
     * @brief isSimilar
     * @param P
     * @return
     *
     * Returns true if two mesh primitives are similar.
     * Two mesh primitives are similar if they have the same attributes
     * and their attribute have the same type
     */
    bool isSimilar( MeshPrimitive const & P) const
    {
        return
            POSITION  .canMerge(P.POSITION   ) &&
            NORMAL    .canMerge(P.NORMAL     ) &&
            TANGENT   .canMerge(P.TANGENT    ) &&
            TEXCOORD_0.canMerge(P.TEXCOORD_0 ) &&
            TEXCOORD_1.canMerge(P.TEXCOORD_1 ) &&
            COLOR_0   .canMerge(P.COLOR_0    ) &&
            JOINTS_0  .canMerge(P.JOINTS_0   ) &&
            WEIGHTS_0 .canMerge(P.WEIGHTS_0  ) &&
            INDEX     .canMerge(P.INDEX      );
    }

    /**
     * @brief indexCount
     * @return
     *
     * Returns the total number of indices in the mesh
     */
    size_t indexCount() const
    {
        return INDEX.attributeCount();
    }

    /**
     * @brief vertexCount
     * @return
     *
     * Returns number of vertices in the mesh. The number of vertices
     * is the minimum (non-zero) attribute count of
     */
    size_t vertexCount() const
    {
        size_t count=std::numeric_limits<size_t>::max();
        for(auto * v :  { &POSITION,
                           &NORMAL,
                           &TANGENT,
                           &TEXCOORD_0,
                           &TEXCOORD_1,
                           &COLOR_0,
                           &JOINTS_0,
                           &WEIGHTS_0})
        {
            auto sh = v->attributeCount();
            if( sh != 0)
                count = std::min<size_t>(count, sh);
        }
        return count;
    }

    /**
     * @brief getVertexFlags
     * @return
     *
     * Return a the vertex flag mask where each bit
     * represents whether the given attribute is available.
     */
    uint32_t getVertexFlags() const
    {
        uint32_t f = 0;
        f |= POSITION   .size() == 0 ? 0 : (1u << 0);
        f |= NORMAL     .size() == 0 ? 0 : (1u << 1);
        f |= TANGENT    .size() == 0 ? 0 : (1u << 2);
        f |= TEXCOORD_0 .size() == 0 ? 0 : (1u << 3);
        f |= TEXCOORD_1 .size() == 0 ? 0 : (1u << 4);
        f |= COLOR_0    .size() == 0 ? 0 : (1u << 5);
        f |= JOINTS_0   .size() == 0 ? 0 : (1u << 6);
        f |= WEIGHTS_0  .size() == 0 ? 0 : (1u << 7);

        return f;
    }

    /**
     * @brief getDrawCall
     * @return
     *
     * Returns the drawcall for the entire mesh. This can be used
     * if there are no primitives listed
     */
    Primitive getDrawCall() const
    {
        DrawCall dc;
        dc.indexOffset  = 0;
        dc.vertexOffset = 0;
        dc.vertexCount  = static_cast<uint32_t>(vertexCount());
        dc.indexCount   = static_cast<uint32_t>(indexCount());
        dc.topology     = topology;
        return dc;
    }

    /**
     * @brief merge
     * @param P
     * @param renumberIndices
     * @return
     *
     * Merges mesh P into the current mesh and returns the full primitive drawcall.
     *
     * The meshes can be merged only if they are similar (ie: they have the same attributes)
     */
    Primitive merge(MeshPrimitive const & P, bool renumberIndices = false)
    {
        DrawCall dc;

        uint32_t currentVertexCount = static_cast<uint32_t>(this->vertexCount());
        uint32_t currentIndexCount  = static_cast<uint32_t>(this->INDEX.size());

        auto origIndexCount  = indexCount();
        auto origVertexCount = vertexCount();

        dc.indexOffset  = static_cast<int32_t>(indexCount() );
        dc.vertexOffset = static_cast<int32_t>(vertexCount());
        dc.vertexCount  = static_cast<uint32_t>(P.vertexCount());
        dc.indexCount   = static_cast<uint32_t>(P.indexCount() );

        if(!POSITION  .canMerge(P.POSITION   )) throw std::runtime_error("Cannot merge. POSITION attribute of meshes are not the same.");
        if(!NORMAL    .canMerge(P.NORMAL     )) throw std::runtime_error("Cannot merge. NORMAL attribute of meshes are not the same.");
        if(!TANGENT   .canMerge(P.TANGENT    )) throw std::runtime_error("Cannot merge. TANGENT attribute of meshes are not the same.");
        if(!TEXCOORD_0.canMerge(P.TEXCOORD_0 )) throw std::runtime_error("Cannot merge. TEXCOORD_0 attribute of meshes are not the same.");
        if(!TEXCOORD_1.canMerge(P.TEXCOORD_1 )) throw std::runtime_error("Cannot merge. TEXCOORD_1 attribute of meshes are not the same.");
        if(!COLOR_0   .canMerge(P.COLOR_0    )) throw std::runtime_error("Cannot merge. COLOR_0 attribute of meshes are not the same.");
        if(!JOINTS_0  .canMerge(P.JOINTS_0   )) throw std::runtime_error("Cannot merge. JOINTS_0 attribute of meshes are not the same.");
        if(!WEIGHTS_0 .canMerge(P.WEIGHTS_0  )) throw std::runtime_error("Cannot merge. WEIGHTS_0 attribute of meshes are not the same.");
        if(!INDEX     .canMerge(P.INDEX      )) throw std::runtime_error("Cannot merge. INDEX attribute of meshes are not the same.");

        {
            POSITION  .merge(P.POSITION  );
            NORMAL    .merge(P.NORMAL    );
            TANGENT   .merge(P.TANGENT   );
            TEXCOORD_0.merge(P.TEXCOORD_0);
            TEXCOORD_1.merge(P.TEXCOORD_1);
            COLOR_0   .merge(P.COLOR_0   );
            JOINTS_0  .merge(P.JOINTS_0  );
            WEIGHTS_0 .merge(P.WEIGHTS_0 );

            INDEX     .merge(P.INDEX     );

            if(renumberIndices)
            {
                auto C = INDEX.size();
                for(uint32_t i=currentIndexCount; i<C; i++)
                {
                    uint32_t v = INDEX.get<uint32_t>(i) + currentVertexCount;
                    INDEX.set(i, v);
                    assert( v == INDEX.get<uint32_t>(i) );
                }

                dc.vertexOffset = 0;    
            }

            for(auto &  c : P.primitives)
            {
                auto & b = primitives.emplace_back(c);
                b.indexOffset += static_cast<int32_t>(origIndexCount);
                b.vertexOffset = renumberIndices ? 0 : static_cast<int>(origVertexCount);
            }

            return dc;
        }
    }

    /**
     * @brief calculateInterleavedStride
     * @return
     *
     * Returns the number of bytes required to copy all the attributes
     * in an interleaved layout: eg:
     *
     * [p0,n0,t0,p1,n1,t1...]
     *
     * The index buffer is not taken into account in the calculation
     */
    uint64_t calculateInterleavedStride() const
    {
        uint64_t stride=0;
        for(auto * v :  { &POSITION,
                           &NORMAL,
                           &TANGENT,
                           &TEXCOORD_0,
                           &TEXCOORD_1,
                           &COLOR_0,
                           &JOINTS_0,
                           &WEIGHTS_0})
        {
            if(v->attributeCount() > 0)
                stride += v->getAttributeSize();
        }
        return stride;
    }


    /**
     * @brief calculateBoundingSphereRadius
     * @param p
     * @return
     *
     * Calculate the bounding sphere of a specific primitive.
     * The center of the sphere is positioned at the origin. If the primitive
     * is fully in some quadrant, then the center of the sphere is still at the origin
     */
    template<typename PositionType=std::array<float,3>, typename IndexComponentType=uint32_t>
    float calculateBoundingSphereRadius(Primitive const & p) const
    {
        float _Max=0.0f;
        forEachVertexIndex(INDEX, p, [&_Max, this](IndexComponentType i)
        {
            auto r = POSITION.at< PositionType >(i);
            auto R2 = r[0]*r[0] + r[1]*r[1] + r[2]*r[2];
            _Max = std::max( _Max,  R2 );
        });
        return std::sqrt(_Max);
    }

    template<typename PositionType=std::array<float,3>, typename IndexComponentType=uint32_t>
    float calculateBoundingSphereRadius() const
    {
        auto P = getDrawCall();
        return calculateBoundingSphereRadius(P);
    }

    /**
     * @brief copySequential
     * @param data
     * @return
     *
     * Copies all the vertex attributes sequentually into the provided buffer
     * and returns the total number of vertices copied.
     *
     *
     * [p0,n0,t0,p1,n1,t1...]
     *
     *
     */
    inline uint64_t copyVertexAttributesInterleaved(void * data, uint64_t offset=0) const
    {
        return copyVertexAttributesInterleaved(static_cast<uint8_t*>(data)+offset,
                         { &POSITION,
                           &NORMAL,
                           &TANGENT,
                           &TEXCOORD_0,
                           &TEXCOORD_1,
                           &COLOR_0,
                           &JOINTS_0,
                           &WEIGHTS_0});
    }

    /**
     * @brief copyVertexAttributesInterleaved
     * @param data
     * @param attrs
     * @return
     *
     * Given a list of VertexAttribute pointers, copy them interleaved into data_write_ptr
     * Eg:
     *   copyVertexAttributeInterleaved(buffer, (&M.POSITION, &M.NORMAL, &M.TEXCOORD_0});
     *
     * will write the following information to buffer
     *
     * buffer  [p0,n0,t0,p1,n1,t1,p2,n2,t2...]
     *
     * Returns the total number of vertices written.
     *
     * Requires: * All attributes must have the same number of vertices
     *           * data_write_ptr must have enough sequental data to write all attribute data
     *
     */
    template<typename T>
    static uint64_t copyVertexAttributesInterleaved(T * data_write_ptr, std::vector<VertexAttribute const*> const &attrs)
    {
        auto stride = gul::calculateInterleavedStride(attrs);
        uint64_t vCount = attrs.front()->attributeCount();

        uint64_t offset = 0;
        for(auto * v :  attrs)
        {
            if(v->size() == 0)
                continue;

            v->strideCopyOffset(
                            data_write_ptr,
                            stride,
                            offset,
                            0,
                            vCount
                        );
            offset += v->getAttributeSize();
        }
        return vCount;
    }

    template<typename T>
    static uint64_t copyVertexAttributesInterleaved(std::vector<T> & dataVec, std::vector<VertexAttribute const*> const &attrs)
    {
        uint64_t vertexStride = 0;
        uint64_t vertexCount  = attrs.front()->attributeCount();

        for(auto * v :  attrs)
        {
            vertexStride += v->getAttributeSize();
        }

        auto totalBytes = vertexCount * vertexStride;

        dataVec.resize( totalBytes / sizeof(T)  );
        copyVertexAttributesInterleaved(dataVec.data(), attrs);

        return vertexCount*vertexStride;
    }


    /**
     * @brief copyVertexAttributesSquential
     * @param data
     * @return
     *
     * Copies the data in sequential layout and retuns the offsets for each
     * attribute.
     *
     * eg:
     *
     * p0,p1,p2,n0,n1,n2,t0,t1,t2...
     *
     * The index buffer is always placed at the end
     */
    std::vector<uint64_t> copyVertexAttributesSquential(void * data) const
    {
        //auto vertexCount = getVertexCount();
        std::vector<uint64_t> offsets;
        uint64_t offset=0;
        for(auto * v :  {  &POSITION,
                           &NORMAL,
                           &TANGENT,
                           &TEXCOORD_0,
                           &TEXCOORD_1,
                           &COLOR_0,
                           &JOINTS_0,
                           &WEIGHTS_0,
                           &INDEX})
        {
            if(!v->empty())
            {
                offsets.push_back(offset);
                //auto attrSize = v->getAttributeSize();

                auto count = v->attributeCount();
                assert( count *  v->getAttributeSize() <= v->m_data.size());
                std::memcpy( static_cast<uint8_t*>(data)+offset, v->m_data.data(), count * v->getAttributeSize());
                offset += count * v->getAttributeSize();
            }
            else
            {
                offsets.push_back(0);
            }
        }
        return offsets;
    }

    /**
     * @brief copyIndex
     * @param data
     * @return
     *
     * Copy the index buffer
     */
    uint64_t copyIndex(void * data) const
    {
        std::memcpy(data, INDEX.m_data.data(), INDEX.m_data.size());
        return INDEX.m_data.size();
    }

    /**
     * @brief getVertexByteSize
     * @return
     *
     * Returns the size in byte of the vertrex in bytes if all the
     * attributes were interleaved
     */
    uint64_t getVertexByteSize() const
    {
        return calculateInterleavedStride();
    }

    inline uint64_t calculateInterleavedBufferSize() const
    {
        return getVertexByteSize() * vertexCount();
    }

    /**
     * @brief fuseVertices
     *
     * Fuse near by vertices. This may not be accurate
     */
    void fuseVertices()
    {
        using _vec2 = std::array<float,2>;
        using _vec3 = std::array<float,3>;
        using _ivec3 = std::array<int32_t,3>;

        std::map< std::tuple<int32_t, int32_t, int32_t>, uint32_t> posToIndex;

        auto & _POS = POSITION;
        auto & _NOR = NORMAL;
        auto & _UV  = TEXCOORD_0;
        auto & _INDEX = INDEX;

        std::vector<_vec3> NEW_POS;
        std::vector<_vec3> NEW_NOR;
        std::vector<_vec2> NEW_UV;

        uint32_t index = 0;
        //uint32_t j     = 0;

        auto vCount = vertexCount();
        for(uint32_t j=0;j<vCount;j++)
        {
            auto p = _POS.at<_vec3>(j);

            _ivec3 P{ int32_t(p[0]*100.0f) , int32_t(p[1]*100.0f) , int32_t(p[2]*100.0f) };

            if( posToIndex.insert( { {P[0], P[1], P[2]}, index }).second)
            {
                NEW_POS.push_back(p);
                if(!_NOR.empty())
                    NEW_NOR.push_back(_NOR.at<_vec3>(j));
                if(!_UV.empty())
                    NEW_UV.push_back(_UV.at<_vec2>(j));
                index++;
            }
        }

        std::vector<uint32_t> newINDEX;
        for(uint32_t j=0;j<_INDEX.attributeCount();j++)
        {
            auto i = _INDEX.at<uint32_t>(j);
            auto p = _POS.at<_vec3>(i);
            _ivec3 P{ int32_t(p[0]*100.0f) , int32_t(p[1]*100.0f) , int32_t(p[2]*100.0f) };
            newINDEX.push_back( posToIndex.at({P[0],P[1],P[2]}) );
        }

        INDEX      = newINDEX;
        POSITION   = NEW_POS;
        NORMAL     = NEW_NOR;
        TEXCOORD_0 = NEW_UV;
    }

    /**
     * @brief rebuildNormals
     *
     * Recalculate the normals for each vertex. Normals are calculated as the average
     * of the face normals attached to the vertex
     */
    void rebuildNormals()
    {
        //using _vec2 = std::array<float,2>;
        using _vec3 = std::array<float,3>;

        {
            auto & I = INDEX;
            auto & P = POSITION;
            std::vector< _vec3 > normals(P.attributeCount(), _vec3({0,0,0}));

            auto iC = I.attributeCount();

            for(size_t j=0; j< iC; j+=3)
            {
                auto i0 = I.at<uint32_t>(j);
                auto i1 = I.at<uint32_t>(j+1);
                auto i2 = I.at<uint32_t>(j+2);

                assert(i0 < vertexCount());
                assert(i1 < vertexCount());
                assert(i2 < vertexCount());

                auto p0 = P.at<_vec3>(i0);
                auto p1 = P.at<_vec3>(i1);
                auto p2 = P.at<_vec3>(i2);

                decltype(p0) v1, v2;
                v1[0] = p1[0] - p0[0];
                v1[1] = p1[1] - p0[1];
                v1[2] = p1[2] - p0[2];

                v2[0] = p2[0] - p0[0];
                v2[1] = p2[1] - p0[1];
                v2[2] = p2[2] - p0[2];

                auto & x = v1;
                auto & y = v2;

                _vec3 n = {
                    x[1] * y[2] - y[1] * x[2],
                    x[2] * y[0] - y[2] * x[0],
                    x[0] * y[1] - y[0] * x[1] };


                normals[i0][0] += n[0];
                normals[i1][0] += n[0];
                normals[i2][0] += n[0];

                normals[i0][1] += n[1];
                normals[i1][1] += n[1];
                normals[i2][1] += n[1];

                normals[i0][2] += n[2];
                normals[i1][2] += n[2];
                normals[i2][2] += n[2];
            }

            for(auto & n : normals)
            {
                auto L = 1.0f / std::sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
                n[0] *= L;
                n[1] *= L;
                n[2] *= L;
            }

            NORMAL = normals;
        }
    }
};


/**
 * @brief translateMesh
 * @param M
 * @param x
 * @param y
 * @param z
 *
 * Adds {x,y,z} to each position value
 */
inline void translateMesh(MeshPrimitive & M, float x, float y, float z)
{
    auto & pos = M.POSITION;

    auto totalCount = pos.size();
    auto numComp    = pos.getNumComponents();

    switch(numComp)
    {
        case 1:
            for(uint32_t i=0;i<totalCount;i++)
            {
                pos.set<float>(i, pos.get<float>(i)+x);
            }
            break;
        case 2:
            for(uint32_t i=0;i<totalCount;i+=2)
            {
                pos.set<float>(i, pos.get<float>(i)+x);
                pos.set<float>(i+1, pos.get<float>(i+1)+y);
            }
            break;
        case 3:
            for(uint32_t i=0;i<totalCount;i+=3)
            {
                pos.set<float>(i, pos.get<float>(i)+x);
                pos.set<float>(i+1, pos.get<float>(i+1)+y);
                pos.set<float>(i+2, pos.get<float>(i+2)+z);
            }
            break;
    }
}

/**
 * @brief Box
 * @param dx
 * @param dy
 * @param dz
 * @return
 *
 * Create a box mesh with side lengths (dx,dy,dz)
 */
inline MeshPrimitive Box(float dx , float dy , float dz )
{
    using _vec2 = std::array<float,2>;
    using _vec3 = std::array<float,3>;

    MeshPrimitive M;

    auto & P = M.POSITION;
    auto & N = M.NORMAL;
    auto & U = M.TEXCOORD_0;
    auto & I = M.INDEX;


//       |       Position                           |   UV         |     Normal    |
        P.push_back( _vec3{0.0f - 0.5f*dx  ,0.0f - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,0.0f}) ; N.push_back( _vec3{0.0f,  0.0f,  1.0f}) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,0.0f - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,0.0f}) ; N.push_back( _vec3{0.0f,  0.0f,  1.0f}) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,dy   - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,1.0f}) ; N.push_back( _vec3{0.0f,  0.0f,  1.0f}) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,0.0f - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,0.0f}) ; N.push_back( _vec3{0.0f,  0.0f,  1.0f}) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,dy   - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,1.0f}) ; N.push_back( _vec3{0.0f,  0.0f,  1.0f}) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,dy   - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,1.0f}) ; N.push_back( _vec3{0.0f,  0.0f,  1.0f}) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,dy   - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,1.0f}) ; N.push_back( _vec3{0.0f,  0.0f, -1.0f}) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,dy   - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,1.0f}) ; N.push_back( _vec3{0.0f,  0.0f, -1.0f}) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,0.0f - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,0.0f}) ; N.push_back( _vec3{0.0f,  0.0f, -1.0f}) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,dy   - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,1.0f}) ; N.push_back( _vec3{0.0f,  0.0f, -1.0f}) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,0.0f - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,0.0f}) ; N.push_back( _vec3{0.0f,  0.0f, -1.0f}) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,0.0f - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,0.0f}) ; N.push_back( _vec3{0.0f,  0.0f, -1.0f}) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,0.0f - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,0.0f}) ; N.push_back( _vec3{-1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,0.0f - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,0.0f}) ; N.push_back( _vec3{-1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,dy   - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,1.0f}) ; N.push_back( _vec3{-1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,0.0f - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,0.0f}) ; N.push_back( _vec3{-1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,dy   - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,1.0f}) ; N.push_back( _vec3{-1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,dy   - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,1.0f}) ; N.push_back( _vec3{-1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,dy   - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,1.0f}) ; N.push_back( _vec3{1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,dy   - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,1.0f}) ; N.push_back( _vec3{1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,0.0f - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,0.0f}) ; N.push_back( _vec3{1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,dy   - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,1.0f}) ; N.push_back( _vec3{1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,0.0f - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,0.0f}) ; N.push_back( _vec3{1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,0.0f - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,0.0f}) ; N.push_back( _vec3{1.0f, 0.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,0.0f - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,0.0f}) ; N.push_back( _vec3{0.0f,-1.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,0.0f - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,0.0f}) ; N.push_back( _vec3{0.0f,-1.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,0.0f - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,1.0f}) ; N.push_back( _vec3{0.0f,-1.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,0.0f - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,0.0f}) ; N.push_back( _vec3{0.0f,-1.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,0.0f - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,1.0f}) ; N.push_back( _vec3{0.0f,-1.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,0.0f - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,1.0f}) ; N.push_back( _vec3{0.0f,-1.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,dy   - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,1.0f}) ; N.push_back( _vec3{0.0f, 1.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,dy   - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,1.0f}) ; N.push_back( _vec3{0.0f, 1.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,dy   - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,0.0f}) ; N.push_back( _vec3{0.0f, 1.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,dy   - 0.5f*dy  ,dz   -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,1.0f}) ; N.push_back( _vec3{0.0f, 1.0f,  0.0f }) ;
        P.push_back( _vec3{dx   - 0.5f*dx  ,dy   - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{1.0f,0.0f}) ; N.push_back( _vec3{0.0f, 1.0f,  0.0f }) ;
        P.push_back( _vec3{0.0f - 0.5f*dx  ,dy   - 0.5f*dy  ,0.0f -0.5f*dz} ) ;  U.push_back( _vec2{0.0f,0.0f}) ; N.push_back( _vec3{0.0f, 1.0f,  0.0f }) ;

    //=========================
    // Edges of the triangle : postion delta


    //=========================
    I.init(eComponentType::UNSIGNED_INT, eType::SCALAR);
    for( uint32_t j=0;j<36;j++)
        I.push_back( j );

    {
        auto & dc = M.primitives.emplace_back();
        dc.indexOffset  = 0;
        dc.vertexOffset = 0;
        dc.vertexCount  = static_cast<uint32_t>(M.vertexCount());
        dc.indexCount   = static_cast<uint32_t>(M.indexCount());
        dc.topology     = gul::Topology::TRIANGLE_LIST;
    }

    return M;
}

inline MeshPrimitive Box(float dx )
{
    return Box(dx,dx,dx);
}

/**
 * @brief Grid
 * @param length - length of the grid
 * @param width - width of the grid
 * @param dl - grid line spacing in the length dimension
 * @param dw - grid line spacing in the width dimension
 * @param majorL -
 * @param majorW
 * @param lscale
 * @param wscale
 * @return
 *
 * Return a grid mesh. Attributes: POSITION, COLOR
 */
inline MeshPrimitive Grid(int length, int width, int dl=1, int dw=1, int majorL=5, int majorW=5, float lscale=1.0f, float wscale=1.0f)
{
    using _vec3  = std::array<float,3>;
    using _uvec4 = std::array<uint8_t,4>;

    MeshPrimitive M;
    M.topology = Topology::LINE_LIST;
    auto & P = M.POSITION;
    auto & C = M.COLOR_0;

    //_uvec4 xColor{1,1,1,255};
    _uvec4 xColor{80,80,80,255};
    _uvec4 majorColor{128,128,128,255};
   // _uvec4 minorColor{255,0,0,255};
    _uvec4 borderColor{255,255,255,255};

    for(int x=-length;x<=length;x+=dl)
    {
        _vec3 p0{ static_cast<float>(x)*lscale, 0.0f, static_cast<float>(-width)*wscale };
        _vec3 p1{ static_cast<float>(x)*lscale, 0.0f, static_cast<float>( width)*wscale };

        P.push_back(p0);
        P.push_back(p1);

        if( x == -length || x==length)
        {
            C.push_back(borderColor);
            C.push_back(borderColor);
        }
        else if( x % majorL==0)
        {
            C.push_back(majorColor);
            C.push_back(majorColor);
        }
        else
        {
            C.push_back(xColor);
            C.push_back(xColor);
        }
    }

    for(int x=-width;x<=width;x+=dw)
    {
        _vec3 p0{ static_cast<float>( length)*lscale, 0.0, static_cast<float>(x)*wscale };
        _vec3 p1{ static_cast<float>(-length)*lscale, 0.0, static_cast<float>(x)*wscale };

        P.push_back(p0);
        P.push_back(p1);

        if( x == -length || x==length)
        {
            C.push_back(borderColor);
            C.push_back(borderColor);
        }
        else if( x % majorW==0)
        {
            C.push_back(majorColor);
            C.push_back(majorColor);
        }
        else
        {
            C.push_back(xColor);
            C.push_back(xColor);
        }
    }

    {
        auto & dc = M.primitives.emplace_back();
        dc.indexOffset  = 0;
        dc.vertexOffset = 0;
        dc.vertexCount  = static_cast<uint32_t>(M.vertexCount());
        dc.indexCount   = static_cast<uint32_t>(M.indexCount());
        dc.topology     = gul::Topology::TRIANGLE_LIST;
    }
    return M;
}


/**
 * @brief Sphere
 * @param radius
 * @param rings
 * @param sectors
 * @return
 *
 * Return a sphere mesh
 */
inline MeshPrimitive Sphere(float radius , uint32_t rings=20, uint32_t sectors=20)
{
    using _vec2 = std::array<float,2>;
    using _vec3 = std::array<float,3>;

    MeshPrimitive M;

    auto & P = M.POSITION;
    auto & N = M.NORMAL;
    auto & U = M.TEXCOORD_0;
    auto & I = M.INDEX;


    float const R = 1.0f / static_cast<float>(rings-1);
    float const S = 1.0f / static_cast<float>(sectors-1);
    unsigned int r, s;

    for(r = 0; r < rings; r++)
    {
        auto rf = static_cast<float>(r);
        for(s = 0; s < sectors; s++)
        {
            auto sf = static_cast<float>(s);

            float const y = std::sin( -3.141592653589f*0.5f + 3.141592653589f * rf * R );
            float const x = std::cos(2*3.141592653589f * sf * S) * std::sin( 3.141592653589f * rf * R );
            float const z = std::sin(2*3.141592653589f * sf * S) * std::sin( 3.141592653589f * rf * R );

            P.push_back( _vec3{ radius*x ,radius*y ,radius*z} );
            U.push_back( _vec2{sf*S, rf*R} );
            N.push_back( _vec3{x,y,z} );
        }
    }

    I.init(eComponentType::UNSIGNED_INT, eType::SCALAR);
    for(r = 0 ; r < rings   - 1 ; r++)
    {
        for(s = 0 ; s < sectors - 1 ; s++)
        {
            I.push_back(  ( (r+1) * sectors + s) ); //0
            I.push_back(  ( (r+1) * sectors + (s+1) ) ); //1
            I.push_back(  (  r * sectors + (s+1) )); //2
            I.push_back(  ( (r+1) * sectors + s )); //0
            I.push_back(  (  r * sectors + (s+1) )); //2
            I.push_back(  (   r * sectors + s )); //3
        }
    }

    {
        auto & dc = M.primitives.emplace_back();
        dc.indexOffset  = 0;
        dc.vertexOffset = 0;
        dc.vertexCount  = static_cast<uint32_t>(M.vertexCount());
        dc.indexCount   = static_cast<uint32_t>(M.indexCount());
        dc.topology     = gul::Topology::TRIANGLE_LIST;
    }
    return M;
}

/**
 * @brief Cylinder
 * @param R
 * @param H
 * @param rSegments
 * @return
 *
 * Return a cylinder mesh
 */
inline MeshPrimitive Cylinder(float R=1.0f, float H=3.0f, uint32_t rSegments=16)
{
    using _vec2 = std::array<float,2>;
    using _vec3 = std::array<float,3>;

    MeshPrimitive M;


    std::vector<_vec3>    P;// = M.POSITION;  //[ vka2::PrimitiveAttribute::POSITION ];
    std::vector<_vec3>    N;// = M.NORMAL;    //[ vka2::PrimitiveAttribute::NORMAL ];
    std::vector<_vec2>    U;// = M.TEXCOORD_0;//[ vka2::PrimitiveAttribute::TEXCOORD_0 ];
    std::vector<uint32_t> I;// = M.INDEX;


    float dt = 2.0f * 3.141592653589f / static_cast<float>(rSegments);

    float t = 0;
    if(1)
    {
        for(uint32_t r=0 ; r<rSegments; r++)
        {
            _vec3 p{  R*std::cos(t) ,  R * std::sin(t),  0 };
            t += dt;
            P.push_back(p);
            N.push_back( _vec3{ std::cos(t), std::sin(t), 0.f } );
            U.push_back( _vec2{ t, 0} );
        }

        for(uint32_t r=0 ; r<rSegments; r++)
        {
            _vec3 p{  R*std::cos(t) ,  R * std::sin(t),  H };
            t += dt;
            P.push_back(p);
            N.push_back( _vec3{ std::cos(t), std::sin(t), 0.f } );
            U.push_back( _vec2{ t, 1} );
        }


        for(uint32_t i=0 ; i < rSegments; ++i)
        {
            const uint32_t a = (i + 0) % rSegments;
            const uint32_t b = (i + 1) % rSegments;
            const uint32_t c = b + rSegments;

            const uint32_t d = a + rSegments;

            I.push_back( static_cast<uint32_t>(a) );
            I.push_back( static_cast<uint32_t>(b) );
            I.push_back( static_cast<uint32_t>(c) );

            I.push_back( static_cast<uint32_t>(a) );
            I.push_back( static_cast<uint32_t>(c) );
            I.push_back( static_cast<uint32_t>(d) );
        }

        M.INDEX = I;
        M.POSITION = P;
        M.NORMAL = N;
        M.TEXCOORD_0 = U;
    }


    if(1)
    { // top cap

        MeshPrimitive M2;

        std::vector<_vec3>     P2;// = M2.POSITION;  //[ vka2::PrimitiveAttribute::POSITION ];
        std::vector<_vec3>     N2;// = M2.NORMAL;    //[ vka2::PrimitiveAttribute::NORMAL ];
        std::vector<_vec2>     U2;// = M2.TEXCOORD_0;//[ vka2::PrimitiveAttribute::TEXCOORD_0 ];
        std::vector<std::array<uint32_t,3> >  I2;// = M2.INDEX;

        t = 0;
        P2.push_back( _vec3{ 0.f, 0.f, H});
        N2.push_back( _vec3{ 0.f, 0.f, 1.f } );
        U2.push_back( _vec2{ 0.5f, 0.5f } );

        for(uint32_t r=0 ; r < rSegments; r++)
        {
            _vec3 p{  R * std::cos(t) ,  R * std::sin(t),  H };
            t += dt;
            P2.push_back(p);
            N2.push_back( _vec3{ 0.f, 0.f, 1.f } );
            U2.push_back( _vec2{ 0.5f+std::cos(t), 0.5f+std::sin(t)} );

            const uint32_t A = 0;
            const uint32_t B = r+1;
            const uint32_t C = (r+1)%rSegments+1 ;

            I2.push_back( std::array<uint32_t,3>({A,B,C}));
        }

        M2.POSITION   = P2;
        M2.NORMAL     = N2;
        M2.TEXCOORD_0 = U2;
        M2.INDEX      = I2;
        M2.INDEX.setType(eType::SCALAR);

        M.merge(M2, true);

        // bottom cap.
        if(1)
        {
            for(auto & p : P2)
                p[2] = 0.0f;

            for(auto & p : N2) // flip normals
            {
                p[0] *= -1.f;
                p[1] *= -1.f;
                p[2] *= -1.f;
            }
            for(auto & p : I2) // reverse winding order
            {
                std::swap(p[0], p[2]);
            }

            M2.INDEX = I2;
            M2.POSITION = P2;
            M2.NORMAL = N2;
            M2.TEXCOORD_0 = U2;
            M2.INDEX.setType(eType::SCALAR);

            M.merge(M2, true);
        }

    }

    {
        auto & dc = M.primitives.emplace_back();
        dc.indexOffset  = 0;
        dc.vertexOffset = 0;
        dc.vertexCount  = static_cast<uint32_t>(M.vertexCount());
        dc.indexCount   = static_cast<uint32_t>(M.indexCount());
        dc.topology     = gul::Topology::TRIANGLE_LIST;
    }

    return M;
}

/**
 * @brief Imposter
 * @return
 *
 * An imposter is a simple quad in the XY plane with normal in the +Z direction
 */
inline MeshPrimitive Imposter(float sideLength=1.0f)
{
    MeshPrimitive M;

    using _vec2 = std::array<float,2>;
    using _vec3 = std::array<float,3>;

    auto & P = M.POSITION;
    auto & N = M.NORMAL;
    auto & I = M.INDEX;
    auto & U = M.TEXCOORD_0;

    P.push_back( _vec3{-sideLength,-sideLength,0});
    P.push_back( _vec3{ sideLength,-sideLength,0});
    P.push_back( _vec3{ sideLength, sideLength,0});
    P.push_back( _vec3{-sideLength, sideLength,0});

    U.push_back( _vec2{0.0f, 1.0f});
    U.push_back( _vec2{1.0f, 1.0f});
    U.push_back( _vec2{1.0f, 0.0f});
    U.push_back( _vec2{0.0f, 0.0f});

    N.push_back(_vec3{0,0,1});
    N.push_back(_vec3{0,0,1});
    N.push_back(_vec3{0,0,1});
    N.push_back(_vec3{0,0,1});

    I = std::vector<uint32_t>{0,1,2,0,2,3};

    {
        auto & dc = M.primitives.emplace_back();
        dc.indexOffset  = 0;
        dc.vertexOffset = 0;
        dc.vertexCount  = static_cast<uint32_t>(M.vertexCount());
        dc.indexCount   = static_cast<uint32_t>(M.indexCount());
        dc.topology     = gul::Topology::TRIANGLE_LIST;
    }
    return M;
}

/**
 * @brief revolve
 * @param XYpoints - pointer to numPoints*2 float values
 * @param numPoints - total number of points
 * @return
 *
 * Given a set of points in the XY plane, revolve the curve around
 * the Z-axis
 */
inline MeshPrimitive revolve(float const * XYpoints, size_t numPoints, size_t segments=10)
{
    using _vec2 = std::array<float,2>;
    using _vec3 = std::array<float,3>;

    MeshPrimitive M;

    std::vector< _vec3 > position;
    std::vector< _vec3 > normal;
    std::vector< _vec2 > uv;

    std::vector<uint32_t> indices;

    for(size_t k=0;k<segments+1;k++)
    {
        float t = ( float(k) / float(segments-1) );
        float th = ( float(k) / float(segments) ) * 2.0f * 3.141592653589f;

        for(size_t i=0;i<numPoints;i++)
        {
            if(k < segments)
            {
                float s = ( float(i) / float(numPoints-1) );

                float R = XYpoints[2*i+1];

                float xp = XYpoints[2*i];
                float yp = R * std::cos(th);
                float zp = R * std::sin(th);

                position.push_back( _vec3{{ xp,yp,zp}} );
                uv.push_back({s,t});
            }
        }
    }

    assert( position.size() == numPoints*segments);
    auto totalPoints = position.size();

    for(uint32_t k=0;k<segments;k++)
    {
        for(uint32_t i=0;i<numPoints-1;i++)
        {
            auto a = k     * numPoints + i;
            auto b = k     * numPoints + i+1;

            auto c = ( (k+1) * numPoints + i  ) % totalPoints;
            auto d = ( (k+1) * numPoints + i+1) % totalPoints;

            indices.push_back(uint32_t(b));
            indices.push_back(uint32_t(a));
            indices.push_back(uint32_t(c));
            indices.push_back(uint32_t(c));
            indices.push_back(uint32_t(d));
            indices.push_back(uint32_t(b));
        }
    }


    M.POSITION   = position;
    M.INDEX      = indices;
    M.TEXCOORD_0 = uv;

    M.rebuildNormals();

    {
        auto & dc = M.primitives.emplace_back();
        dc.indexOffset  = 0;
        dc.vertexOffset = 0;
        dc.vertexCount  = static_cast<uint32_t>(M.vertexCount());
        dc.indexCount   = static_cast<uint32_t>(M.indexCount());
        dc.topology     = gul::Topology::TRIANGLE_LIST;
    }

    return M;
}


inline MeshPrimitive Arrow(float bodyLength, float bodyRadius, float headLength, float headRadius)
{
    auto bl = bodyLength;
    auto br = bodyRadius;
    auto hl = headLength;
    auto hr = headRadius;

    std::vector<float> points;

    points.push_back( 0 );
    points.push_back( 0 );

    points.push_back( 0 );
    points.push_back( br );

    points.push_back( 0 );
    points.push_back( br );

    points.push_back( bl );
    points.push_back( br );

    points.push_back( bl );
    points.push_back( br );

    points.push_back( bl );
    points.push_back( hr );

    points.push_back( bl );
    points.push_back( hr );

    points.push_back( hl+bl );
    points.push_back( 0 );

    return revolve(points.data(), points.size()/2, 10 );
}


inline MeshPrimitive ReadOBJ(std::ifstream & in)
{
    using _vec2 = std::array<float,2>;
    using _vec3 = std::array<float,3>;

    std::vector< _vec3 > position;
    std::vector< _vec3 > normal;
    std::vector< _vec2 > uv;

    struct faceIndex
    {
        uint32_t p=0;
        uint32_t t=0;
        uint32_t n=0;
    };

    std::vector< faceIndex > quads;
    std::vector< faceIndex > tris;

    auto split = [](std::string s, std::string delimiter)
    {
        using namespace std;
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        string token;
        vector<string> res;

        while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }

        res.push_back (s.substr (pos_start));
        return res;
    };

    auto getFace = [&](std::string s) -> faceIndex
    {
        faceIndex F;
        auto S = split(s, "/");
        if(S.size() == 3)
        {
            F.p = static_cast<uint32_t>(std::stoi( S[0] ));
            if( S[1].size() != 0)
                F.t =  static_cast<uint32_t>(std::stoi(S[1]));
            if( S[2].size() != 0)
                F.n =  static_cast<uint32_t>(std::stoi(S[2]));

            return F;
        }
        else if(S.size() == 1)
        {
            F.p = static_cast<uint32_t>(std::stoi( S[0] ));
        }
        return F;
    };

    while(!in.eof())
    {
        std::string line;
        std::string fullLine;
        std::getline(in, fullLine);

        std::istringstream ins(fullLine);

        ins >> line;
        if(line == "v")
        {
            _vec3 p;
            ins  >> p[0];
            ins  >> p[1];
            ins  >> p[2];
            position.push_back(p);
        }
        else if(line == "vn")
        {
            _vec3 p;
            ins  >> p[0];
            ins  >> p[1];
            ins  >> p[2];
            normal.push_back(p);
        }
        else if(line == "vt")
        {
            _vec2 p;
            ins  >> p[0];
            ins  >> p[1];
            uv.push_back(p);
        }
        else if(line == "f")
        {
            std::string faceLine;

            if(fullLine.front() == 'f')
            {
                faceLine = fullLine.substr(2);
            }
            auto sp = split(faceLine, " ");

            if(sp.size() == 4)
            {
                for(auto & v : sp)
                {
                    faceIndex Fa = getFace(v);
                    quads.push_back(Fa);
                }
            }
            if(sp.size() == 3)
            {
                for(auto & v : sp)
                {
                    faceIndex Fa = getFace(v);
                    tris.push_back(Fa);
                }
            }
            //std::cout << faceLine << std::endl;
        }
        else
        {
            //std::string bah;
            //std::getline(in, bah);
          //  std::cout << line << std::endl;
        }
    }


    gul::MeshPrimitive M;

    std::vector<_vec3> POSITION;
    std::vector<_vec2> TEXCOORD;
    std::vector<_vec3> NORMAL;
    std::vector<uint32_t> INDEX;

    for(size_t i=0;i<tris.size(); i+= 3)
    {
        auto & I1 = tris[i];
        auto & I2 = tris[i+1];
        auto & I3 = tris[i+2];

        POSITION.push_back(position[I1.p-1]);
        POSITION.push_back(position[I2.p-1]);
        POSITION.push_back(position[I3.p-1]);

        if(I1.n*I2.n*I3.n > 0 )
        {
            NORMAL.push_back(normal[I1.n-1]);
            NORMAL.push_back(normal[I2.n-1]);
            NORMAL.push_back(normal[I3.n-1]);
        }

        if(I1.t*I2.t*I3.t > 0 )
        {
            TEXCOORD.push_back(uv[I1.t-1]);
            TEXCOORD.push_back(uv[I2.t-1]);
            TEXCOORD.push_back(uv[I3.t-1]);
        }
    }

    for(size_t i=0;i<quads.size(); i+= 4)
    {
        auto & I1 = quads[i];
        auto & I2 = quads[i+1];
        auto & I3 = quads[i+2];
        auto & I4 = quads[i+3];

        POSITION.push_back(position[I1.p - 1]);
        POSITION.push_back(position[I2.p - 1]);
        POSITION.push_back(position[I3.p - 1]);
        POSITION.push_back(position[I1.p - 1]);
        POSITION.push_back(position[I3.p - 1]);
        POSITION.push_back(position[I4.p - 1]);

        NORMAL.push_back(normal[I1.n - 1] );
        NORMAL.push_back(normal[I2.n - 1] );
        NORMAL.push_back(normal[I3.n - 1] );
        NORMAL.push_back(normal[I1.n - 1] );
        NORMAL.push_back(normal[I3.n - 1] );
        NORMAL.push_back(normal[I4.n - 1] );

        TEXCOORD.push_back(uv[I1.t - 1] );
        TEXCOORD.push_back(uv[I2.t - 1] );
        TEXCOORD.push_back(uv[I3.t - 1] );
        TEXCOORD.push_back(uv[I1.t - 1] );
        TEXCOORD.push_back(uv[I3.t - 1] );
        TEXCOORD.push_back(uv[I4.t - 1] );
    }
    uint32_t i=0;
    for(auto & x : POSITION)
    {
        (void)x;
        INDEX.push_back(i++);
    }

    M.POSITION = std::move(POSITION);
    M.INDEX = std::move(INDEX);

    if(NORMAL.size() == 0)
    {
        M.rebuildNormals();
    }
    else
    {
        M.NORMAL = std::move(NORMAL);
    }
    M.TEXCOORD_0 = std::move(TEXCOORD);

    return M;
}

}

#endif

