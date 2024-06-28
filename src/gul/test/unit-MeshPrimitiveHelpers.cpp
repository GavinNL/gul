#include <catch2/catch_all.hpp>
#include <iostream>
#include <gul/MeshPrimitiveFunctions.h>

SCENARIO("Vertex Attribute Reshaping")
{
    GIVEN("Vertex attribute of scalar floats")
    {
        gul::VertexAttribute V(gul::eComponentType::FLOAT, gul::eType::SCALAR);
        std::vector<float> raw = {0.5f,1.0f,0.8f,0.25f,0.75f,0.6f};
        V = raw;

        REQUIRE( V.attributeCount() == 6 );
        REQUIRE( V.getShape()[0] == 6);
        REQUIRE( V.getShape()[1] == 1);

        WHEN("We set the type to VEC2")
        {
            V.setType(gul::eType::VEC2);

            THEN("The size and shapes are different")
            {
                REQUIRE( V.attributeCount() == 3 );
                REQUIRE( V.getShape()[0] == 3);
                REQUIRE( V.getShape()[1] == 2);

                WHEN("We set the type back to SCALAR")
                {
                    V.setType(gul::eType::SCALAR);

                    THEN("The size and shapes are different")
                    {
                        REQUIRE( V.attributeCount() == 6 );
                        REQUIRE( V.getShape()[0] == 6);
                        REQUIRE( V.getShape()[1] == 1);
                    }
                }
            }
        }

        WHEN("We set the type to VEC3")
        {
            V.setType(gul::eType::VEC3);

            THEN("The size and shapes are different")
            {
                REQUIRE( V.attributeCount() == 2 );
                REQUIRE( V.getShape()[0] == 2);
                REQUIRE( V.getShape()[1] == 3);

                WHEN("We set the type back to SCALAR")
                {
                    V.setType(gul::eType::SCALAR);

                    THEN("The size and shapes are different")
                    {
                        REQUIRE( V.attributeCount() == 6 );
                        REQUIRE( V.getShape()[0] == 6);
                        REQUIRE( V.getShape()[1] == 1);
                    }
                }
            }
        }

        WHEN("We set the type to VEC4")
        {
            V.setType(gul::eType::VEC4);

            THEN("Then we lose information because there are not enough elements")
            {
                REQUIRE( V.attributeCount() == 1 );
                REQUIRE( V.getShape()[0] == 1);
                REQUIRE( V.getShape()[1] == 4);

                WHEN("We set the type back to SCALAR")
                {
                    V.setType(gul::eType::SCALAR);

                    THEN("The size has returned to normal")
                    {
                        REQUIRE( V.attributeCount() == 6 );
                        REQUIRE( V.getShape()[0] == 6);
                        REQUIRE( V.getShape()[1] == 1);
                    }
                }
            }
        }
    }
}


uint32_t vec2toUint(glm::uvec2 const &v)
{
    return ((v[0] << 16) & 0xFFFF0000) | (v[1] & 0x0000FFFF);
}

glm::uvec2 Uinttovec2(uint32_t v)
{
    return glm::uvec2( v>>16 , v&0x0000FFFF );
}

glm::uvec3 vec4tovec3(glm::uvec4 const &v)
{
    return glm::uvec3( v[0], v[1], vec2toUint( glm::vec2(v[2], v[3])));
}
glm::uvec4 vec3tovec4(glm::uvec3 const &v)
{
    return glm::uvec4( v[0], v[1], Uinttovec2(v[2]) ) ;
}

