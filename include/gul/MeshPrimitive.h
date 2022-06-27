#ifndef GUL_MESH_PRIMITIVE_H
#define GUL_MESH_PRIMITIVE_H

#include <variant>
#include <vector>
#include <array>
#include <cstring>
#include <tuple>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <glm/glm.hpp>

namespace gul
{

#define _VECTYPES(_T) \
std::vector< _T                             >, \
std::vector< glm::vec<2, _T, glm::defaultp> >, \
std::vector< glm::vec<3, _T, glm::defaultp> >, \
std::vector< glm::vec<4, _T, glm::defaultp> >

using VertexAttribute_v = std::variant<
                            _VECTYPES(float) ,
                            _VECTYPES(double) ,
                            _VECTYPES(int32_t) ,
                            _VECTYPES(uint32_t) ,
                            _VECTYPES(int16_t) ,
                            _VECTYPES(uint16_t) ,
                            _VECTYPES(int8_t) ,
                            _VECTYPES(uint8_t) ,

                            std::vector< glm::mat3 >,
                            std::vector< glm::mat4 >,
                            std::vector< glm::dmat3 >,
                            std::vector< glm::dmat4 >
                    >;

#undef _VECTYPES

template<typename T>
VertexAttribute_v generateFromAccessor_t(uint32_t numComponents)
{
    switch(numComponents)
    {
        case 1: return std::vector< T >();
            break;
        case 2: return std::vector< glm::vec<2, T, glm::defaultp > >();
            break;
        case 3: return std::vector< glm::vec<3, T, glm::defaultp > >();
            break;
        case 4: return std::vector< glm::vec<4, T, glm::defaultp > >();
            break;
        default:
            throw std::runtime_error("Not supported for vertex attributes");
    }
}

inline VertexAttribute_v generateFromGLTFAccessor(uint32_t GL_componentType, uint32_t numComponents)
{
    switch(GL_componentType)
    {
        case 5120: return generateFromAccessor_t<int8_t>(numComponents);
        case 5121: return generateFromAccessor_t<uint8_t>(numComponents);
        case 5122: return generateFromAccessor_t<int16_t>(numComponents);
        case 5123: return generateFromAccessor_t<uint16_t>(numComponents);
        case 5124: return generateFromAccessor_t<int32_t>(numComponents);
        case 5125: return generateFromAccessor_t<uint32_t>(numComponents);
        case 5126: return generateFromAccessor_t<float>(numComponents);
        case 5130: return generateFromAccessor_t<double>(numComponents);
        default:
            throw std::runtime_error("Not supported component type");
    }
}

template<typename T>
inline constexpr uint32_t getNumComponents()
{
    if constexpr( std::is_fundamental<T>::value )
    {
        return 1u;
    }
    else // glm type
    {
        return static_cast<uint32_t>(T::length());
    }
}

/**
 * @brief byteSize
 * @param v
 * @return
 *
 * Returns the byte size of the entire attribute
 */
inline uint64_t VertexAttributeByteSize(VertexAttribute_v const & v)
{
    return std::visit( [&](auto && arg)
    {
        using V = std::decay_t<decltype(arg)>; //std::vector<attr_type>
        using attr_type = typename V::value_type;
        return arg.size() * sizeof(attr_type);
    }, v);
}

/**
 * @brief attributeSize
 * @param v
 * @return
 *
 * Returns the size of the attribute array's value_type
 */
inline size_t VertexAttributeSizeOf(VertexAttribute_v const & v)
{
    return std::visit( [&](auto && arg)
    {
        using V = std::decay_t<decltype(arg)>; //std::vector<attr_type>
        using attr_type = typename V::value_type;
        return sizeof(attr_type);
    }, v);
}

/**
 * @brief attributeCount
 * @param v
 * @return
 * Returns the number of attributes
 */
inline size_t VertexAttributeCount(VertexAttribute_v const & v)
{
    return std::visit( [&](auto && arg)
    {
        return arg.size();
    }, v);
}

/**
 * @brief attributeCount
 * @param v
 * @return
 * Returns the number of attributes
 */
inline size_t VertexAttributeNumComponents(VertexAttribute_v const & v)
{
    return std::visit( [&](auto && arg)
    {
        using V = std::decay_t<decltype(arg)>; //std::vector<attr_type>
        using attr_type = typename V::value_type;
        return getNumComponents<attr_type>();
    }, v);
}

/**
 * @brief VertexAttributeMerge
 * @param v
 * @return
 *
 * Merges all the values from B into A, returns the size of A before the merge.
 */
inline size_t VertexAttributeMerge(VertexAttribute_v & A, VertexAttribute_v const & B)
{
    return std::visit( [&](auto && arg)
    {
        using V = std::decay_t<decltype(arg)>; //std::vector<attr_type>

        auto c = arg.size();
        auto & b = std::get<V>(B);
        arg.insert(arg.end(), b.begin(), b.end());
        return c;
    }, A);
}

/**
 * @brief copySequential
 * @param data
 * @param V
 * @return
 *
 * Copies the attribute data sequentially
 * if given two attributes p and n,
 * the data is copied as follows
 *
 * p0,p1,p2,p3....n0,n1,n2,n3
 */
inline std::vector<size_t> VertexAttributeCopySequential(void * data, std::vector<VertexAttribute_v const*> const & V)
{
    std::vector<size_t> offsets;
    size_t off=0;

    for(auto & v : V)
    {
        if(v)
        {
            auto count = VertexAttributeCount(*v);
            if(count)
            {
                offsets.push_back( off);
                off += VertexAttributeByteSize(*v);
            }
            else
            {
                offsets.push_back(0);
            }
        }
        else
        {
            offsets.push_back(0);
        }
    }

    auto dOut = static_cast<uint8_t*>(data);
    for(auto & v : V)
    {
        if( v != nullptr)
        {
            std::visit( [&](auto && arg)
                    {
                        using T = std::decay_t<decltype(arg)>;
                        std::memcpy(dOut, arg.data(), arg.size()*sizeof(typename T::value_type));
                        dOut += arg.size()*sizeof(typename T::value_type);
                    }, *v);
        }
    }
    return offsets;
}

/**
 * @brief strideCopy
 * @param start
 * @param v
 * @param stride
 *
 * Performs a stride copy of the attribute's data
 *
 * stride should be at least as large as attributeSize(v), otherwise
 * data will be overwrritten.
 */
inline void VertexAttributeStrideCopy(void * start, VertexAttribute_v const &v, size_t stride)
{
    return std::visit( [stride, start](auto && arg)
    {
        auto dOut = static_cast<uint8_t*>(start);
        for(auto & a : arg)
        {
            std::memcpy(dOut, &a, sizeof(a));
            dOut += stride;
        }
    }, v);
}


/**
 * @brief copyInterleaved
 * @param data
 * @param V
 * @param startIndex
 * @param count
 *
 * Copies the attribute data into the buffer but
 * interleaves each attribute.
 *
 * if given two attributes p and n,
 * the data is copied as follows
 *
 * p0,n0,p1,n1,p2,n2...
 */
inline size_t VertexAttributeInterleaved(void * data, std::vector<VertexAttribute_v const*> const & V, size_t startIndex=0, size_t count=std::numeric_limits<size_t>::max())
{
    //size_t byteSize=0;
    uint64_t stride=0;

    uint8_t * out = static_cast<uint8_t*>(data);
    //size_t off=0;

    (void)startIndex;
    for(auto & v : V)
    {
        auto attrCount = VertexAttributeCount(*v);
        if(attrCount)
        {
            count = std::min(attrCount,count);
            stride += VertexAttributeSizeOf(*v);
        }
    }

    size_t offset = 0;
    for(auto & v : V)
    {
        auto attrCount = VertexAttributeCount(*v);
        if(attrCount)
        {
            VertexAttributeStrideCopy(out + offset, *v, stride);
            offset += VertexAttributeSizeOf(*v);
        }
    }

    return stride * count;
}

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
 * The attributes are
 */
struct MeshPrimitive
{
    using attribute_type = VertexAttribute_v;

