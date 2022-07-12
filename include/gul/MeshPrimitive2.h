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
    UNKNOWN = 0,
    SCALAR  = 1,
    VEC2    = 2,
    VEC3    = 3,
    VEC4    = 4
};


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
    VertexAttribute& operator=(std::vector<T> const & v)
    {
        m_data.resize( v.size() * sizeof(T));
        std::memcpy(m_data.data(), v.data(), m_data.size());
        return *this;
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
        std::memcpy(&v, m_data.data() + index * getAttributeSize() + componentIndex * getComponentSizeOf(m_componentType), sizeof(T));
        return v;
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
        return static_cast<uint32_t>(m_type);
    }

    static uint32_t getComponentSizeOf(eComponentType c)
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
     * @brief getAttributeSize
     * @return
     *
     * Returns the size of the attribute. If it returns 0 it means that
     * the attribute type has not been set
     */
    uint32_t getAttributeSize() const
    {
        return getComponentSizeOf(m_componentType) * getNumComponents();
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
        return m_data.size() / getAttributeSize();
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
     * Merge B to the end of the attribute vector and return the index
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

        auto d_out = static_cast<uint8_t*>(data);
        for(uint64_t i=0;i<c;i++)
        {
            std::memcpy(d_out, &m_data[i], s);
            d_out += stride;
        }
    }

    void clear()
    {
        m_data.clear();
    }

    std::vector<uint8_t> m_data;
    eComponentType       m_componentType = eComponentType::UNKNOWN;
    eType                m_type = eType::UNKNOWN;
};


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

    attribute_type POSITION   = attribute_type(eComponentType::FLOAT, eType::VEC3);
    attribute_type NORMAL     = attribute_type(eComponentType::FLOAT, eType::VEC3);
    attribute_type TANGENT    = attribute_type(eComponentType::FLOAT, eType::VEC4);
    attribute_type TEXCOORD_0 = attribute_type(eComponentType::FLOAT, eType::VEC2);
    attribute_type TEXCOORD_1 = attribute_type(eComponentType::FLOAT, eType::VEC2);
    attribute_type COLOR_0    = attribute_type(eComponentType::UNSIGNED_BYTE, eType::VEC4);
    attribute_type JOINTS_0   = attribute_type(eComponentType::UNSIGNED_SHORT, eType::VEC4);
    attribute_type WEIGHTS_0  = attribute_type(eComponentType::FLOAT, eType::VEC4);

    attribute_type INDEX      = attribute_type(eComponentType::UNSIGNED_INT, eType::SCALAR);

    Topology       topology = Topology::TRIANGLE_LIST;

    std::vector<DrawCall> subMeshes;

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
     * @brief calculateDeviceSize
     * @return
     *
     * Calculate the amount of bytes this mesh takes on the
     * the GPU if all vertices were placed one after the
     * other
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

    size_t indexCount() const
    {
        return INDEX.attributeCount();
    }
    size_t vertexCount() const
    {
        return POSITION.attributeCount();
    }

    DrawCall getDrawCall() const
    {
        DrawCall dc;
        dc.indexOffset  = static_cast<int32_t>(0);
        dc.vertexOffset = static_cast<int32_t>(0);
        dc.vertexCount  = static_cast<uint32_t>(vertexCount());
        dc.indexCount   = static_cast<uint32_t>(indexCount());
        dc.topology     = topology;
        return dc;
    }

    DrawCall merge(MeshPrimitive const & P)
    {
        DrawCall dc;

        dc.indexOffset  = static_cast<int32_t>(indexCount() );
        dc.vertexOffset = static_cast<int32_t>(vertexCount());
        dc.vertexCount = static_cast<uint32_t>(P.vertexCount());
        dc.indexCount  = static_cast<uint32_t>(P.indexCount() );

        if( isSimilar(P) )
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

            subMeshes.push_back(dc);
            return dc;
        }
        throw std::runtime_error("MeshPrimitives are not similar");
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
            stride += v->getAttributeSize();
        }
        return stride;
    }
    /**
     * @brief copySequential
     * @param data
     * @return
     *
     * Copies all the vertex attributes sequentually into the provided buffer
     * and returns the stride from one vertex to the next
     *
     * [p0,n0,t0,p1,n1,t1...]
     *
     *
     */
    inline uint64_t copyVertexAttributesInterleaved(void * data) const
    {
        uint64_t stride = calculateInterleavedStride();
        uint64_t offset = 0;

        for(auto * v :  {  &POSITION,
                           &NORMAL,
                           &TANGENT,
                           &TEXCOORD_0,
                           &TEXCOORD_1,
                           &COLOR_0,
                           &JOINTS_0,
                           &WEIGHTS_0})
        {
            auto & V = *v;
            V.strideCopy( static_cast<uint8_t*>(data)+offset, stride);
            offset += V.getAttributeSize();
        }
        return stride;
    }


    /**
     * @brief getVertexCount
     * @return
     *
     * Returns the number of vertices in the mesh. The number of veertices
     * is the munimum number of vertices in all the attributes
     */
    uint64_t getVertexCount() const
    {
        uint64_t vertexCount = std::numeric_limits<uint64_t>::max();
        for(auto * v :  {  &POSITION,
                           &NORMAL,
                           &TANGENT,
                           &TEXCOORD_0,
                           &TEXCOORD_1,
                           &COLOR_0,
                           &JOINTS_0,
                           &WEIGHTS_0})
        {
            auto attrSize = v->getAttributeSize();
            (void)attrSize;
            if(v->attributeCount() != 0)
                vertexCount = std::min<uint64_t>(vertexCount, v->attributeCount());
        }
        return vertexCount;
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
        auto vertexCount = getVertexCount();
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
                auto attrSize = v->getAttributeSize();

                auto count = v->attributeCount();
                assert( count * attrSize <= v->m_data.size());
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
     * @brief getVertexSize
     * @return
     *
     * Returns the size of the vertrex in bytes if all the
     * attributes were interleaved
     */
    uint64_t getVertexSize() const
    {
        return calculateInterleavedStride();
    }

    inline uint64_t calculateInterleavedBufferSize() const
    {
        return getVertexSize() * getVertexCount();
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
        uint32_t j     = 0;

        auto vCount = getVertexCount();
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
     * Recalculate the normals for each vertex
     */
    void rebuildNormals()
    {
        using _vec2 = std::array<float,2>;
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

                auto p0 = P.at<_vec3>(i0);
                auto p1 = P.at<_vec3>(i1);
                auto p2 = P.at<_vec3>(i2);

                decltype(p0) v1, v2;
                v1[0] = p1[0] - p0[1];
                v1[1] = p1[1] - p0[1];
                v1[2] = p1[2] - p0[2];

                v2[0] = p2[0] - p0[1];
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
                auto L = 1.0f / n[0]*n[0] + n[1]*n[1] + n[2]*n[2];
                n[0] *= L;
                n[1] *= L;
                n[2] *= L;
            }

            NORMAL = normals;
        }
    }
};

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
    I.init(eComponentType::UNSIGNED_SHORT, eType::SCALAR);
    for( uint16_t j=0;j<36;j++)
        I.push_back( j );

    return M;
}

inline MeshPrimitive Box(float dx )
{
    return Box(dx,dx,dx);
}

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

    return M;
}

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

    I.init(eComponentType::UNSIGNED_SHORT, eType::SCALAR);
    for(r = 0 ; r < rings   - 1 ; r++)
    {
        for(s = 0 ; s < sectors - 1 ; s++)
        {
            I.push_back(  static_cast<uint16_t>( (r+1) * sectors + s) ); //0
            I.push_back(  static_cast<uint16_t>( (r+1) * sectors + (s+1) ) ); //1
            I.push_back(  static_cast<uint16_t>(  r * sectors + (s+1) )); //2
            I.push_back(  static_cast<uint16_t>( (r+1) * sectors + s )); //0
            I.push_back(  static_cast<uint16_t>(  r * sectors + (s+1) )); //2
            I.push_back(  static_cast<uint16_t>(   r * sectors + s )); //3
        }
    }

    return M;
}

/**
 * @brief Imposter
 * @return
 *
 * An imposter is a simple quad in the XY plane
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

    return M;
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
