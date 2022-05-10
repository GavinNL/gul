#ifndef GUL_RESOURCE_MANAGER_H
#define GUL_RESOURCE_MANAGER_H

#include "uri.h"
#include <filesystem>
#include <chrono>
#include <typeindex>

namespace gul
{

template<typename T>
struct SingleResourceManager;

template<typename T>
struct Resource_t
{
    using resource_type = T;

    Resource_t(gul::uri const & _u) : uri(_u)
    {

    }

    gul::uri const & getUri() const
    {
        return uri;
    }

    bool isLoaded() const
    {
        return value.has_value();
    }

    /**
     * @brief emplace_resource
     * @param v
     *
     * Ssets the resource data
     */
    void emplace_resource(T && v)
    {
        std::unique_lock L(*m_mutex);
        value = std::move(v);
        m_loadTime = std::chrono::system_clock::now();
    }

    /**
     * @brief load
     * @return
     *
     * Loads the resource and returns true if the resource was loaded
     * and false if the resource has already been loaded
     */
    bool load()
    {
        if(!value.has_value())
        {
            auto v = (*m_loader)(uri);
            emplace_resource( std::move(v) );
            return true;
        }
        return false;
    }

    auto getLoadTime() const
    {
        return m_loadTime;
    }

    /**
     * @brief get
     * @return
     *
     * Returns a reference to the actual data.
     * If the data is not loaded. the loader will be called.
     *
     */
    T & get()
    {
        if(!value.has_value())
        {
            value = (*m_loader)(uri);
        }
        return *value;
    }

protected:
    using loader_function   = std::function<T(gul::uri const &)>;
    using unloader_function = std::function<void(std::shared_ptr<Resource_t<T>>)>;

    std::optional<T>                                     value;
    gul::uri                                             uri;
    std::shared_ptr<std::function<T(gul::uri const &C)>> m_loader;
    std::chrono::system_clock::time_point                m_loadTime;
    std::shared_ptr<std::mutex>                          m_mutex;
    bool m_dirty = true;
    friend class SingleResourceManager<T>;
};

template<typename T>
using ResourceID = std::shared_ptr<Resource_t<T>>;

template<typename T>
struct SingleResourceManager
{
    using resource_type = T;
    using resource_handle  = ResourceID<resource_type>;

    /**
     * @brief findResource
     * @param uri
     * @return
     *
     * Finds a resource. Returns a the ResourceID.
     * This does not load the resource data. It simly
     * returns the handle
     */
    resource_handle findResource(gul::uri const & uri)
    {
        std::unique_lock L(m_mutex);
        auto &r = m_resources[uri.toString()];
        if(!r)
        {
            r = std::make_shared< Resource_t<T> >(uri);
            r->m_loader = m_loader;
            r->m_mutex  = m_mutex;
        }
        return r;
    }
    resource_handle find(gul::uri const & uri)
    {
        return findResource(uri);
    }

    /**
     * @brief get
     * @param u
     * @return
     *
     * Load a resource from a uri. Returns the reference to the
     * resource
     */
    resource_handle get( gul::uri const & u)
    {
        auto r = findResource(u);
        r->load();
        return r;
    }

    /**
     * @brief setLoader
     * @param C
     *
     * Sets the resource loader which can load the uri
     * from disk. The signature of the function should be
     *
     * T [](gul::uri const & _uri)
     * {
     * }
     */
    template<typename callable_t>
    void setLoader(callable_t && C)
    {
        m_loader = std::make_shared<loader_function>();
        *m_loader = C;
        for(auto & [a,b] : m_resources)
        {
            b->m_loader = m_loader;
        }
    }

protected:
    using loader_function = std::function<T(gul::uri const &)>;
    using unloader_function = std::function<void(resource_handle)>;
    std::shared_ptr<loader_function>               m_loader;
    std::shared_ptr<unloader_function>             m_unloader;
    std::unordered_map<std::string, ResourceID<T>> m_resources;
    std::shared_ptr<std::mutex>                    m_mutex = std::make_shared<std::mutex>();
};

class ResourceManager
{
public:
    template<typename T>
    ResourceID<T> get(gul::uri const & _uri)
    {
        auto l = getSingleResourceManager<T>();
        return l->get(_uri);
    }

    template<typename T>
    typename SingleResourceManager<T>::resource_handle findResource(gul::uri const & u)
    {
        auto l = getSingleResourceManager<T>();
        return l->findResource(u);
    }
    template<typename T, typename callable_t>
    void setLoader(callable_t && C)
    {
        auto l = getSingleResourceManager<T>();
        l->setLoader(C);
    }

    template<typename T>
    std::shared_ptr< SingleResourceManager<T> > getSingleResourceManager()
    {
        std::type_index i(typeid (T));
        auto & f = m_singleResources[i];
        if(!f)
        {
            f = std::make_shared< SingleResourceManager<T> >();
            std::static_pointer_cast< void >(f);
        }
        return std::static_pointer_cast< SingleResourceManager<T> >(f);
    }
    std::map< std::type_index, std::shared_ptr<void> > m_singleResources;
};


}

#endif