    attribute_type POSITION   = std::vector<glm::vec3>  ();
    attribute_type NORMAL     = std::vector<glm::vec3>  ();
    attribute_type TANGENT    = std::vector<glm::vec3>  ();
    attribute_type TEXCOORD_0 = std::vector<glm::vec2>  ();
    attribute_type TEXCOORD_1 = std::vector<glm::vec2>  ();
    attribute_type COLOR_0    = std::vector<glm::u8vec4>();
    attribute_type JOINTS_0   = std::vector<glm::u16vec4>();
    attribute_type WEIGHTS_0  = std::vector<glm::vec4>();

    attribute_type INDEX    = std::vector<uint32_t>();

    Topology       topology = Topology::TRIANGLE_LIST;

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
            std::visit([](auto&& arg)
            {
                arg.clear();
            }, *attr );
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

        size += VertexAttributeByteSize(POSITION  );
        size += VertexAttributeByteSize(NORMAL    );
        size += VertexAttributeByteSize(TANGENT   );
        size += VertexAttributeByteSize(TEXCOORD_0);
        size += VertexAttributeByteSize(TEXCOORD_1);
        size += VertexAttributeByteSize(COLOR_0   );
        size += VertexAttributeByteSize(JOINTS_0  );
        size += VertexAttributeByteSize(WEIGHTS_0 );
        size += VertexAttributeByteSize(INDEX);

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
            POSITION  .index() == P.POSITION   .index() &&
            NORMAL    .index() == P.NORMAL     .index() &&
            TANGENT   .index() == P.TANGENT    .index() &&
            TEXCOORD_0.index() == P.TEXCOORD_0 .index() &&
            TEXCOORD_1.index() == P.TEXCOORD_1 .index() &&
            COLOR_0   .index() == P.COLOR_0    .index() &&
            JOINTS_0  .index() == P.JOINTS_0   .index() &&
            WEIGHTS_0 .index() == P.WEIGHTS_0  .index() &&
            INDEX     .index() == P.INDEX      .index();
    }