SCENARIO("convertAttribe_t")
{
    GIVEN("Vertex attribute of floats")
    {
        gul::VertexAttribute V(gul::eComponentType::UNSIGNED_INT, gul::eType::SCALAR);
        std::vector<uint32_t> raw = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        V = raw;
        V.setType(gul::eType::VEC2);

        REQUIRE( V.attributeCount() == 3);

        WHEN("We compress it into uint16")
        {
            gul::convertAttribute_t<glm::uvec2, uint32_t>(V, vec2toUint);
            //gul::packUnorm2x16(V);

            THEN("The type changed")
            {
                REQUIRE(V.getType() == gul::eType::SCALAR);
                REQUIRE(V.getComponentType() == gul::eComponentType::UNSIGNED_INT);
            }
            THEN("The shape has changed")
            {
                REQUIRE(V.getShape()[0] == 3);
                REQUIRE(V.getShape()[1] == 1);
            }
            THEN("The total byte size is half its original length")
            {
                REQUIRE(V.getByteSize()      == 12);
                REQUIRE(V.getAttributeSize() == 4);
                REQUIRE(V.getNumComponents() == 1);
            }
            THEN("We can get each of the values uint16")
            {
                REQUIRE(V.get<uint32_t>(0) == 0x00010002 );
                REQUIRE(V.get<uint32_t>(1) == 0x00030004 );
                REQUIRE(V.get<uint32_t>(2) == 0x00050006 );
            }
            WHEN("We uncompress")
            {
                gul::convertAttribute_t<uint32_t, glm::uvec2>(V, Uinttovec2);

                THEN("The type changed")
                {
                    REQUIRE(V.getType() == gul::eType::VEC2);
                    REQUIRE(V.getComponentType() == gul::eComponentType::UNSIGNED_INT);
                }
                THEN("The shape has reverted")
                {
                    REQUIRE(V.getShape()[0] == 3);
                    REQUIRE(V.getShape()[1] == 2);
                }
                THEN("The values are Catch::Approximately back to their original")
                {
                    REQUIRE(V.get<uint32_t>(0) == 0x01);
                    REQUIRE(V.get<uint32_t>(1) == 0x02);
                    REQUIRE(V.get<uint32_t>(2) == 0x03);
                    REQUIRE(V.get<uint32_t>(3) == 0x04);
                    REQUIRE(V.get<uint32_t>(4) == 0x05);
                    REQUIRE(V.get<uint32_t>(5) == 0x06);
                }
            }

        }
    }
}


SCENARIO("convertAttribe_t vec4 to vec3")
{
    GIVEN("Vertex attribute of floats")
    {
        gul::VertexAttribute V(gul::eComponentType::UNSIGNED_INT, gul::eType::SCALAR);
        std::vector<uint32_t> raw = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        V = raw;
        V.setType(gul::eType::VEC4);

        REQUIRE( V.attributeCount() == 2);
        REQUIRE( V.getType() == gul::eType::VEC4);

        REQUIRE(V.getByteSize()      == 32);
        REQUIRE(V.getAttributeSize() == 16);
        REQUIRE(V.getNumComponents() == 4);

        WHEN("We compress it into uint16")
        {
            gul::convertAttribute_t<glm::uvec4, glm::uvec3>(V, vec4tovec3);

            THEN("The type changed")
            {
                REQUIRE(V.getType() == gul::eType::VEC3);
                REQUIRE(V.getComponentType() == gul::eComponentType::UNSIGNED_INT);
            }

            THEN("The shape has changed")
            {
                REQUIRE(V.getShape()[0] == 2);
                REQUIRE(V.getShape()[1] == 3);
            }

            THEN("The total byte size is half its original length")
            {
                REQUIRE(V.getByteSize()      == 24);
                REQUIRE(V.getAttributeSize() == 12);
                REQUIRE(V.getNumComponents() == 3);
            }

            THEN("We can get each of the values uint16")
            {
                REQUIRE(V.get<uint32_t>(0) == 0x01 );
                REQUIRE(V.get<uint32_t>(1) == 0x02 );
                REQUIRE(V.get<uint32_t>(2) == 0x00030004 );

                REQUIRE(V.get<uint32_t>(3) == 0x05 );
                REQUIRE(V.get<uint32_t>(4) == 0x06 );
                REQUIRE(V.get<uint32_t>(5) == 0x00070008 );
            }
            WHEN("We uncompress")
            {
                gul::convertAttribute_t<glm::uvec3, glm::uvec4>(V, vec3tovec4);

                REQUIRE( V.attributeCount() == 2);
                THEN("The type changed")
                {
                    REQUIRE(V.getType() == gul::eType::VEC4);
                    REQUIRE(V.getComponentType() == gul::eComponentType::UNSIGNED_INT);
                }
                THEN("The shape has reverted")
                {
                    REQUIRE(V.getShape()[0] == 2);
                    REQUIRE(V.getShape()[1] == 4);
                }
                THEN("The values are Catch::Approximately back to their original")
                {
                    REQUIRE(V.get<uint32_t>(0) == 0x01);
                    REQUIRE(V.get<uint32_t>(1) == 0x02);
                    REQUIRE(V.get<uint32_t>(2) == 0x03);
                    REQUIRE(V.get<uint32_t>(3) == 0x04);
                    REQUIRE(V.get<uint32_t>(4) == 0x05);
                    REQUIRE(V.get<uint32_t>(5) == 0x06);
                    REQUIRE(V.get<uint32_t>(6) == 0x07);
                    REQUIRE(V.get<uint32_t>(7) == 0x08);
                }
            }
        }
    }
}



