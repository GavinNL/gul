#ifndef GUL_GLTF_LOADER_H
#define GUL_GLTF_LOADER_H

#include "MeshPrimitive2.h"
#include "Image.h"
#include "math/Transform.h"

#include <fstream>
#include <vector>

#include <nlohmann/json.hpp>
#include "ImageLoad_stb.h"
//#include "spdlog/spdlog.h"

namespace gul
{

template<typename T>
struct typed_id
{
    uint32_t index=0xFFFFFFFF;
    operator bool() const
    {
        return index != 0xFFFFFFFF;
    }
};


struct Sampler
{
    int magFilter = 9729;
    int minFilter = 9729;
    int wrapS     = 10497;
    int wrapT     = 10497;
};

struct KHR_texture_basisu_t
{
    typed_id<Image> source;
};

struct Texture
{
    typed_id<Sampler> sampler;
    typed_id<Image>   source;

    std::optional<KHR_texture_basisu_t> KHR_texture_basisu;
};

struct Material
{
    glm::vec3 emissiveFactor = glm::vec3(0,0,0);

    struct
    {
        glm::vec4 baseColorFactor = glm::vec4(1,1,1,1);

        float metallicFactor  = 1.0f;
        float roughnessFactor = 1.0f;

        struct
        {
            typed_id<Texture> index;
            int texCoord = 0;
        } baseColorTexture, metallicRoughnessTexture;

    } pbrMetallicRoughness;

    struct
    {
        typed_id<Texture> index;
        int texCoord = 0;
        float scale = 1.0f;
    } normalTexture;

    struct
    {
        typed_id<Texture> index;
        int texCoord = 0;
        float strength = 1.0f;
    } occlusionTexture;

    struct
    {
        typed_id<Texture> index;
        int texCoord = 0;
    } emissiveTexture;

    std::string alphaMode = "OPAQUE";
    float alphaCutOff = 0.5f;
    bool doubleSided = false;

};

struct Primitive
{
    // the regular GLTF asset has the primitives
    // explicitaly defined in here. Instead
    // we give an index to look up the primitive
    typed_id<MeshPrimitive> primitive;
    typed_id<Material>      material;
};

struct Mesh
{
    std::vector<Primitive> primitive;
};

struct Node : gul::Transform
{
    std::string                 name;
    std::vector<uint32_t>       children;
    typed_id<Mesh>              mesh;
};

struct Image_ref
{
    std::string mimeType;
    std::string name;
    std::string uri;

    gul::Image img;
};

struct GLTFAsset
{
    Image& get(typed_id<Image_ref> id)
    {
        return images.at(id.index).img;
    }
    Mesh& get(typed_id<Mesh> id)
    {
        return meshes.at(id.index);
    }

    std::vector<Node>          nodes;
    std::vector<Mesh>          meshes;
    //std::vector<Material>      materials;
    //std::vector<Texture>       textures;
    //std::vector<Sampler>       samplers;
    // low level objects
    std::vector<MeshPrimitive> primitives;
    std::vector<Image_ref>         images;

    // low

#define _DEF_VECTOR(type, name) \
    std::vector<type> name;\
    type & get(typed_id<type> id)\
    {\
        return name.at(id.index);\
    }

