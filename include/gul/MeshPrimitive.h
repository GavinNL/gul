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
static constexpr uint32_t getGLNumComponents()
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

struct MeshPrimitive
{
    using attribute_type = VertexAttribute_v;

    attribute_type POSITION   = std::vector<glm::vec3>  ();
    attribute_type NORMAL     = std::vector<glm::vec3>  ();
    attribute_type TANGENT    = std::vector<glm::vec3>  ();
    attribute_type TEXCOORD_0 = std::vector<glm::vec2>  ();
    attribute_type TEXCOORD_1 = std::vector<glm::vec3>  ();
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

        size += byteSize(POSITION  );
        size += byteSize(NORMAL    );
        size += byteSize(TANGENT   );
        size += byteSize(TEXCOORD_0);
        size += byteSize(TEXCOORD_1);
        size += byteSize(COLOR_0   );
        size += byteSize(JOINTS_0  );
        size += byteSize(WEIGHTS_0 );
        size += byteSize(INDEX);

        return size;
    }

    static uint64_t byteSize(attribute_type const & v)
    {
        return std::visit( [&](auto && arg)
        {
            using V = std::decay_t<decltype(arg)>; //std::vector<attr_type>
            using attr_type = typename V::value_type;
            return arg.size() * sizeof(attr_type);
        }, v);
    }
    static uint64_t attributeSize(attribute_type const & v)
    {
        return std::visit( [&](auto && arg)
        {
            using V = std::decay_t<decltype(arg)>; //std::vector<attr_type>
            using attr_type = typename V::value_type;
            return sizeof(attr_type);
        }, v);
    }
    static size_t attributeCount(attribute_type const & v)
    {
        return std::visit( [&](auto && arg)
        {
            return arg.size();
        }, v);
    }

    static std::vector<size_t> copySequential(void * data, std::vector<VertexAttribute_v const*> const & V)
    {
        std::vector<size_t> offsets;
        size_t off=0;
        for(auto & v : V)
        {
            offsets.push_back( off);
            off += attributeSize(*v)*attributeCount(*v);
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

    static void copyInterleaved(void * data, std::vector<VertexAttribute_v const*> const & V, size_t startIndex=0, size_t count=std::numeric_limits<size_t>::max())
    {
        (void)data;
        (void)V;
        uint64_t stride=0;

        uint8_t * out = static_cast<uint8_t*>(data);
        (void)out;

        size_t S=0;
        std::vector<size_t> offsets;
        size_t off=0;
        for(auto & v : V)
        {
            stride += attributeSize(*v);
            S = std::min(S, attributeCount(*v) );
            offsets.push_back( off);
            off+=attributeSize(*v);
        }

        count = std::min(count, attributeCount(*V.front()));

        size_t last = startIndex + count;
        last = std::min(last, attributeCount(*V.front()));

        for(size_t i=startIndex;i<last;i++)
        {
            strideCopy(out + offsets[i], *V[i], stride);
        }
    }

    static void strideCopy(void * start, VertexAttribute_v const &v, size_t stride)
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

    std::array<uint64_t,9> copyData(void * data) const
    {
        std::array<uint64_t,9> outputOffsets = {0};

        uint64_t offset = 0;
        auto _cpy = [&](auto const &D) -> uint64_t
        {
            return std::visit([&](auto && arg) -> uint64_t
            {
                using T = std::decay_t<decltype(arg)>;
                auto d = static_cast<uint8_t*>(data) + offset;
                auto bytesize = arg.size() * sizeof( typename T::value_type);

                if( bytesize > 0)
                {
                    std::memcpy( d, arg.data() , bytesize );

                    auto currentOffset = offset;
                    offset += bytesize;
                    return currentOffset;
                }
                return 0u;
            }, D);

            return 0u;
        };

        outputOffsets[0] = _cpy(POSITION);
        outputOffsets[1] = _cpy(NORMAL);
        outputOffsets[2] = _cpy(TANGENT);
        outputOffsets[3] = _cpy(TEXCOORD_0);
        outputOffsets[4] = _cpy(TEXCOORD_1);
        outputOffsets[5] = _cpy(COLOR_0);
        outputOffsets[6] = _cpy(JOINTS_0);
        outputOffsets[7] = _cpy(WEIGHTS_0);
        outputOffsets[8] = _cpy(INDEX);

        return outputOffsets;
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