SCENARIO("pack 2xfloats into 1 u32")
{
    GIVEN("Vertex attribute of floats")
    {
        gul::VertexAttribute V(gul::eComponentType::FLOAT, gul::eType::SCALAR);
        std::vector<float> raw = {0.5f,1.0f,0.8f,0.25f,0.75f,0.f};
        V = raw;
        V.setType(gul::eType::VEC2);

        REQUIRE( V.attributeCount() == 3);

        WHEN("We compress it into uint16")
        {
            gul::convertAttribute_t<glm::vec2, uint32_t>(V, glm::packUnorm2x16);

            THEN("The type changed")
            {
                REQUIRE(V.getType() == gul::eType::SCALAR);
                REQUIRE(V.getComponentType() == gul::eComponentType::UNSIGNED_INT);
            }
            THEN("The shape has changed")
            {
                REQUIRE(V.getShape()[0] == 3);
                REQUIRE(V.getShape()[1] == 1);
            }
            THEN("The total byte size is half its original length")
            {
                REQUIRE(V.getByteSize()      == 12);
                REQUIRE(V.getAttributeSize() == 4);
                REQUIRE(V.getNumComponents() == 1);
            }
            THEN("We can get each of the values uint16")
            {
                REQUIRE(V.get<uint16_t>(0) == 32768 );
                REQUIRE(V.get<uint16_t>(1) == 65535 );
                REQUIRE(V.get<uint16_t>(2) == 52428 );
                REQUIRE(V.get<uint16_t>(3) == 16384 );
                REQUIRE(V.get<uint16_t>(4) == 49151 );
                REQUIRE(V.get<uint16_t>(5) == 0 );
            }
            WHEN("We uncompress")
            {
                gul::convertAttribute_t<uint32_t, glm::vec2>(V, glm::unpackUnorm2x16);
                V.setType(gul::eType::VEC2);

                THEN("The type changed")
                {
                    REQUIRE(V.getType() == gul::eType::VEC2);
                    REQUIRE(V.getComponentType() == gul::eComponentType::FLOAT);
                }
                THEN("The shape has reverted")
                {
                    REQUIRE(V.getShape()[0] == 3);
                    REQUIRE(V.getShape()[1] == 2);
                }
                THEN("The values are Catch::Approximately back to their original")
                {
                    REQUIRE(V.get<float>(0) == Catch::Approx(0.5) .epsilon(0.01));
                    REQUIRE(V.get<float>(1) == Catch::Approx(1.0) .epsilon(0.01));
                    REQUIRE(V.get<float>(2) == Catch::Approx(0.8) .epsilon(0.01));
                    REQUIRE(V.get<float>(3) == Catch::Approx(0.25).epsilon(0.01));
                    REQUIRE(V.get<float>(4) == Catch::Approx(0.75).epsilon(0.01));
                    REQUIRE(V.get<float>(5) == Catch::Approx(0.0) .epsilon(0.01));
                }
            }
        }
    }
}


