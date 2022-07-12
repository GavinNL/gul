#ifndef GUL_OCTREE_H
#define GUL_OCTREE_H

#include <glm/glm.hpp>
#include <vector>
#include "aabb.h"
#include <unordered_map>
#include <algorithm>

namespace gul
{


template<typename T>
class OctreeNode
{
public:
    using value_type = T;
    using vec_type   = glm::vec3;
    using bb_type    = bb3f;
    using node_type  = OctreeNode<T>;

    using V = std::pair<value_type, bb_type>;

    OctreeNode(vec_type const & centerPoint, float powerOfTwo) : boundingBox(centerPoint-vec_type(powerOfTwo),centerPoint+vec_type(powerOfTwo))
    {

    }


    OctreeNode(vec_type const & lB, vec_type const & uB) : boundingBox(lB,uB)
    {

    }


    size_t size() const
    {
        auto c = objects.size();
        for(auto & CH : children)
        {
            c += CH.size();
        }
        return c;
    }
    void erase(T const & v)
    {
        auto it = std::remove_if(objects.begin(), objects.end(), [&](auto & p) {return p.first==v;});
        objects.erase(it, objects.end());
    }


    OctreeNode* insert(T const & value, bb_type const & b)
    {
        // if there are no objects in this node
        // then we can place it directly in its
        // object list
        if(objects.size() == 0)
        {
            objects.emplace_back(value, b);
            return this;
        }
        else
        {
            // if the object was not inserted,
            // we need to split the node and into 8 children
            if(children.size() == 0)
            {
                split();
            }

            for(auto & c : children)
            {
                // check if the object can be fully contained within
                // the child node
                if( c.boundingBox.contains(b))
                {
                    return c.insert(value, b);
                }
            }
            objects.emplace_back(value, b);
        }
        return nullptr;
    }

    /**
     * @brief split
     *
     * Split the box into 8 children
     */
    void split()
    {
        auto bb = boundingBox;
        bb.upperBound = mix(bb.lowerBound, bb.upperBound, 0.5f);

        auto D = bb.upperBound-bb.lowerBound;
        static constexpr vec_type offsets[] = {  {0,0,0},
                                {0,0,1},
                                {0,1,0},
                                {0,1,1},
                                {1,0,0},
                                {1,0,1},
                                {1,1,0},
                                {1,1,1}
                             };
        children.reserve(8);
        for(auto & o : offsets)
        {
            children.emplace_back( bb.lowerBound + D*o, bb.upperBound + D*o);
        }

    }

    template<typename geometry_type, typename D>
    void query(geometry_type const & b, D && callable) const
    {
        if( intersects(boundingBox,b))
        {
            for(auto & o : objects)
            {
                if(intersects(o.second,b))
                {
                    callable(o.first);
                }
            }
            for(auto & c : children)
            {
                c.query(b, callable);
            }
        }
    }

    bb_type                 boundingBox;
    std::vector<node_type>  children;
    std::vector<V>          objects;
};

template<typename T>
class Octree
{
public:
    using value_type = T;
    using vec_type   = glm::vec3;
    using bb_type    = bb3f;
    using node_type  = OctreeNode<T>;

    struct RegistryData
    {
        bb_type    box;
        node_type *node;
    };


    Octree(vec_type const & centerPoint, float powerOfTwo) : m_node(centerPoint-vec_type(powerOfTwo),centerPoint+vec_type(powerOfTwo))
    {

    }


    Octree(vec_type const & lB, vec_type const & uB) : m_node(lB,uB)
    {

    }

    size_t size() const
    {
        return m_node.size();
    }
    void erase(value_type const & v)
    {
        auto & X = m_objPosition.at(v);
        X.node->erase(v);
    }


    void insert(value_type const & v, bb_type const & box)
    {
        auto np = m_node.insert(v, box);
        if(np)
        {
            auto & K = m_objPosition[v];
            K.box    = box;
            K.node   = np;
        }
    }

    template<typename geometry_type, typename D>
    void query( std::remove_cv_t<geometry_type> const & b, D && callable) const
    {
        m_node.query(b, callable);
    }
protected:
    std::unordered_map<T, RegistryData> m_objPosition;
    node_type                           m_node;
};

}

#endif
