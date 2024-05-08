#ifndef GUL_MESH_PRIMITIVE_OPTIMIZER_H
#define GUL_MESH_PRIMITIVE_OPTIMIZER_H

#include "MeshPrimitive2.h"
#include <meshoptimizer.h>

namespace gul
{

/**
 * @brief OptimizeMeshPrimitive
 * @param V
 *
 * Uses Arseny Kapoulkine's MeshOptimizer library to generate Level of Details
 * for the mesh primitive. ( https://github.com/zeux/meshoptimizer )
 *
 * Each level of detail is a set of different indices which are stored
 * as submeshes.
 *
 * Requirements:
 *          The submesh array in the MeshPrimitive must be cleared
 *          The mesh MUST have index buffers
 *          The index buffer must be uint32
 *
 * GenerateMeshLOD(M, 10, 0.7f, 0.01f);
 * assert(M.subMeshes.size() > 0);
 *
 */
inline std::vector<DrawCall> GenerateMeshLOD(MeshPrimitive & V, size_t lod_count = 10, float thresholdPowerBase=0.7f, float target_error = 1e-2)
{
    assert( V.subMeshes.size() == 0);
    assert( V.indexCount() > 0);
    assert( V.INDEX.getAttributeSize() == 4);

    const size_t kCacheSize = 16;


    uint32_t vertexStride = V.POSITION.getAttributeSize();
    uint32_t vertexCount  = V.POSITION.attributeCount();
    auto      vertexData  = static_cast<float const*>(V.POSITION.data());

    // generate 4 LOD levels (1-4), with each subsequent LOD using 70% triangles
    // note that each LOD uses the same (shared) vertex buffer
    std::vector<std::vector<uint32_t>> lods(lod_count);

    auto indices = V.INDEX.toVector<uint32_t>();
    lods[0] = indices;

    for (size_t i = 1; i < lod_count; ++i)
    {
        std::vector<unsigned int>& lod = lods[i];

        float threshold = powf(thresholdPowerBase, float(i));
        size_t target_index_count = size_t(indices.size() * threshold) / 3 * 3;
        //float target_error = 1e-2f;

        // we can simplify all the way from base level or from the last result
        // simplifying from the base level sometimes produces better results, but simplifying from last level is faster
        const std::vector<unsigned int>& source = lods[i - 1];
        //const std::vector<unsigned int>& source = lods[0];

        if (source.size() < target_index_count)
            target_index_count = source.size();

        lod.resize(source.size());
        lod.resize(meshopt_simplify(&lod[0], &source[0], source.size(), vertexData, vertexCount, vertexStride, target_index_count, target_error));
    }


    // optimize each individual LOD for vertex cache & overdraw
    for (size_t i = 0; i < lod_count; ++i)
    {
        std::vector<unsigned int>& lod = lods[i];

        meshopt_optimizeVertexCache(&lod[0], &lod[0], lod.size(), vertexCount);
        meshopt_optimizeOverdraw(&lod[0], &lod[0], lod.size(), vertexData, vertexCount, vertexStride, 1.0f);
    }

    // concatenate all LODs into one IB
    // note: the order of concatenation is important - since we optimize the entire IB for vertex fetch,
    // putting coarse LODs first makes sure that the vertex range referenced by them is as small as possible
    // some GPUs process the entire range referenced by the index buffer region so doing this optimizes the vertex transform
    // cost for coarse LODs
    // this order also produces much better vertex fetch cache coherency for coarse LODs (since they're essentially optimized first)
    // somewhat surprisingly, the vertex fetch cache coherency for fine LODs doesn't seem to suffer that much.
    auto lod_index_offsets = std::vector<size_t>(lod_count, 0);//[lod_count] = {};
    auto lod_index_counts  = std::vector<size_t>(lod_count, 0);//[lod_count] = {};
    size_t total_index_count = 0;

    for (int i = lod_count - 1; i >= 0; --i)
    {
        lod_index_offsets[i] = total_index_count;
        lod_index_counts[i] = lods[i].size();

        total_index_count += lods[i].size();
    }

    indices.resize(total_index_count);

    V.subMeshes.clear();
    std::vector<DrawCall> _out;
    for (size_t i = 0; i < lod_count; ++i)
    {
        memcpy(&indices[lod_index_offsets[i]], &lods[i][0], lods[i].size() * sizeof(lods[i][0]));
        DrawCall dc;
        dc.indexCount   = uint32_t(lods[i].size());
        dc.indexOffset  = lod_index_offsets[i];
        dc.vertexCount  = 0;
        dc.vertexOffset = 0;

        V.subMeshes.push_back(dc);
        _out.push_back(dc);
    }
    V.INDEX = indices;

    return _out;
    #if 0
    return lods;
    std::vector<Vertex> vertices = mesh.vertices;

    // vertex fetch optimization should go last as it depends on the final index order
    // note that the order of LODs above affects vertex fetch results
    meshopt_optimizeVertexFetch(&vertices[0], &indices[0], indices.size(), &vertices[0], vertices.size(), vertexStride);

    double end = timestamp();

    printf("%-9s: %d triangles => %d LOD levels down to %d triangles in %.2f msec, optimized in %.2f msec\n",
        "SimplifyC",
        int(lod_index_counts[0]) / 3, int(lod_count), int(lod_index_counts[lod_count - 1]) / 3,
        (middle - start) * 1000, (end - middle) * 1000);

    // for using LOD data at runtime, in addition to vertices and indices you have to save lod_index_offsets/lod_index_counts.

    {
        meshopt_VertexCacheStatistics vcs0 = meshopt_analyzeVertexCache(&indices[lod_index_offsets[0]], lod_index_counts[0], vertices.size(), kCacheSize, 0, 0);
        meshopt_VertexFetchStatistics vfs0 = meshopt_analyzeVertexFetch(&indices[lod_index_offsets[0]], lod_index_counts[0], vertices.size(), vertexStride);
        meshopt_VertexCacheStatistics vcsN = meshopt_analyzeVertexCache(&indices[lod_index_offsets[lod_count - 1]], lod_index_counts[lod_count - 1], vertices.size(), kCacheSize, 0, 0);
        meshopt_VertexFetchStatistics vfsN = meshopt_analyzeVertexFetch(&indices[lod_index_offsets[lod_count - 1]], lod_index_counts[lod_count - 1], vertices.size(), vertexStride);

        typedef PackedVertexOct PV;

        std::vector<PV> pv(vertices.size());
        packMesh(pv, vertices);

        std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(vertices.size(), sizeof(PV)));
        vbuf.resize(meshopt_encodeVertexBuffer(&vbuf[0], vbuf.size(), &pv[0], vertices.size(), sizeof(PV)));

        std::vector<unsigned char> ibuf(meshopt_encodeIndexBufferBound(indices.size(), vertices.size()));
        ibuf.resize(meshopt_encodeIndexBuffer(&ibuf[0], ibuf.size(), &indices[0], indices.size()));

        printf("%-9s  ACMR %f...%f Overfetch %f..%f Codec VB %.1f bits/vertex IB %.1f bits/triangle\n",
            "",
            vcs0.acmr, vcsN.acmr, vfs0.overfetch, vfsN.overfetch,
            double(vbuf.size()) / double(vertices.size()) * 8,
            double(ibuf.size()) / double(indices.size() / 3) * 8);
    }
#endif
}