SCENARIO("pack 3xfloats into 1 u32")
{
    GIVEN("Vertex attribute of floats")
    {
        gul::VertexAttribute V(gul::eComponentType::FLOAT, gul::eType::SCALAR);
        std::vector<float> raw = {-1.f,-1.f,-1.f,0.f,0.f,0.f,1.f,1.f,1.f,0.5f,0.5f,0.5f,-0.5f,-0.5f,-0.5f};
        V = raw;
        V.setType(gul::eType::VEC3);

        REQUIRE( V.attributeCount() == 5);

        WHEN("We compress it into uint16")
        {
            gul::convertAttribute_t<glm::vec3, uint32_t>(V, gul::packSnorm3x10);
            //gul::unpackUnorm2x16(V);
            //gul::packSnorm3x10(V);

            THEN("The type changed")
            {
                REQUIRE(V.getType() == gul::eType::SCALAR);
                REQUIRE(V.getComponentType() == gul::eComponentType::UNSIGNED_INT);
            }
            THEN("The shape has changed")
            {
                REQUIRE(V.getShape()[0] == 5);
                REQUIRE(V.getShape()[1] == 1);
            }
            THEN("The total byte size is half its original length")
            {
                REQUIRE(V.getByteSize()      == 5*4);
                REQUIRE(V.getAttributeSize() == 4);
                REQUIRE(V.getNumComponents() == 1);
            }
            WHEN("We uncompress")
            {
                gul::convertAttribute_t<uint32_t, glm::vec3>(V, gul::unpackSnorm3x10);
                V.setType(gul::eType::VEC3);
                //gul::unpackSnorm3x10(V);

                THEN("The type changed")
                {
                    REQUIRE(V.getType() == gul::eType::VEC3);
                    REQUIRE(V.getComponentType() == gul::eComponentType::FLOAT);
                }
                THEN("The shape has reverted")
                {
                    REQUIRE(V.getShape()[0] == 5);
                    REQUIRE(V.getShape()[1] == 3);
                }
                THEN("The values are Catch::Approximately back to their original")
                {
                    REQUIRE(V.get<float>(0) == Catch::Approx(-1.0f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(1) == Catch::Approx(-1.0f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(2) == Catch::Approx(-1.0f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(3) == Catch::Approx(-0.00f) .epsilon(0.03f));
                    REQUIRE(V.get<float>(4) == Catch::Approx(-0.00f) .epsilon(0.03f));
                    REQUIRE(V.get<float>(5) == Catch::Approx(-0.00f) .epsilon(0.03f));
                    REQUIRE(V.get<float>(6) == Catch::Approx( 1.0f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(7) == Catch::Approx( 1.0f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(8) == Catch::Approx( 1.0f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(9) == Catch::Approx(  0.5f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(10) == Catch::Approx( 0.5f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(11) == Catch::Approx( 0.5f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(12) == Catch::Approx(-0.5f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(13) == Catch::Approx(-0.5f) .epsilon(0.01f));
                    REQUIRE(V.get<float>(14) == Catch::Approx(-0.5f) .epsilon(0.01f));
                }
            }
        }
    }
}


SCENARIO("Pack Mesh")
{
    GIVEN("A mesh of uncompressed data")
    {
        auto S = gul::Sphere(10, 60,60);

        auto precompressed_size = S.calculateDeviceSize();

        WHEN("We pack the mesh")
        {
            gul::packMesh(S);

            auto compressed_size = S.calculateDeviceSize();
            std::cout << "Raw : " << precompressed_size << std::endl;
            std::cout << "Comp: " << compressed_size << std::endl;

            THEN("the mesh takes up less space")
            {
                REQUIRE(compressed_size < precompressed_size);
            }

            THEN("We can copy attributes to separate interleaved buffers")
            {
                auto attrCount = S.vertexCount();
                REQUIRE( attrCount > 0);

                std::vector<uint8_t> data(compressed_size);
                REQUIRE(attrCount == S.copyVertexAttributesInterleaved(data.data(), {&S.POSITION, &S.NORMAL} ) );
            }
        }
    }
}


SCENARIO("Mesh Dump")
{
    auto S = gul::Sphere(10, 60,60);

    GIVEN("A mesh of uncompressed data")
    {
        std::ofstream out("dump.mesh",std::ios_base::binary);
        S.dump(out);
    }
}

