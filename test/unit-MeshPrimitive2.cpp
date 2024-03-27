#include <catch2/catch.hpp>
#include <iostream>
#include <gul/MeshPrimitive2.h>
#include <glm/glm.hpp>
#include <glm/gtc/vec1.hpp>

SCENARIO("types")
{

    REQUIRE( gul::type_to_component<glm::vec1>() == gul::eComponentType::FLOAT);
    REQUIRE( gul::type_to_component<float    >() == gul::eComponentType::FLOAT);
    REQUIRE( gul::type_to_component<glm::mat2>() == gul::eComponentType::FLOAT);
    REQUIRE( gul::type_to_component<glm::vec4>() == gul::eComponentType::FLOAT);
    REQUIRE( gul::type_to_component<glm::mat4>() == gul::eComponentType::FLOAT);

    REQUIRE( gul::type_to_type<uint32_t>() == gul::eType::SCALAR);

    REQUIRE( gul::type_to_type<glm::vec1>() == gul::eType::SCALAR);
    REQUIRE( gul::type_to_type<glm::vec2>() == gul::eType::VEC2);
    REQUIRE( gul::type_to_type<glm::vec3>() == gul::eType::VEC3);
    REQUIRE( gul::type_to_type<glm::vec4>() == gul::eType::VEC4);

    REQUIRE( gul::type_to_type<glm::mat2>() == gul::eType::MAT2);
    REQUIRE( gul::type_to_type<glm::mat3>() == gul::eType::MAT3);
    REQUIRE( gul::type_to_type<glm::mat4>() == gul::eType::MAT4);

}

SCENARIO("Initialize attribute with vector")
{
    GIVEN("A vertex attribute with items")
    {
        gul::VertexAttribute V = std::vector<glm::uvec2>( {{1,2}, {3,4}});

        REQUIRE( V.getComponentType() == gul::eComponentType::UNSIGNED_INT);
        REQUIRE( V.getType() == gul::eType::VEC2);
        REQUIRE( V.attributeCount() == 2);
        REQUIRE( V.size() == 4); // 4 total components

        auto readBack = V.toVector<glm::uvec2>();

        REQUIRE( readBack[0][0] == 1);
        REQUIRE( readBack[0][1] == 2);
        REQUIRE( readBack[1][0] == 3);
        REQUIRE( readBack[1][1] == 4);
    }
}


SCENARIO("changing types")
{
    GIVEN("A vertex attribute with 3 items")
    {
        gul::VertexAttribute V = std::vector<glm::uvec2>( {{1,2}, {3,4}, {5,6}});

        REQUIRE( V.getComponentType() == gul::eComponentType::UNSIGNED_INT);
        REQUIRE( V.getType() == gul::eType::VEC2);
        REQUIRE( V.attributeCount() == 3);
        REQUIRE( V.size() == 6); // 4 total components

        WHEN("We change the type to a scalar")
        {
            V.setType( gul::eType::SCALAR);

            REQUIRE(V.attributeCount() == 6);
            REQUIRE(V.size() == 6);

            auto readBack = V.toVector<uint32_t>();
            REQUIRE(readBack.size() == V.attributeCount() );
            REQUIRE( readBack[0] == 1);
            REQUIRE( readBack[1] == 2);
            REQUIRE( readBack[2] == 3);
            REQUIRE( readBack[3] == 4);
            REQUIRE( readBack[4] == 5);
            REQUIRE( readBack[5] == 6);
        }
        WHEN("We change the type to a VEC3")
        {
            V.setType( gul::eType::VEC3);

            REQUIRE(V.attributeCount() == 2);
            REQUIRE(V.size() == 6);

            auto readBack = V.toVector<glm::uvec3>();
            REQUIRE(readBack.size() == V.attributeCount() );
            REQUIRE( readBack[0][0] == 1);
            REQUIRE( readBack[0][1] == 2);
            REQUIRE( readBack[0][2] == 3);
            REQUIRE( readBack[1][0] == 4);
            REQUIRE( readBack[1][1] == 5);
            REQUIRE( readBack[1][2] == 6);
        }
        WHEN("We change the type to a VEC4")
        {
            V.setType( gul::eType::VEC4);

            THEN("We have only 1 attribute, since 4 does not evenly divide 6")
            {
                // only 1 item is available
                REQUIRE(V.attributeCount() == 1);
                REQUIRE(V.size() == 4);

                auto readBack = V.toVector<glm::uvec4>();
                REQUIRE(readBack.size() == V.attributeCount() );
                REQUIRE( readBack[0][0] == 1);
                REQUIRE( readBack[0][1] == 2);
                REQUIRE( readBack[0][2] == 3);
                REQUIRE( readBack[0][3] == 4);
            }
        }
        WHEN("We change the type to a MAT2")
        {
            V.setType( gul::eType::MAT2);

            THEN("We have only 1 attribute, since 4 does not evenly divide 6")
            {
                // only 1 item is available
                REQUIRE(V.attributeCount() == 1);
                REQUIRE(V.size() == 4);

                auto readBack = V.toVector<glm::umat2x2>();
                REQUIRE(readBack.size() == V.attributeCount() );
                REQUIRE( readBack[0][0][0] == 1);
                REQUIRE( readBack[0][0][1] == 2);
                REQUIRE( readBack[0][1][0] == 3);
                REQUIRE( readBack[0][1][1] == 4);
            }
        }
    }
}


