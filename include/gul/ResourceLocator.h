#ifndef GUL_UTILS_RESOURCE_LOCATOR
#define GUL_UTILS_RESOURCE_LOCATOR

#include<string>
#include<algorithm>
#include<memory>
#include<vector>
#include<stdexcept>
#include<unordered_set>
#include<fstream>

#if defined __linux__
    #include <unistd.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <pwd.h>
#endif

#if defined _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <stdlib.h>
#endif

#if defined __APPLE__
    #include <unistd.h>
    #include <libproc.h>
#endif

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

/**
 * @brief getExecutablePath
 * @return
 *
 * Returns the path to the current executable
 */
inline std::string getExecutablePath()
{
    char path[FILENAME_MAX];

#if defined __linux__
    auto i = readlink("/proc/self/exe", path, FILENAME_MAX );
    path[i] = 0;
    return path;
#elif defined _WIN32
    auto i = GetModuleFileNameA( NULL, path, FILENAME_MAX);
    auto p = path_type( path );
    return join( get_cwd(), path);
#elif defined __APPLE__

    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

    auto pid = getpid();
    auto ret = proc_pidpath (pid, pathbuf, sizeof(pathbuf));
    if ( ret <= 0 ) {
        return "";
    } else {
        return pathbuf;
    }
#endif
}


/**
 * @brief The ResourceLocator class
 *
 * The resources container allows you to add root paths to it. for example
 *
 * ResourceLocator r;
 * r.push_back("/usr/bin");
 * r.push_back("/usr/local/bin");
 * r.push_back("/bin");
 *
 * And then search for files relatives to the rooth paths.
 *
 * assert( r.get("bash") == "/bin/bash");
 *
 */
class ResourceLocator
{
public:
    std::vector< fs::path > roots;


    /**
     * @brief addPath
     * @param path
     *
     * Add a path to the set of resource paths.
     */
    void push_back(fs::path const& absPath)
    {
        if( !absPath.is_absolute() )
        {
            throw std::runtime_error("Must be an absolute path");
        }
        roots.push_back(absPath);
    }

    /**
     * @brief clearPaths
     *
     * Clear all the paths.
     */
    void clear()
    {
        roots.clear();
    }

    size_t size() const
    {
        return roots.size();
    }

    std::vector<uint8_t> readResourceBIN(const fs::path &relPath) const
    {
        auto path = locate(relPath);
        if( path == "")
        {
            throw std::invalid_argument( std::string("File does not exist: ") + relPath.string());
        }
        std::ifstream stream(path, std::ios::in | std::ios::binary);
        std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        return contents;
    }

    std::string readResourceASCII(const fs::path &relPath) const
    {
        auto path = locate(relPath);
        if( path == "")
        {
            throw std::invalid_argument( std::string("File does not exist: ") + relPath.string());
        }
        std::ifstream t( path );
        if(!t)
        {
            throw std::invalid_argument( std::string("Unable to open file: ") + path.string());
        }
        std::string asciiSrc((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        return asciiSrc;
    }

    /**
     * @brief locate
     * @param file_name
     * @return
     *
     * Locate a resource, returns the absolute path to the
     * resource
     */
    fs::path locate(fs::path const& relPath) const
    {
        for(auto const & p1 : roots )
        {
            auto abs_path = p1 / relPath;

            if( fs::exists(abs_path) )
            {
                return abs_path;
            }
        }
        return {};
    }

    /**
     * @brief listDirectory
     * @param dirPath
     * @return
     *
     * Returns a vector of file paths contained in dirPath.
     *
     * dirPath must be a relative path
     */
    std::vector<fs::path> listDirectory( fs::path const & dirPath) const
    {
        auto absDirPath = locate(dirPath);
        if( !fs::is_directory(absDirPath) )
        {
            throw std::runtime_error("Must be a directory path");
        }
        std::vector<fs::path> out;
        for(auto& p: fs::directory_iterator( absDirPath ) )
        {
            out.push_back( p );
        }
        return out;
    }

    /**
     * @brief locateAll
     * @param relPath
     * @return
     *
     * Locates a list of all paths that match ROOT[i]/relPath
     * And returns the root path for which it exists in.
     */
    std::vector<fs::path> locateAll( fs::path const & relPath) const
    {
        std::vector<fs::path> all;
        for(auto const & p1 : roots )
        {
            auto abs_path = p1 / relPath;

            if( fs::exists(abs_path) )
            {
                all.push_back( p1 );
            }
        }
        return all;
    }

    /**
     * @brief listDirectoryUnion
     * @param dirPath
     * @return
     *
     * Given a relative path, lists all the files that exist in that
     * directory, but do not include duplicated file names.
     * For example if the root paths are: /tmp/A and /tmp/B
     * and the dir structure is
     *
     * /tmp/A/File1
     * /tmp/A/File2
     * /tmp/B/File1
     * /tmp/B/File3
     *
     * It will return: /tmp/A/File1, /tmp/A/File2, /tmp/B/File3
     *
     * /tmp/B/File1 will not be returned because it is shadowded
     * by a previous root path
     */
    std::vector<fs::path> listDirectoryUnion( fs::path const & relPath) const
    {
        // locate all the paths that match ROOT[i]/relPath
        auto validDirs = locateAll(relPath);

        std::unordered_set<std::string> found;
        std::vector<fs::path> unionFiles;

        for(auto const & p1 : validDirs)
        {
            auto n = p1.string().size();

            auto dirPath = p1/relPath;

            if( fs::is_directory(dirPath))
            {
                for(auto & p: fs::directory_iterator( dirPath ) )
                {
                    auto leaf = p.path().string().erase(0,n+1);
                    if( found.insert(leaf).second == true)
                    {
                        unionFiles.push_back( p );
                    }
                }
            }
        }
        return unionFiles;
    }

};


}


#endif
