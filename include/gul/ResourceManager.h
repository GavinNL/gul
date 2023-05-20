#ifndef GUL_RESOURCE_MANAGER_H
#define GUL_RESOURCE_MANAGER_H

#include "uri.h"
#include <chrono>
#include <typeindex>
#include <mutex>
#include <optional>
#include <atomic>
#include <any>
#include <map>
#include <unordered_map>

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
struct Resource_t;

template<typename T>
using ResourceID = std::shared_ptr<Resource_t<T>>;

template<typename T>
using wResourceID = std::weak_ptr<Resource_t<T>>;


template<typename T>
struct SingleResourceManagerData
{
    using resource_type = T;
    using resource_handle  = ResourceID<resource_type>;

    using loader_function   = std::function<T(gul::uri const &)>;
    using unloader_function = std::function<void(resource_handle)>;

    std::vector< std::function<bool(resource_handle)> >   m_onLoadCallbacks;
    std::vector< std::function<bool(resource_handle)> >   m_onUnloadCallbacks;

    loader_function                 m_loader;
    unloader_function               m_unloader;
    std::unordered_map<std::string, ResourceID<T>> m_resources;
    std::mutex m_mutex;
};

enum class eResourceState : uint8_t
{
    NOT_LOADED,
    LOADING,
    LOADED
};

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
        return getState() == eResourceState::LOADED && value.has_value();
    }

    /**
     * @brief emplace_resource
     * @param v
     *
     * Sets the resource data
     */
    void emplace_resource(T && v)
    {
        auto L = getLockGuard();
        value = std::move(v);
        updateLoadTime();
        setState(eResourceState::LOADED);
        m_dirty = false;
        auto it = std::remove_if(m_data->m_onLoadCallbacks.begin(), m_data->m_onLoadCallbacks.end(),[self = m_self.lock()](auto &V)
                                 {
                                     return V(self);
                                 });
        m_data->m_onLoadCallbacks.erase(it, m_data->m_onLoadCallbacks.end());
    }

    /**
     * @brief updateLoadTime
     * @param loadTime
     *
     * Updates the load time of the current resource
     */
    void updateLoadTime(std::chrono::system_clock::time_point t = std::chrono::system_clock::now())
    {
        m_loadTime = t;
    }

    /**
     * @brief updateAccessTime
     * @param t
     *
     * Updates the resource's access time.
     */
    void updateAccessTime(std::chrono::system_clock::time_point t = std::chrono::system_clock::now())
    {
        m_accessTime = t;
    }

    auto getAccessTime() const
    {
        return m_accessTime;
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

    void setState(eResourceState s)
    {
        m_state = s;
    }
    eResourceState getState() const
    {
        return m_state;
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
        return getState() == eResourceState::LOADING;
    }

    /**
     * @brief unload
     *
     * Unloads the resource and calls any callback functions
     */
    void unload()
    {
        if(isLoaded())
        {
            auto self = m_self.lock();
            auto it = std::remove_if(m_data->m_onUnloadCallbacks.begin(), m_data->m_onUnloadCallbacks.end(),[self = m_self.lock()](auto &V)
            {
                return V(self);
            });
            m_data->m_onUnloadCallbacks.erase(it, m_data->m_onUnloadCallbacks.end());
            value.reset();
            m_unloadLater = false;
        }
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
            this->setState(eResourceState::LOADING);

            auto v = (m_data->m_loader)(uri);
            emplace_resource( std::move(v) );

            this->setState(eResourceState::LOADED);
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
        return (m_data->m_loader)(uri);
    }

    std::lock_guard<std::mutex> getLockGuard()
    {
        return std::lock_guard<std::mutex>(m_data->m_mutex);
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
        if(isLoaded())
        {
            return *value;
        }
        if(isLoading())
        {
            throw std::runtime_error("Resource is currently loading in the background: " + getUri().toString());
        }
        {
            auto b = getBackgroundLoader();
            b();
            return get();
        }
    }

    std::any const & getUserData() const
    {
        return m_userData;
    }
    std::any & getUserData()
    {
        return m_userData;
    }

    //================================================================
    // User Variables: Variables which can be attached to the resource
    // for user-specific things
    //================================================================

    // Get a reference to the map of variables
    std::unordered_map<std::string, std::any> & getUserVars()
    {
        return m_userVars;
    }
    std::unordered_map<std::string, std::any> const & getUserVars() const
    {
        return m_userVars;
    }

    template<typename V>
    V& setUserVar(std::string const & x, V const & val)
    {
        auto & vv = m_userVars[x];
        vv = val;
        return std::any_cast<V&>(x);
    }

    template<typename V>
    V& setUserVar(std::string const & x, V && val)
    {
        auto & vv = m_userVars[x];
        vv = std::move(val);
        return std::any_cast<V&>(x);
    }

    std::any const & getUserVar(std::string const & x) const
    {
        return m_userVars.at(x);
    }

    std::any & getUserVar(std::string const & x)
    {
        return m_userVars[x];
    }

    template<typename V>
    V const & getUserVar(std::string const & x) const
    {
        return std::any_cast<V const&>(getUserVar(x));
    }
    template<typename V>
    V & getUserVar(std::string const & x)
    {
        return std::any_cast<V&>(getUserVar(x));
    }

    void eraseUserVar(std::string const & x)
    {
        m_userVars.erase(x);
    }

protected:
    wResourceID<resource_type> m_self;
    std::shared_ptr<SingleResourceManagerData<resource_type> > m_data;
    std::optional<T>                                     value;
    gul::uri                                             uri;
    std::chrono::system_clock::time_point                m_loadTime;
    std::chrono::system_clock::time_point                m_accessTime = std::chrono::system_clock::now(); // the last time this resource was accessed
    std::unordered_map<std::string, std::any>            m_userVars;

    std::atomic<eResourceState> m_state = {};

    bool m_unloadLater         = false;
    bool m_dirty               = true;
    //std::atomic<bool> m_isBackgroundLoading = false;

    std::any m_userData;
    friend struct SingleResourceManager<T>;
};

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

    SingleResourceManager()
    {
        m_data = std::make_shared<SingleResourceManagerData<resource_type> >();
    }
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
        std::lock_guard<std::mutex> L(m_data->m_mutex);
        auto &r = m_data->m_resources[uri.toString()];
        if(!r)
        {
            r = std::make_shared< Resource_t<T> >(uri);
            r->m_data = m_data;
            r->m_self = r;
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
        m_data->m_loader = std::move(C);
    }

    /**
     * @brief insertOnLoadCallback
     * @param v
     *
     * Sets function which will be called when the resource is emplaced.
     * when id->emplace_resource( ) is called
     *
     * The callback function should return TRUE if the callback function
     * should be removed after it is called.
     */
    void insertOnLoadCallback(std::function<bool( resource_handle )> && v)
    {
        m_data->m_onLoadCallbacks.push_back(v);
    }

    /**
     * @brief insertOnUnloadCallback
     * @param v
     *
     * Sets a function to be called when a resource is to be unloaded.
     * The callback function will be called BEFORE the resource data is removed.
     * id->get() will still be able to be called.
     *
     * The callback function should return TURE if the callback function should
     * be removed after it is called.
     *
     */
    void insertOnUnloadCallback(std::function<bool( resource_handle )> && v)
    {
        m_data->m_onUnloadCallbacks.push_back(v);
    }

    /**
     * @brief processUnload
     *
     * Checks if any resources can be unloaded and unloads them.
     * This will call any onUnload callbacks.
     */
    void processUnload()
    {
        for(auto & [a,b] : m_data->m_resources)
        {
            (void)a;
            if(b->m_unloadLater)
            {
                b->unload();
            }
        }
    }

    template<typename callable_t>
    void forEach(callable_t &&  c)
    {
        for(auto & [a,b] : m_data->m_resources)
        {
            c(b);
        }
    }

    auto begin()
    {
        return m_data->m_resources.begin();
    }
    auto end()
    {
        return m_data->m_resources.end();
    }

protected:
    std::shared_ptr<SingleResourceManagerData<resource_type> > m_data;
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
            m_singleResources[i] = std::static_pointer_cast< void >(f);
            return getSingleResourceManager<T>();
        }
        return std::static_pointer_cast< SingleResourceManager<T> >(f);
    }
protected:
    std::map< std::type_index, std::shared_ptr<void> > m_singleResources;
};


}

#endif