SCENARIO("strideCopyOffset")
{
    GIVEN("A vertex attribute with 3 items")
    {
        gul::VertexAttribute V = std::vector<glm::uvec2>( {{1,2}, {3,4}, {5,6}});

        REQUIRE( V.getComponentType() == gul::eComponentType::UNSIGNED_INT);
        REQUIRE( V.getType() == gul::eType::VEC2);
        REQUIRE( V.attributeCount() == 3);
        REQUIRE( V.size() == 6); // 4 total components

        auto readBack = V.toVector<glm::uvec2>();

        WHEN("We do a strideCopy with a stride size of 2*sizeof( glm::vec2)")
        {
            std::vector<uint32_t> D = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};

            V.strideCopyOffset(D.data(),
                               2*sizeof(glm::uvec2),
                               0,   // copy data to
                               1,   // start at the second index
                               10); // copy 10 attributes

            THEN("Every other value is copied")
            {
                REQUIRE( D[0] == 3);
                REQUIRE( D[1] == 4);
                REQUIRE( D[2] == 0); // skipped
                REQUIRE( D[3] == 0); // skipped
                REQUIRE( D[4] == 5);
                REQUIRE( D[5] == 6);
                REQUIRE( D[6] == 0); // not copied, overflow
                REQUIRE( D[7] == 0); // not copied, overflow
                REQUIRE( D[8] == 0); // not copied, overflow
                REQUIRE( D[9] == 0); // not copied, overflow
            }
        }
    }
}


SCENARIO("Accessing data")
{
    GIVEN("A vertex attribute with items")
    {
        gul::VertexAttribute V = std::vector<glm::uvec2>( {{1,2}, {3,4}, {5,6}});

        WHEN("When we use get()")
        {
            REQUIRE(V.get<uint32_t>(0) == 1);
            REQUIRE(V.get<uint32_t>(1) == 2);
            REQUIRE(V.get<uint32_t>(2) == 3);
            REQUIRE(V.get<uint32_t>(3) == 4);
            REQUIRE(V.get<uint32_t>(4) == 5);
            REQUIRE(V.get<uint32_t>(5) == 6);
        }
        WHEN("When we use getAttributeAs()")
        {
            REQUIRE(V.getAttributeAs<uint32_t>(0) == 1); // reads V[0][0]
            REQUIRE(V.getAttributeAs<uint32_t>(1) == 3); // reads V[1][0]
            REQUIRE(V.getAttributeAs<uint32_t>(2) == 5); // reads V[2][0]
        }
    }
}

