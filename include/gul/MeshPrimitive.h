#ifndef GUL_MESH_PRIMITIVE_H
#define GUL_MESH_PRIMITIVE_H

#include <variant>
#include <vector>
#include <array>
#include <cstring>
#include <tuple>
#include <glm/glm.hpp>

namespace gul
{
using VertexAttribute_v = std::variant<
                            std::vector< glm::vec2 >,
                            std::vector< glm::vec3 >,
                            std::vector< glm::vec4 >,
                            std::vector< glm::dvec2 >,
                            std::vector< glm::dvec3 >,
                            std::vector< glm::dvec4 >,
                            std::vector< glm::u32vec2 >,
                            std::vector< glm::u32vec3 >,
                            std::vector< glm::u32vec4 >,
                            std::vector< glm::u16vec2 >,
                            std::vector< glm::u16vec3 >,
                            std::vector< glm::u16vec4 >,
                            std::vector< glm::u8vec2 >,
                            std::vector< glm::u8vec3 >,
                            std::vector< glm::u8vec4 >,
                            std::vector< glm::i32vec2 >,
                            std::vector< glm::i32vec3 >,
                            std::vector< glm::i32vec4 >,
                            std::vector< glm::i16vec2 >,
                            std::vector< glm::i16vec3 >,
                            std::vector< glm::i16vec4 >,
                            std::vector< glm::i8vec2 >,
                            std::vector< glm::i8vec3 >,
                            std::vector< glm::i8vec4 >,
                            std::vector< uint8_t >,
                            std::vector< uint16_t >,
                            std::vector< uint32_t >,
                            std::vector< int8_t >,
                            std::vector< int16_t >,
                            std::vector< int32_t >,
                            std::vector< float >,
                            std::vector< double >
                    >;

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
        offsets.push_back( off);
        off += VertexAttributeByteSize(*v);
    }

    auto dOut = static_cast<uint8_t*>(data);
    for(auto & v : V)
    {
        std::visit( [&](auto && arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    std::memcpy(dOut, arg.data(), arg.size()*sizeof(typename T::value_type));
                    dOut += arg.size()*sizeof(typename T::value_type);
                }, *v);
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
    size_t byteSize=0;
    uint64_t stride=0;

    uint8_t * out = static_cast<uint8_t*>(data);
    (void)out;

    size_t S=0;
    std::vector<size_t> offsets;
    size_t off=0;
    for(auto & v : V)
    {
        stride += VertexAttributeSizeOf(*v);
        S = std::min(S, VertexAttributeCount(*v) );
        offsets.push_back( off);
        off+=VertexAttributeSizeOf(*v);
    }

    count = std::min(count, VertexAttributeCount(*V.front()));

    size_t last = startIndex + count;
    last = std::min(last, VertexAttributeCount(*V.front()));

    for(size_t i=startIndex;i<last;i++)
    {
        VertexAttributeStrideCopy(out + offsets[i], *V[i], stride);
    }
    return byteSize;
}



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

    attribute_type INDEX      = std::vector<uint32_t>();

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

inline MeshPrimitive Grid(int length, int width, int dl=1, int dw=1, int majorL=5, int majorW=5, float lscale=1.0f, float wscale=1.0f)
{

    using _vec3  = glm::vec3;//std::array<float,3>;
    using _uvec4 = glm::u8vec4;//std::array<uint8_t,4>;

    MeshPrimitive M;

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



}

#endif