    size_t indexCount() const
    {
        return VertexAttributeCount(INDEX);
    }
    size_t vertexCount() const
    {
        return VertexAttributeCount(POSITION);
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
            VertexAttributeMerge(POSITION  , P.POSITION  );
            VertexAttributeMerge(NORMAL    , P.NORMAL    );
            VertexAttributeMerge(TANGENT   , P.TANGENT   );
            VertexAttributeMerge(TEXCOORD_0, P.TEXCOORD_0);
            VertexAttributeMerge(TEXCOORD_1, P.TEXCOORD_1);
            VertexAttributeMerge(COLOR_0   , P.COLOR_0   );
            VertexAttributeMerge(JOINTS_0  , P.JOINTS_0  );
            VertexAttributeMerge(WEIGHTS_0 , P.WEIGHTS_0 );
            VertexAttributeMerge(INDEX     , P.INDEX     );
            return dc;
        }
        throw std::runtime_error("MeshPrimitives are not similar");
    }

    /**
     * @brief copySequential
     * @param data
     * @return
     *
     * Copies all the vertex attributes sequentually into the provided buffer
     * and returns the offsets of each attribute
     *
     * [p0,p1,p2,n0,n1,n2,t0,t1,t2...]
     *
     *
     */
    inline std::vector<size_t> copySequential(void * data) const
    {
        return VertexAttributeCopySequential(data,
                                      {
                                          &POSITION,
                                          &NORMAL,
                                          &TANGENT,
                                          &TEXCOORD_0,
                                          &TEXCOORD_1,
                                          &COLOR_0,
                                          &JOINTS_0,
                                          &WEIGHTS_0,
                                          &INDEX
                                      });

    }

    inline size_t copyVertexAttributesInterleaved(void * data) const
    {
        size_t attrCount=0;
        auto stride = calculateInterleavedStride();
        uint8_t * offset = static_cast<uint8_t*>(data);
        for(auto & V : { &POSITION,
                         &NORMAL,
                         &TANGENT,
                         &TEXCOORD_0,
                         &TEXCOORD_1,
                         &COLOR_0,
                         &JOINTS_0,
                         &WEIGHTS_0})
        {
            auto count = gul::VertexAttributeCount(*V);
            if( count != 0)
            {
                VertexAttributeStrideCopy(offset, *V, stride);
                offset += gul::VertexAttributeSizeOf(*V);
                attrCount = std::max(count, attrCount);
            }
        }
        return attrCount * stride;
        //return VertexAttributeInterleaved(data,
        //                              {
        //                                  &POSITION,
        //                                  &NORMAL,
        //                                  &TANGENT,
        //                                  &TEXCOORD_0,
        //                                  &TEXCOORD_1,
        //                                  &COLOR_0,
        //                                  &JOINTS_0,
        //                                  &WEIGHTS_0,
        //                              });

    }

    inline size_t copyIndex(void * data) const
    {
        return std::visit( [data](auto && arg)
        {
            using type_ = typename std::decay_t<decltype(arg)>::value_type;
            std::memcpy(data, arg.data(), arg.size() * sizeof(type_));
            return arg.size() * sizeof(type_);
        }, INDEX);

    }

    inline size_t calculateInterleavedStride() const
    {
        size_t stride = 0;
        for(auto & V : { &POSITION,
                         &NORMAL,
                         &TANGENT,
                         &TEXCOORD_0,
                         &TEXCOORD_1,
                         &COLOR_0,
                         &JOINTS_0,
                         &WEIGHTS_0})
        {
            auto count = gul::VertexAttributeCount(*V);
            if(count)
            {
                stride += gul::VertexAttributeSizeOf(*V);
            }
        }
        return stride;
    }

    inline uint64_t calculateInterleavedBufferSize() const
    {
        size_t bufferSize = 0;
        for(auto & V : { &POSITION,
                         &NORMAL,
                         &TANGENT,
                         &TEXCOORD_0,
                         &TEXCOORD_1,
                         &COLOR_0,
                         &JOINTS_0,
                         &WEIGHTS_0})
        {
            auto count = gul::VertexAttributeCount(*V);
            if(count)
            {

                bufferSize += gul::VertexAttributeByteSize(*V);
            }
        }
        return bufferSize;
    }

    /**
     * @brief fuseVertices
     *
     * Fuse near by vertices. This may not be accurate
     */
    void fuseVertices()
    {
        std::map< std::tuple<int32_t, int32_t, int32_t>, uint32_t> posToIndex;

        auto & _POS = std::get< std::vector<glm::vec3> >(POSITION);
        auto & _NOR = std::get< std::vector<glm::vec3> >(NORMAL);
        auto & _UV = std::get< std::vector<glm::vec2> >(TEXCOORD_0);

        auto & _INDEX = std::get< std::vector<uint32_t> >(INDEX);

        std::vector<glm::vec3> NEW_POS;
        std::vector<glm::vec3> NEW_NOR;
        std::vector<glm::vec2> NEW_UV;

        uint32_t index = 0;
        uint32_t j=0;
        for(auto & p : _POS)
        {
            glm::ivec3 P( p*100.0f );
            if( posToIndex.insert( { {P.x, P.y, P.z}, index }).second)
            {
                NEW_POS.push_back(p);
                if(!_NOR.empty())
                    NEW_NOR.push_back(_NOR[j]);
                if(!_UV.empty())
                    NEW_UV.push_back(_UV[j]);
                index++;
            }
            j++;
        }

        std::vector<uint32_t> newINDEX;
        for(auto i : _INDEX)
        {
            auto & p = _POS[i];
            glm::ivec3 P( p*100.0f );
            newINDEX.push_back( posToIndex[{P.x,P.y,P.z}]);
        }
        INDEX = std::move(newINDEX);
        POSITION = std::move(NEW_POS);
        NORMAL = std::move(NEW_NOR);
        TEXCOORD_0= std::move(NEW_UV);
    }

    void rebuildNormals()
    {
        if( std::holds_alternative< std::vector<uint32_t> >(INDEX))
        {
            auto & I = std::get< std::vector<uint32_t > >(INDEX);
            auto & P = std::get< std::vector<glm::vec3> >(POSITION);
            std::vector< glm::vec3 > normals(P.size(), glm::vec3(0,0,0));

            for(size_t i=0;i<I.size();i+=3)
            {
                auto v1 = P[i+1] - P[i];
                auto v2 = P[i+2] - P[i];
                auto n = glm::cross(v1,v2);

                normals[i] += n;
                normals[i+1] += n;
                normals[i+2] += n;
            }
            for(auto & n : normals)
            {
                n = glm::normalize(n);
            }
            NORMAL = std::move(normals);
        }
    }
};