SCENARIO("Merging")
{
    GIVEN("A vertex attribute with items")
    {
        gul::VertexAttribute V = std::vector<glm::uvec2>( {{1,2}, {3,4}, {5,6}});
        gul::VertexAttribute V2 = std::vector<glm::uvec2>( {{7,8}, {9,10}});

        auto byteOffset = V.merge(V2);

        REQUIRE( byteOffset == sizeof(glm::uvec2) * 3);
        REQUIRE( V.attributeCount() == 5);
    }
}


SCENARIO("Mesh::calculateInterleavedStride")
{
    GIVEN("A 2 vertex attribute with items")
    {
        gul::VertexAttribute V1 = std::vector<glm::uvec2>( {{1,2}, {3,4}, {5,6}});
        gul::VertexAttribute V2 = std::vector<glm::ivec2>( {{-1,-2}, {-3,-4}, {-5,-6}});

        REQUIRE(16 == gul::calculateInterleavedStride({&V1, &V2}));
    }
}

SCENARIO("Mesh::calculateInterleavedBytes")
{
    GIVEN("A 2 vertex attribute with items")
    {
        gul::VertexAttribute V1 = std::vector<glm::uvec2>( {{1,2}, {3,4}, {5,6}});
        gul::VertexAttribute V2 = std::vector<glm::ivec2>( {{-1,-2}, {-3,-4}, {-5,-6}});

        REQUIRE(48 == gul::calculateInterleavedBytes({&V1, &V2}));
    }
}

SCENARIO("Mesh::copyVertexAttributesInterleaved")
{
    GIVEN("A 2 vertex attribute with items")
    {
        gul::VertexAttribute V1 = std::vector<glm::uvec2>( {{1,2}, {3,4}, {5,6}});
        gul::VertexAttribute V2 = std::vector<glm::ivec2>( {{-1,-2}, {-3,-4}, {-5,-6}});

        struct Vertex
        {
            glm::uvec2 a;
            glm::ivec2 b;
        };

        REQUIRE( offsetof(Vertex, a) == 0);
        REQUIRE( offsetof(Vertex, b) == sizeof(glm::uvec2));

        THEN("We can copy each attribute sequentually to a vertex vector")
        {
            std::vector<Vertex> raw;
            gul::MeshPrimitive::copyVertexAttributesInterleaved(raw, {&V1,&V2});

            REQUIRE( raw.size() == 3);

            WHEN("When we use get()")
            {
                REQUIRE(raw[0].a[0] == 1);
                REQUIRE(raw[0].a[1] == 2);
                REQUIRE(raw[1].a[0] == 3);
                REQUIRE(raw[1].a[1] == 4);
                REQUIRE(raw[2].a[0] == 5);
                REQUIRE(raw[2].a[1] == 6);

                REQUIRE(raw[0].b[0] == -1);
                REQUIRE(raw[0].b[1] == -2);
                REQUIRE(raw[1].b[0] == -3);
                REQUIRE(raw[1].b[1] == -4);
                REQUIRE(raw[2].b[0] == -5);
                REQUIRE(raw[2].b[1] == -6);
            }
        }
    }
}

