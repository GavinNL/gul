#ifndef GUL_MESH_PRIMITIVE_TINY_OBJ_H
#define GUL_MESH_PRIMITIVE_TINY_OBJ_H

#include "MeshPrimitive2.h"
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>
#include <filesystem>


namespace gul
{

inline MeshPrimitive MeshPrimitive_TinyObjLoad(std::filesystem::path const &inputFile,
                             std::string *error,
                             std::string *warning)
{
    std::string inputfile = inputFile.generic_string();//"cornell_box.obj";
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(inputfile, reader_config)) {
        if (!reader.Error().empty()) {
            *error = reader.Error();
        }
        return {};
        exit(1);
    }

    if (!reader.Warning().empty()) {
        *warning = reader.Warning();
        //std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
    (void)materials;

    MeshPrimitive output;
    uint32_t _index=0;
    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++)
    {
        auto & _shape = shapes[s];

        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < _shape.mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(_shape.mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                tinyobj::index_t idx = _shape.mesh.indices[index_offset + v];

                tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];

                output.POSITION.push_back(vx);
                output.POSITION.push_back(vy);
                output.POSITION.push_back(vz);
                (void)vx;
                (void)vy;
                (void)vz;
                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
                    tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
                    tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
                    output.NORMAL.push_back(nx);
                    output.NORMAL.push_back(ny);
                    output.NORMAL.push_back(nz);
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
                    tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];

                    output.TEXCOORD_0.push_back(tx);
                    output.TEXCOORD_0.push_back(ty);
                }

                // Optional: vertex colors
                // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
                // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
                // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
            }
            index_offset += fv;

            output.INDEX.push_back(_index++);
            output.INDEX.push_back(_index++);
            output.INDEX.push_back(_index++);
            // per-face material
            //_shape.mesh.material_ids[f];
        }
    }
    return output;
}


}

#endif

