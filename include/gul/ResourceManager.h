#ifndef GUL_RESOURCE_MANAGER_H
#define GUL_RESOURCE_MANAGER_H

#include "uri.h"
#include <chrono>
#include <typeindex>
#include <mutex>
#include <optional>
#include <atomic>


// C++17 includes the <filesystem> library, but
// unfortunately gcc7 does not have a finalized version of it
// it is in the <experimental/filesystem lib
// this section includes the proper header
// depending on whether the header exists and
// includes that. It also sets the
// nfcbn::nf namespace
#if __has_include(<filesystem>)

    #include <filesystem>
    namespace gul
    {
        namespace fs = std::filesystem;
    }

#elif __has_include(<experimental/filesystem>)

    #include <experimental/filesystem>
    namespace gul
    {
        namespace fs = std::experimental::filesystem;
    }

#else
    #error There is no <filesystem> or <experimental/filesystem>
#endif


namespace gul
{


template <typename TP>
inline std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
              + system_clock::now());
    return system_clock::to_time_t(sctp);
}

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

    /**
     * @brief isLoaded
     * @return
     *
     * Returns true if the resource has been loaded and is
     * available through get();
     */
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
        auto L = getLockGuard();
        value = std::move(v);
        updateLoadTime();
        setIsLoading(false);
    }

    /**
     * @brief updateLoadTime
     * @param loadTime
     *
     * Updates the load time of the current resource
     */
    void updateLoadTime(std::chrono::system_clock::time_point loadTime = std::chrono::system_clock::now())
    {
        m_loadTime = loadTime;
    }

    /**
     * @brief load
     * @return
     *
     * Loads the resource and returns true if the resource was loaded
     * and false if the resource has already been loaded.
     *
     * This function should be called on the main thread.
     *
     */
    bool load()
    {
        if(!value.has_value())
        {
            auto f = getBackgroundLoader();
            f(); // call it on the same thread
            return true;
        }
        return false;
    }

    /**
     * @brief isLoading
     * @return
     *
     * Returns if the resource is currently scheduled
     * or loading in the background.
     */
    bool isLoading() const
    {
        return m_isBackgroundLoading;
    }

    void setIsLoading(bool t)
    {
        m_isBackgroundLoading.store(t);
    }

    /**
     * @brief loadBackground
     *
     * gets a functional object which can be called on a different thread
     * do load the resource in the background.
     */
    auto getBackgroundLoader()
    {
        return
        [this]()
        {
            this->setIsLoading(true);

            auto v = (*m_loader)(uri);
            emplace_resource( std::move(v) );

            this->setIsLoading(false);
        };
    }

    /**
     * @brief loadCopy
     *
     * Call the loader function and return a copy of the object
     * that was loaded. This does not modify the resource
     */
    auto loadCopy() const
    {
        return (*m_loader)(uri);
    }

    std::lock_guard<std::mutex> getLockGuard()
    {
        return std::lock_guard<std::mutex>(*this->m_mutex);
    }

    auto getLoadTime() const
    {
        return m_loadTime;
    }

    auto getLoadTime_time_t() const
    {
        return to_time_t(getLoadTime());
    }


    /**
     * @brief scheduleUnload
     *
     * Sets the flag to unload the resource at a later time
     *
     */
    void scheduleUnload()
    {
        m_unloadLater = true;
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
        if(m_isBackgroundLoading)
        {
            throw std::runtime_error("Resource is currently loading in the background.");
        }
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

    bool m_unloadLater         = false;
    bool m_dirty               = true;
    std::atomic<bool> m_isBackgroundLoading = false;

    friend struct SingleResourceManager<T>;
};

template<typename T>
using ResourceID = std::shared_ptr<Resource_t<T>>;


/**
 * @brief The SingleResourceManager struct
 *
 * Manages a single resource type
 */
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
        std::lock_guard<std::mutex> L(*m_mutex);
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

    /**
     * @brief processUnload
     *
     * Checks if any resources can be unloaded
     */
    void processUnload()
    {
        for(auto & [a,b] : m_resources)
        {
            if(b->m_unloadLater)
            {
                b->value.reset();
                b->m_unloadLater = false;
            }
        }
    }

    template<typename callable_t>
    void forEach(callable_t &&  c)
    {
        for(auto & [a,b] : m_resources)
        {
            c(b);
        }
    }

    auto begin()
    {
        return m_resources.begin();
    }
    auto end()
    {
        return m_resources.end();
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
    static auto getFileModifyTime_time_t(fs::path const & p)
    {
        fs::file_time_type file_time = last_write_time(p);
        return to_time_t(file_time);
    }

    /**
     * @brief get
     * @param _uri
     * @return
     *
     * Returns a specifc resource. The resource will be loaded if it hasn't already been
     */
    template<typename T>
    ResourceID<T> get(gul::uri const & _uri)
    {
        auto l = getSingleResourceManager<T>();
        return l->get(_uri);
    }

    /**
     * @brief findResource
     * @param u
     * @return
     *
     * Finds a specific resource. The resource may or may not be loaded.
     */
    template<typename T>
    typename SingleResourceManager<T>::resource_handle findResource(gul::uri const & u)
    {
        auto l = getSingleResourceManager<T>();
        return l->findResource(u);
    }

    /**
     * @brief setLoader
     * @param C
     *
     * Sets the resource loader for a particular resource.
     *
     * The functional should have the form:
     *
     *     T (gul::uri const & _uri)
     */
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
protected:
    std::map< std::type_index, std::shared_ptr<void> > m_singleResources;
};


}

#endif