SCENARIO("get minmax")
{
    GIVEN("A vertex attribute with 3 items")
    {
        gul::VertexAttribute V = std::vector<glm::uvec2>( {{1,2}, {3,4}, {5,6}, {7,8} });

        WHEN("We change the type to a scalar")
        {
            V.setType( gul::eType::SCALAR);

            auto [_m, _M] = V.getMinMax<uint32_t>();
            REQUIRE( _m.size() == 1);
            REQUIRE( _M.size() == 1);
            REQUIRE( _m[0] == 1);
            REQUIRE( _M[0] == 8);
        }

        WHEN("We change the type to a vec2")
        {
            V.setType( gul::eType::VEC2);

            auto [_m, _M] = V.getMinMax<uint32_t>();
            REQUIRE( _m.size() == 2);
            REQUIRE( _M.size() == 2);
            REQUIRE( _m[0] == 1);
            REQUIRE( _m[1] == 2);
            REQUIRE( _M[0] == 7);
            REQUIRE( _M[1] == 8);
        }

        WHEN("We change the type to a vec4")
        {
            V.setType( gul::eType::VEC4);

            auto [_m, _M] = V.getMinMax<uint32_t>();
            REQUIRE( _m.size() == 4);
            REQUIRE( _M.size() == 4);
            REQUIRE( _m[0] == 1);
            REQUIRE( _m[1] == 2);
            REQUIRE( _m[2] == 3);
            REQUIRE( _m[3] == 4);
            REQUIRE( _M[0] == 5);
            REQUIRE( _M[1] == 6);
            REQUIRE( _M[2] == 7);
            REQUIRE( _M[3] == 8);
        }
        WHEN("We change the type to a mat4")
        {
            V.setType( gul::eType::MAT2);

            auto [_m, _M] = V.getMinMax<uint32_t>();
            REQUIRE( _m.size() == 4);
            REQUIRE( _M.size() == 4);
            REQUIRE( _m[0] == 1);
            REQUIRE( _m[1] == 2);
            REQUIRE( _m[2] == 3);
            REQUIRE( _m[3] == 4);
            REQUIRE( _M[0] == 5);
            REQUIRE( _M[1] == 6);
            REQUIRE( _M[2] == 7);
            REQUIRE( _M[3] == 8);
        }
    }

    GIVEN("A vertex attribute with 3 items")
    {
        gul::VertexAttribute V = std::vector<glm::vec2>( {{1,2}, {7,8}, {3,4}, {5,6} });

        WHEN("We change the type to a scalar")
        {
            auto [_m, _M] = V.getMinMax<float>();
            REQUIRE( _m[0] == Approx(1));
            REQUIRE( _m[1] == Approx(2));
            REQUIRE( _M[0] == Approx(7));
            REQUIRE( _M[1] == Approx(8));
        }
    }
}



SCENARIO("Mesh")
{
    // box mesh has position, normals, texcoords0, index
    auto S = gul::Box(1,1,1);

    constexpr uint64_t vertexCount = 36;
    constexpr uint64_t indexCount = 36;
    constexpr uint64_t vertexSize = sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2);
    constexpr uint64_t vertexDeviceSize = vertexCount * vertexSize;
    constexpr uint64_t indexDeviceSize = indexCount * sizeof(uint32_t);
    constexpr uint64_t deviceSize = indexDeviceSize + vertexDeviceSize;


    REQUIRE(S.INDEX.attributeCount() == indexCount);
    REQUIRE(S.indexCount() == indexCount);

    REQUIRE( S.POSITION.attributeCount() == vertexCount);
    REQUIRE( S.NORMAL.attributeCount() == vertexCount);
    REQUIRE( S.TEXCOORD_0.attributeCount() == vertexCount);

    REQUIRE( S.calculateDeviceSize() == deviceSize);

    REQUIRE(S.getVertexByteSize() == vertexSize);

    REQUIRE( vertexDeviceSize == S.calculateInterleavedBufferSize() );

    THEN("We can copy the mesh vertices in interleaved format")
    {
        std::vector<uint8_t> raw;
        raw.resize(vertexDeviceSize);
        auto totalCopied = S.copyVertexAttributesInterleaved(raw.data(), 0);
        REQUIRE( totalCopied == vertexCount);
    }

    THEN("We can copy the mesh vertices in sequential format")
    {
        std::vector<uint8_t> raw;

        raw.resize( S.calculateDeviceSize() ); // need device size

        auto offsets = S.copyVertexAttributesSquential(raw.data());

        REQUIRE( offsets.size() == 9);
        REQUIRE( offsets[0] == 0); // positions at 0
        REQUIRE( offsets[1] == 36*12); // normal after position
        REQUIRE( offsets[2] == 0);
        REQUIRE( offsets[3] == 36*(12+12)); // texcoord after position/normal
        REQUIRE( offsets[4] == 0);
        REQUIRE( offsets[5] == 0);
        REQUIRE( offsets[6] == 0);
        REQUIRE( offsets[7] == 0);
        REQUIRE( offsets[8] == 36*(12+12+8)); // indices after position/normal/texcoord
    }
}