inline std::vector<DrawCall> GenerateMeshLOD(MeshPrimitive & V, size_t lod_count = 10, float thresholdPowerBase=0.7f, float target_error = 1e-2)
{
    assert( V.subMeshes.size() == 0);
    assert( V.indexCount() > 0);
    assert( V.INDEX.getAttributeSize() == 4);

    const size_t kCacheSize = 16;


    uint32_t vertexStride = V.POSITION.getAttributeSize();
    uint32_t vertexCount  = V.POSITION.attributeCount();
    auto      vertexData  = static_cast<float const*>(V.POSITION.data());

    // generate 4 LOD levels (1-4), with each subsequent LOD using 70% triangles
    // note that each LOD uses the same (shared) vertex buffer
    std::vector<std::vector<uint32_t>> lods(lod_count);

    auto indices = V.INDEX.toVector<uint32_t>();
    lods[0] = indices;

    for (size_t i = 1; i < lod_count; ++i)
    {
        std::vector<unsigned int>& lod = lods[i];

        float threshold = powf(thresholdPowerBase, float(i));
        size_t target_index_count = size_t(indices.size() * threshold) / 3 * 3;
        //float target_error = 1e-2f;

        // we can simplify all the way from base level or from the last result
        // simplifying from the base level sometimes produces better results, but simplifying from last level is faster
        const std::vector<unsigned int>& source = lods[i - 1];
        //const std::vector<unsigned int>& source = lods[0];

        if (source.size() < target_index_count)
            target_index_count = source.size();

        lod.resize(source.size());
        lod.resize(meshopt_simplify(&lod[0], &source[0], source.size(), vertexData, vertexCount, vertexStride, target_index_count, target_error));
    }


    // optimize each individual LOD for vertex cache & overdraw
    for (size_t i = 0; i < lod_count; ++i)
    {
        std::vector<unsigned int>& lod = lods[i];

        meshopt_optimizeVertexCache(&lod[0], &lod[0], lod.size(), vertexCount);
        meshopt_optimizeOverdraw(&lod[0], &lod[0], lod.size(), vertexData, vertexCount, vertexStride, 1.0f);
    }

    // concatenate all LODs into one IB
    // note: the order of concatenation is important - since we optimize the entire IB for vertex fetch,
    // putting coarse LODs first makes sure that the vertex range referenced by them is as small as possible
    // some GPUs process the entire range referenced by the index buffer region so doing this optimizes the vertex transform
    // cost for coarse LODs
    // this order also produces much better vertex fetch cache coherency for coarse LODs (since they're essentially optimized first)
    // somewhat surprisingly, the vertex fetch cache coherency for fine LODs doesn't seem to suffer that much.
    auto lod_index_offsets = std::vector<size_t>(lod_count, 0);//[lod_count] = {};
    auto lod_index_counts  = std::vector<size_t>(lod_count, 0);//[lod_count] = {};
    size_t total_index_count = 0;

    for (int i = lod_count - 1; i >= 0; --i)
    {
        lod_index_offsets[i] = total_index_count;
        lod_index_counts[i] = lods[i].size();

        total_index_count += lods[i].size();
    }

    indices.resize(total_index_count);

    V.subMeshes.clear();
    std::vector<DrawCall> _out;
    for (size_t i = 0; i < lod_count; ++i)
    {
        memcpy(&indices[lod_index_offsets[i]], &lods[i][0], lods[i].size() * sizeof(lods[i][0]));
        DrawCall dc;
        dc.indexCount   = uint32_t(lods[i].size());
        dc.indexOffset  = lod_index_offsets[i];
        dc.vertexCount  = 0;
        dc.vertexOffset = 0;

        V.subMeshes.push_back(dc);
        _out.push_back(dc);
    }
    V.INDEX = indices;

    return _out;
}


}

#endif