    _DEF_VECTOR(Material, materials);
    _DEF_VECTOR(Texture, textures);
    _DEF_VECTOR(Sampler, samplers);
};

GLTFAsset loadGLTF(std::istream & in, std::string const & rootPath, bool loadAllImages = true)
{
    using namespace nlohmann;

    std::map<uint32_t, std::vector<char>> buffers;
    std::vector<VertexAttribute>          accessors;
    std::unordered_map<json, uint32_t>    meshPrimitives;

    GLTFAsset G;

    json J;
    auto first = in.peek();
    if( first == 0x67 ) // GLTF magic number: 0x46546C67
    {
        // this is a GLB
        uint32_t magic;
        uint32_t version;
        uint32_t length;

        in.read( reinterpret_cast<char*>(&magic), 4);
        in.read( reinterpret_cast<char*>(&version), 4);
        in.read( reinterpret_cast<char*>(&length), 4);

        assert(magic == 0x46546C67);


        uint32_t json_chunk_length;
        uint32_t json_chunk_type;

        in.read( reinterpret_cast<char*>(&json_chunk_length), 4);
        in.read( reinterpret_cast<char*>(&json_chunk_type), 4);

        assert(json_chunk_type == 0x4E4F534A);

        std::string json_string(json_chunk_length, ' ');

        in.read(&json_string[0], json_chunk_length);

        J = json::parse(json_string);

        uint32_t bin_chunk_length;
        uint32_t bin_chunk_type;

        in.read( reinterpret_cast<char*>(&bin_chunk_length), 4);
        in.read( reinterpret_cast<char*>(&bin_chunk_type), 4);


        assert(bin_chunk_type == 0x004E4942);
        buffers[0].resize( bin_chunk_length);
        in.read(buffers[0].data(), bin_chunk_length);

    }
    else
    {
        in >> J;

        uint32_t bi=0;
        for(auto & b : J["buffers"])
        {
            //std::cout << "Buffer found: " << b["name"] << std::endl;
            if( b.contains("uri"))
            {
                auto path = rootPath + "/" + b.at("uri").get<std::string>();
                std::ifstream i( path, std::ios_base::binary);

                buffers[bi] =
                        std::vector<char>( std::istreambuf_iterator<char>(i),
                                           std::istreambuf_iterator<char>() );
            }
            ++bi;
        }
    }


    struct Accessor
    {
        uint32_t stride = 0;
        uint32_t componentType = 0;
        std::string type;
    };



    auto _extractAccessor = [](nlohmann::json const & doc, nlohmann::json const & acc, std::map<uint32_t, std::vector<char>> const & bufs) -> VertexAttribute
    {
        auto type = acc.value("type", std::string("UNKNOWN"));
        auto componentType = acc.value("componentType", 0u);
        auto count = acc.value("count", 0u);
        auto byteOffset = acc.value("byteOffset", 0u);

        if(acc.count("bufferView") == 0 )
        {
            std::cout << "Sparse Accessor Found" << std::endl;
            auto V = initializeFromGLTFAccessor(count, componentType, type);
            return V;
            /*
            "sparse": {
                "count": 10,
                "indices": {
                    "bufferView": 1,
                    "byteOffset": 0,
                    "componentType": 5123
                },
                "values": {
                    "bufferView": 2,
                    "byteOffset": 0
                }
            }
            */
            if(acc.count("sparse") == 0)
                throw std::runtime_error("Invalid Accessor. bufferView is not set, and \"sprase\" property not set");

            auto & sparse = acc.at("sparse");

            std::cout << sparse.dump(4) << std::endl;
            auto sparse_count = sparse.at("count").get<uint32_t>();

            VertexAttribute indices;
            VertexAttribute values;
            {
                auto indices_count      = sparse_count;
                auto indices_bufferView = sparse.at(json::json_pointer("/indices/bufferView")).get<uint32_t>();
                auto indices_byteOffset = sparse.value(json::json_pointer("/indices/byteOffset"), 0u);
                auto indices_componentType = sparse.at(json::json_pointer("/indices/componentType")).get<uint32_t>();

                //auto bv_i = acc["bufferView"].get<uint32_t>();
                auto & bv = doc["bufferViews"][ indices_bufferView ];
                auto bufferViewByteStride = bv.value("byteStride", 0u);
                auto bufferViewByteOffset = bv.value("byteOffset", 0u);

                auto b_i  = bv["buffer"].get<uint32_t>();
                auto & buffer = bufs.at(b_i);

                auto attr = fromGLTFAccessor(buffer.data() + bufferViewByteOffset,
                                                bufferViewByteStride,
                                                indices_count,
                                                indices_byteOffset,
                                                indices_componentType,
                                                "SCALAR");
                indices = std::move(attr);
            }
            {
                auto values_count      = sparse_count;
                auto values_bufferView = sparse.at(json::json_pointer("/values/bufferView")).get<uint32_t>();
                auto values_byteOffset = sparse.value(json::json_pointer("/indices/byteOffset"), 0u);
                auto values_componentType = componentType;

                auto & bv = doc["bufferViews"][ values_bufferView ];
                auto b_i  = bv["buffer"].get<uint32_t>();
                auto & buffer = bufs.at(b_i);

                auto attr = fromGLTFAccessor(buffer.data() + bv.value("byteOffset", 0u),
                                                bv.value("byteStride", 0u),
                                                values_count,
                                                values_byteOffset,
                                                values_componentType,
                                                type);
                values = std::move(attr);
            }
#if 1
            //if(values.getComponentType() == eComponentType::FLOAT)
            {
                auto numComponents = values.getNumComponents();
                auto index_size    = indices.getAttributeSize();
                auto value_size    = values.getAttributeSize();

                auto indexCount = indices.attributeCount();

                for(uint32_t i=0;i<indexCount;i++)
                {
                    uint32_t index = 0;
                    switch(indices.getComponentType())
                    {
                        case eComponentType::BYTE:           index = static_cast<uint32_t>(indices.getComponentValue<uint8_t>(i));
                        case eComponentType::UNSIGNED_BYTE:  index = static_cast<uint32_t>(indices.getComponentValue<int8_t>(i));
                        case eComponentType::SHORT:          index = static_cast<uint32_t>(indices.getComponentValue<int16_t>(i));
                        case eComponentType::UNSIGNED_SHORT: index = static_cast<uint32_t>(indices.getComponentValue<uint16_t>(i));
                        case eComponentType::INT:            index = static_cast<uint32_t>(indices.getComponentValue<int32_t>(i));
                        case eComponentType::UNSIGNED_INT:   index = static_cast<uint32_t>(indices.getComponentValue<uint32_t>(i));
                        default:
                            throw std::runtime_error("Invalid component type. Must be integral");
                    }
#define DO_TYPE(TYPE)\
                    if(values.getComponentType() == type_to_component<TYPE>())\
                    for(uint j=0;j<numComponents;j++)\
                    {\
                        auto compVal = values.getComponentValue<TYPE>(numComponents*i + j);\
                        auto srcVal  = V.getComponentValue<TYPE>(numComponents * index + j);\
                        srcVal += compVal;\
                        V.setComponentValue<TYPE>(numComponents * index + j, srcVal);\
                    }

                    DO_TYPE(float)
                    DO_TYPE(uint16_t)
                    DO_TYPE(uint32_t)
                    DO_TYPE(int16_t)
                    DO_TYPE(int32_t)
                    DO_TYPE(double)
                    DO_TYPE(int8_t)
                    DO_TYPE(uint8_t)
                }
            }
#endif
            assert(values.attributeCount() == indices.attributeCount());
            return {};
        }
        else
        {
            std::cout << "Regular Accessor Found" << std::endl;
            auto bv_i = acc["bufferView"].get<uint32_t>();
            auto & bv = doc["bufferViews"][ bv_i ];
            auto b_i  = bv["buffer"].get<uint32_t>();
            auto & buffer = bufs.at(b_i);

            auto bufferViewByteStride = bv.value("byteStride", 0u);
            auto bufferViewByteOffset = bv.value("byteOffset", 0u);

            auto V = fromGLTFAccessor(buffer.data() + bufferViewByteOffset,
                                      bufferViewByteStride, count, byteOffset,componentType,type);
            return V;
        }
    };

    for(auto & a : J["accessors"])
    {
        //std::cout << a.dump(4) << std::endl;
        accessors.push_back( _extractAccessor(J, a, buffers) );
    };

    auto ends_with = [](std::string_view str, std::string_view suffix)
    {
        return str.size() >= suffix.size() && str.compare(str.size()-suffix.size(), suffix.size(), suffix) == 0;
    };

    std::cout << "Accessors Loaded successfully" << std::endl;
    for(auto & i : J["images"])
    {
        auto & Im = G.images.emplace_back();

        Im.mimeType = i.value("mimeType", std::string(""));
        Im.name = i.value("name", std::string(""));
        if(i.contains("uri"))
        {
            auto path = rootPath + "/" + i.at("uri").get<std::string>();
            Im.uri = i.at("uri").get<std::string>();

            // check if its jpg or png
            if( ends_with(Im.uri, "png") || ends_with(Im.uri, "jpg") || ends_with(Im.uri, "jpeg"))
                Im.img = gul::loadImage(path);
        }
        else if(i.contains("bufferView"))
        {
            //uint32_t bvi = ;
            auto & b  = J["bufferViews"][  i["bufferView"].get<uint32_t>()  ] ;
            uint32_t bufferIndex = b["buffer"];

            auto bufferData = buffers[bufferIndex].data();
            if(b.contains("byteOffset"))
            {
                bufferData += b["byteOffset"].get<uint32_t>();
            }
            auto byteLength = b["byteLength"].get<int32_t>();

            auto I = gul::loadImage(bufferData, byteLength);

            Im.img = std::move(I);
        }
    }

    for(auto & v : J["meshes"])
    {
        auto & m = G.meshes.emplace_back();
        for(auto & p : v["primitives"])
        {
            json P;
            P = p["attributes"];
            P["indices"] = p["indices"];

            if(meshPrimitives.count(P) == 0)
            {
                gul::MeshPrimitive M;
                if(P.contains("POSITION")) M.POSITION = accessors[   P["POSITION"].get<uint32_t>() ];
                if(P.contains("NORMAL")) M.NORMAL = accessors[     P["NORMAL"].get<uint32_t>() ];
                if(P.contains("TANGENT")) M.TANGENT = accessors[    P["TANGENT"].get<uint32_t>() ];
                if(P.contains("TEXCOORD_0")) M.TEXCOORD_0 = accessors[ P["TEXCOORD_0"].get<uint32_t>() ];
                if(P.contains("TEXCOORD_1")) M.TEXCOORD_1 = accessors[ P["TEXCOORD_1"].get<uint32_t>() ];
                if(P.contains("JOINTS_0")) M.JOINTS_0 = accessors[   P["JOINTS_0"].get<uint32_t>() ];
                if(P.contains("WEIGHTS_0")) M.WEIGHTS_0 = accessors[  P["WEIGHTS_0"].get<uint32_t>() ];
                if(P.contains("COLOR_0")) M.COLOR_0 = accessors[    P["COLOR_0"].get<uint32_t>() ];
                if(P.contains("indices")) M.INDEX = accessors[    P["indices"].get<uint32_t>() ];
                //std::cout << "Mesh found: " << v["name"] << ":  " << M.calculateDeviceSize() << std::endl;


                G.primitives.push_back(std::move(M));
                meshPrimitives[P] = uint32_t(G.primitives.size()-1);
            }

            auto & pr = m.primitive.emplace_back();
            pr.primitive.index = meshPrimitives.at(P);
            if(p.count("material"))
            {
                pr.material.index = p.at("material").get<uint32_t>();
            }
        }
    }

    for(auto & n : J["nodes"])
    {
        auto & N = G.nodes.emplace_back();
        if(n.contains("translation"))
        {
            N.position.x = n.at("translation")[0].get<float>();
            N.position.y = n.at("translation")[1].get<float>();
            N.position.z = n.at("translation")[2].get<float>();
        }
        if(n.contains("rotation"))
        {
            N.rotation.x = n.at("rotation")[0].get<float>();
            N.rotation.y = n.at("rotation")[1].get<float>();
            N.rotation.z = n.at("rotation")[2].get<float>();
            N.rotation.w = n.at("rotation")[3].get<float>();
        }
        if(n.contains("scale"))
        {
            N.scale.x = n.at("scale")[0].get<float>();
            N.scale.y = n.at("scale")[1].get<float>();
            N.scale.z = n.at("scale")[2].get<float>();
        }
        if(n.contains("children"))
        {
            N.children = n.at("children").get< std::vector<uint32_t> >();
        }
        if(n.contains("mesh"))
        {
            N.mesh.index = n.at("mesh").get< uint32_t >();
        }
        N.name = n.value("name", N.name);
    }

    for(auto & t : J["samplers"])
    {
        auto & S = G.samplers.emplace_back();
        if(t.contains("magFilter")) S.magFilter = t["magFilter"].get<int>();
        if(t.contains("minFilter")) S.magFilter = t["minFilter"].get<int>();
        if(t.contains("wrapS"))     S.wrapS = t["wrapS"].get<int>();
        if(t.contains("wrapT"))     S.wrapT = t["wrapT"].get<int>();
    }

    for(auto & t : J["textures"])
    {
        auto & S = G.textures.emplace_back();
        if(t.contains("sampler")) S.sampler.index  = t["sampler"].get<uint32_t>();
        if(t.contains("source"))  S.source .index  = t["source"].get<uint32_t>();
        if(t.contains("extensions"))
        {
            auto & ext = t.at("extensions");
            if(ext.contains("KHR_texture_basisu"))
            {
                auto & khr = S.KHR_texture_basisu.emplace();
                khr.source.index = ext.at("KHR_texture_basisu").value("source", 0xFFFFFFFF);
            }
        }
    }


    for(auto & t : J["materials"])
    {
        auto & S = G.materials.emplace_back();

        //json jj;
        #define _GETIF(VAR, jj, prop)\
        if(jj.contains(#prop))\
        {\
            VAR = jj[#prop].get< decltype(VAR) >();\
        }\

        if(t.contains("normalTexture"))
        {
            auto & normalTexture = t["normalTexture"];

            _GETIF(S.normalTexture.index.index , normalTexture, "index"   );
            _GETIF(S.normalTexture.texCoord    , normalTexture, "texCoord");
            _GETIF(S.normalTexture.scale       , normalTexture, "scale"   );
        }
        if(t.contains("occlusionTexture"))
        {
            auto & normalTexture = t["occlusionTexture"];

            _GETIF(S.occlusionTexture.index.index , normalTexture, "index"   );
            _GETIF(S.occlusionTexture.texCoord    , normalTexture, "texCoord");
            _GETIF(S.occlusionTexture.strength    , normalTexture, "strength"   );
        }
        if(t.contains("emissiveFactor"))
        {
            auto & bc = t["emissiveFactor"];
            S.emissiveFactor = glm::vec3(bc[0],bc[1],bc[2]);
        }
        if(t.contains("pbrMetallicRoughness"))
        {
            auto & pbrMetallicRoughness = t["pbrMetallicRoughness"];
            if(pbrMetallicRoughness.contains("baseColorFactor"))
            {
                auto & bc = pbrMetallicRoughness["baseColorFactor"];
                S.pbrMetallicRoughness.baseColorFactor = glm::vec4(bc[0],bc[1],bc[2],bc[3]);
            }
            if(pbrMetallicRoughness.contains("baseColorTexture"))
            {
                auto &bc = pbrMetallicRoughness["baseColorTexture"];
                S.pbrMetallicRoughness.baseColorTexture.index.index = bc.value("index", 0u);//bc["index"];
                S.pbrMetallicRoughness.baseColorTexture.texCoord = bc.value("texCoord", 0);
            }
            if(pbrMetallicRoughness.contains("metallicRoughnessTexture"))
            {
                auto &bc = pbrMetallicRoughness["metallicRoughnessTexture"];

                S.pbrMetallicRoughness.metallicRoughnessTexture.index.index = bc.value("index", 0u);
                S.pbrMetallicRoughness.metallicRoughnessTexture.texCoord = bc.value("texCoord", 0);
            }
            if(t.contains("emissiveTexture"))
            {
                auto &bc = t["emissiveTexture"];

                S.emissiveTexture.index.index = bc.value("index", 0u);
                S.emissiveTexture.texCoord = bc.value("texCoord", 0);
            }
            if(t.contains("occlusionTexture"))
            {
                auto &bc = t["occlusionTexture"];

                S.occlusionTexture.index.index = bc.value("index", 0u);
                S.occlusionTexture.texCoord = bc.value("texCoord", 0);
                S.occlusionTexture.strength = bc.value("strength", 1.f);
            }
            S.pbrMetallicRoughness.metallicFactor  = pbrMetallicRoughness.value("metallicFactor", 1.0f);
            S.pbrMetallicRoughness.roughnessFactor = pbrMetallicRoughness.value("roughnessFactor", 1.0f);

            if(t.contains("normalTexture"))
            {
                auto &bc = t["normalTexture"];

                S.normalTexture.index.index = bc.value("index", 0u);
                S.normalTexture.texCoord = bc.value("texCoord", 0);
            }

            S.alphaCutOff = t.value("alphaCutOff", 0.5f);
            S.alphaMode = t.value("alphaMode", std::string("OPAQUE"));
            S.doubleSided = t.value("doubleSided", false);
        }
    }

    return G;

}

}

#endif