SCENARIO("Mesh Merge")
{
    // box mesh has position, normals, texcoords0, index
    auto FirstMesh = gul::Box(1,1,1);
    auto S = gul::Sphere(1.0);

    // only a single primitive in the box mesh
    REQUIRE( FirstMesh.primitives.size() == 1 );

    REQUIRE(FirstMesh.primitives[0].indexCount == 36);
    REQUIRE(FirstMesh.primitives[0].vertexCount == 36);
    REQUIRE(FirstMesh.primitives[0].indexOffset == 0);
    REQUIRE(FirstMesh.primitives[0].vertexOffset == 0);

    WHEN("We merge the sphere into the box with renumbering the indices")
    {
        FirstMesh.merge(S, true);

        THEN("There are two primitives")
        {
            REQUIRE(FirstMesh.primitives.size() == 2);

            THEN("The index offset will be the number of indices in the first mesh. and the vertex offset is zero")
            {
                REQUIRE( static_cast<uint32_t>(FirstMesh.primitives[1].indexOffset) == FirstMesh.primitives[0].indexCount );
                REQUIRE( FirstMesh.primitives[1].vertexOffset == 0 );
            }

            THEN("Each index in the merged mesh is offset")
            {
                uint32_t indexOffset = FirstMesh.primitives[0].indexCount;

                for(uint32_t i=0; i < FirstMesh.primitives[1].indexCount; i++)
                {
                    uint32_t j = i + static_cast<uint32_t>(FirstMesh.primitives[1].indexOffset);

                    REQUIRE(FirstMesh.INDEX.get<uint32_t>(j) - indexOffset == S.INDEX.get<uint32_t>(i) );
                }
            }
        }
    }
    WHEN("We merge the sphere into the box without renumbering the indices")
    {
        FirstMesh.merge(S, false);

        THEN("There are two primitives")
        {
            REQUIRE(FirstMesh.primitives.size() == 2);
        }
        THEN("The index offset will be the number of indices in the first mesh. and the vertex offset is the number of vertices in the first mesh")
        {
            REQUIRE( static_cast<uint32_t>(FirstMesh.primitives[1].indexOffset) == FirstMesh.primitives[0].indexCount );
            REQUIRE( static_cast<uint32_t>(FirstMesh.primitives[1].vertexOffset) == FirstMesh.primitives[0].vertexCount );
        }
        THEN("Each index in the merged mesh is NOT offset")
        {
            for(uint32_t i=0; i < FirstMesh.primitives[1].indexCount; i++)
            {
                uint32_t j = i + static_cast<uint32_t>(FirstMesh.primitives[1].indexOffset);

                REQUIRE(S.INDEX.get<uint32_t>(i) == FirstMesh.INDEX.get<uint32_t>(j) );
            }
        }
    }
}


SCENARIO("Bounding Sphere")
{
    // box mesh has position, normals, texcoords0, index
    auto B = gul::Box(1,1,1);
    auto S = gul::Sphere(1.0);


    auto br = B.calculateBoundingSphereRadius();
    auto sr = S.calculateBoundingSphereRadius();

    REQUIRE( sr > 0.99f );
    REQUIRE( sr < 1.09f );

    REQUIRE( br > 0.865f );
    REQUIRE( br < 0.867f ); // sqrt(1+1+1)

    {
        auto sr1 = S.calculateBoundingSphereRadius(S.primitives[0]);
        REQUIRE( sr1 > 0.99f );
        REQUIRE( sr1 < 1.09f );
    }
    {
        B.merge(S);
        auto sr1 = B.calculateBoundingSphereRadius(B.primitives[1]);
        REQUIRE( sr1 > 0.99f );
        REQUIRE( sr1 < 1.09f );
    }
}


SCENARIO("Test Base Primitives")
{
    // box mesh has position, normals, texcoords0, index
    auto B = gul::Box(1,1,1);
    auto S = gul::Sphere(1.0);
    auto C = gul::Cylinder();


    REQUIRE( B.vertexCount() > 0);
    REQUIRE( S.vertexCount() > 0);
    REQUIRE( C.vertexCount() > 0);

}