inline MeshPrimitive Box(float dx , float dy , float dz )
{
    using _vec2 = glm::vec2;//std::array<float,2>;
    using _vec3 = glm::vec3;//std::array<float,3>;

    MeshPrimitive M;

    auto & P = std::get< std::vector<glm::vec3> >(M.POSITION);
    auto & N = std::get< std::vector<glm::vec3> >(M.NORMAL);
    auto & U = std::get< std::vector<glm::vec2> >(M.TEXCOORD_0);
    auto & I = std::get< std::vector<uint32_t > >(M.INDEX);


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

    using _vec3  = glm::vec3;//std::array<float,3>;
    using _uvec4 = glm::u8vec4;//std::array<uint8_t,4>;

    MeshPrimitive M;
    M.topology = Topology::LINE_LIST;
    auto & P = std::get< std::vector<glm::vec3> >(M.POSITION);
    auto & C = std::get< std::vector<glm::u8vec4> >(M.COLOR_0);

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
    using _vec2 = glm::vec2;//std::array<float,2>;
    using _vec3 = glm::vec3;//std::array<float,3>;

    MeshPrimitive M;


    auto & P = std::get< std::vector<glm::vec3> >(M.POSITION);
    auto & N = std::get< std::vector<glm::vec3> >(M.NORMAL);
    auto & U = std::get< std::vector<glm::vec2> >(M.TEXCOORD_0);
    auto & I = std::get< std::vector<uint32_t > >(M.INDEX);


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

    auto & P = std::get< std::vector<glm::vec3> >(M.POSITION);
    auto & N = std::get< std::vector<glm::vec3> >(M.NORMAL);
    auto & I = std::get< std::vector<uint32_t > >(M.INDEX);
    auto & U = std::get< std::vector<glm::vec2> >(M.TEXCOORD_0);

    P.emplace_back(-sideLength,-sideLength,0);
    P.emplace_back( sideLength,-sideLength,0);
    P.emplace_back( sideLength, sideLength,0);
    P.emplace_back(-sideLength, sideLength,0);

    U.emplace_back( 0.0f, 1.0f);
    U.emplace_back( 1.0f, 1.0f);
    U.emplace_back( 1.0f, 0.0f);
    U.emplace_back( 0.0f, 0.0f);

    N.emplace_back(0,0,1);
    N.emplace_back(0,0,1);
    N.emplace_back(0,0,1);
    N.emplace_back(0,0,1);

    I = {0,1,2,0,2,3};

    return M;
}

inline MeshPrimitive ReadOBJ(std::ifstream & in)
{
    std::vector< glm::vec3 > position;
    std::vector< glm::vec3 > normal;
    std::vector< glm::vec2 > uv;

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
            glm::vec3 p;
            ins  >> p.x;
            ins  >> p.y;
            ins  >> p.z;
            position.push_back(p);
        }
        else if(line == "vn")
        {
            glm::vec3 p;
            ins  >> p.x;
            ins  >> p.y;
            ins  >> p.z;
            normal.push_back(p);
        }
        else if(line == "vt")
        {
            glm::vec2 p;
            ins  >> p.x;
            ins  >> p.y;
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

    std::vector<glm::vec3> POSITION;
    std::vector<glm::vec2> TEXCOORD;
    std::vector<glm::vec3> NORMAL;
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