SCENARIO("Load OBJ")
{
    std::istringstream SSO;
    SSO.str(R"foo(# Blender v3.0.1 OBJ File: ''
# www.blender.org
mtllib untitled.mtl
o Cube
v 1.000000 1.000000 -1.000000
v 1.000000 -1.000000 -1.000000
v 1.000000 1.000000 1.000000
v 1.000000 -1.000000 1.000000
v -1.000000 1.000000 -1.000000
v -1.000000 -1.000000 -1.000000
v -1.000000 1.000000 1.000000
v -1.000000 -1.000000 1.000000
vt 0.625000 0.500000
vt 0.875000 0.500000
vt 0.875000 0.750000
vt 0.625000 0.750000
vt 0.375000 0.750000
vt 0.625000 1.000000
vt 0.375000 1.000000
vt 0.375000 0.000000
vt 0.625000 0.000000
vt 0.625000 0.250000
vt 0.375000 0.250000
vt 0.125000 0.500000
vt 0.375000 0.500000
vt 0.125000 0.750000
vn 0.0000 1.0000 0.0000
vn 0.0000 0.0000 1.0000
vn -1.0000 0.0000 0.0000
vn 0.0000 -1.0000 0.0000
vn 1.0000 0.0000 0.0000
vn 0.0000 0.0000 -1.0000
usemtl Material
s off
f 1/1/1 5/2/1 7/3/1 3/4/1
f 4/5/2 3/4/2 7/6/2 8/7/2
f 8/8/3 7/9/3 5/10/3 6/11/3
f 6/12/4 2/13/4 4/5/4 8/14/4
f 2/13/5 1/1/5 3/4/5 4/5/5
f 6/11/6 5/10/6 1/1/6 2/13/6
)foo");

    using vec3      = std::array<float, 3>;
    using vec2      = std::array<float, 2>;
    using tri_face  = std::array<uint32_t, 3>;
    using quad_face = std::array<uint32_t, 4>;

    std::vector<vec3> pos, norm;
    std::vector<vec2> uv;
    std::vector<tri_face> tris;
    std::vector<quad_face> quads;

    using vertex_id = std::tuple<uint32_t, uint32_t, uint32_t>;
    std::map<vertex_id, uint32_t> vertex_to_index;

    std::string blah;
    std::string line;

    while(!SSO.eof())
    {
        std::getline(SSO, line);
        if(line.empty())
            continue;
       // std::cout << line << std::endl;

        if(line[1] == 'n')
        {
            std::istringstream ss(line);
            std::string v;
            vec3 & p = norm.emplace_back();
            ss >> v;
            ss >> p[0];
            ss >> p[1];
            ss >> p[2];
            std::cout << p[0] << ", " << p[1] << ", " << p[2] << std::endl;
        }
        else if(line[1] == 't')
        {
            std::istringstream ss(line);
            std::string v;
            vec2 & p = uv.emplace_back();
            ss >> v;
            ss >> p[0];
            ss >> p[1];
            std::cout << p[0] << ", " << p[1] << std::endl;
        }
        else if(line[0] == 'v')
        {
            std::istringstream ss(line);
            std::string v;
            vec3 & p = pos.emplace_back();
            ss >> v;
            ss >> p[0];
            ss >> p[1];
            ss >> p[2];
            std::cout << p[0] << ", " << p[1] << ", " << p[2] << std::endl;
        }
        else if(line[0] == 'f')
        {
            // f 6/11/6 5/10/6 1/1/6 2/13/6
            // f 6/11/6 5/10/6 1/1/6
            // f 6//6 5//6 1//6
            // f 6 5 1
            std::istringstream ss(line);
            std::string _b;
            ss >> _b; // read the 'f'

            auto _extractVertexID = [](std::string str)
            {
                std::istringstream s(str);
                vertex_id v = {};
                // can either be a/b/c
                //               a//c
                //               a
                s >> std::get<0>(v);
                if(s.eof())
                    return v;


                if(s.peek() == '/')
                {
                    s.get();
                }
                if(s.peek() == '/')
                {
                    s.get();
                    s >> std::get<2>(v);
                }
                else
                {
                    s >> std::get<1>(v);
                }

                if(s.peek() == '/')
                {
                    s.get();
                    s >> std::get<2>(v);
                }
                return v;
            };

            std::array<vertex_id, 4> _faceIndex;
            uint32_t j=0;
            while(!ss.eof())
            {
                std::string vertex_id_str;
                ss >> vertex_id_str;

                if(!vertex_id_str.empty())
                {
                    auto & V = _faceIndex[j++] = _extractVertexID(vertex_id_str);
                    std::cout << std::get<0>(V) << "  " <<  std::get<1>(V) << "  " << std::get<2>(V) << std::endl;
                }
            }

            // we now have the verttex'x position index, normal index and uv index
            // in the form of a 3-tuple
            // Insert the tuple into the ma
            if(j==3) // triangle
            {
                tri_face t = { vertex_to_index.insert( {_faceIndex[0], static_cast<uint32_t>(vertex_to_index.size())}).first->second,
                               vertex_to_index.insert( {_faceIndex[1], static_cast<uint32_t>(vertex_to_index.size())}).first->second,
                               vertex_to_index.insert( {_faceIndex[2], static_cast<uint32_t>(vertex_to_index.size())}).first->second};

                tris.push_back(t);
            }
            if(j==4) // triangle
            {
               quad_face t = { vertex_to_index.insert( {_faceIndex[0], static_cast<uint32_t>(vertex_to_index.size())}).first->second,
                               vertex_to_index.insert( {_faceIndex[1], static_cast<uint32_t>(vertex_to_index.size())}).first->second,
                               vertex_to_index.insert( {_faceIndex[2], static_cast<uint32_t>(vertex_to_index.size())}).first->second,
                               vertex_to_index.insert( {_faceIndex[3], static_cast<uint32_t>(vertex_to_index.size())}).first->second};

                quads.push_back(t);
            }
            // end
        }
    }

    for(auto & t : quads)
    {
        //std::cout << t[0] << ", " << t[1] << ", " << t[2] << ", " << t[3] <<  std::endl;
        tris.push_back( {t[0], t[1], t[2]});
        tris.push_back( {t[0], t[2], t[3]});
    }
    std::cout << "Total Unique Vertices: " << vertex_to_index.size() << std::endl;
    for(auto & t : tris)
    {
        std::cout << t[0] << ", " << t[1] << ", " << t[2] << std::endl;
    }

    pos.resize(vertex_to_index.size());
    norm.resize(vertex_to_index.size());
    uv.resize(vertex_to_index.size());

    gul::MeshPrimitive P;

    P.TEXCOORD_0.setType(gul::eType::VEC2);
    P.TEXCOORD_0.setComponent(gul::eComponentType::FLOAT);

    P.POSITION.setType(gul::eType::VEC3);
    P.POSITION.setComponent(gul::eComponentType::FLOAT);

    P.NORMAL.setType(gul::eType::VEC3);
    P.NORMAL.setComponent(gul::eComponentType::FLOAT);

    P.POSITION.resize(vertex_to_index.size());
    P.TEXCOORD_0.resize(vertex_to_index.size());
    P.NORMAL.resize(vertex_to_index.size());

    for(auto & [v, index] : vertex_to_index)
    {
        if(std::get<0>(v) != 0) P.POSITION.set(index  , pos.at( std::get<0>(v)-1) );
        if(std::get<1>(v) != 0) P.NORMAL.set(index    , norm.at(std::get<1>(v)-1) );
        if(std::get<2>(v) != 0) P.TEXCOORD_0.set(index, uv.at(  std::get<2>(v)-1) );
    }
    if(P.NORMAL.attributeCount() != P.POSITION.attributeCount())
        P.NORMAL = {};
    if(P.TEXCOORD_0.attributeCount() != P.POSITION.attributeCount())
        P.TEXCOORD_0 = {};
    //for(auto & t : tris)
    //{
    //    P.INDEX.push_back(t[0]);
    //    P.INDEX.push_back(t[1]);
    //    P.INDEX.push_back(t[2]);
    //}
}


