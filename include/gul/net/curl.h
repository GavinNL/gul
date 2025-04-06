#ifndef GUL_CURL_H
#define GUL_CURL_H

#include <string.h>
#include "../uri.h"


// C++17 includes the <filesystem> library, but
// unfortunately gcc7 does not have a finalized version of it
// it is in the <experimental/filesystem lib
// this section includes the proper header
// depending on whether the header exists and
// includes that. It also sets the
// nf namespace
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
 * @brief The HTTP2 class
 *
 * A helper class for curl to get files from the internet
 */
struct CURL
{
    std::string CACHE_PATH = (fs::temp_directory_path() / fs::path("gul_wget")).string();
    std::string CURL_PATH =
                #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
                    "C:\\Windows\\System32\\curl.exe";
                #else
                    "/usr/bin/curl";
                #endif

    std::string CURL_ADDITIONAL_FLAGS = " -s ";


    /**
     * @brief get_or_cached
     * @param _url
     * @return
     *
     * Gets the path to the local cached file if it exists,
     * or downloads it if it doesn't exist
     */
    fs::path get_cached_or_download(gul::uri const & _url) const
    {
        auto P = cache_file(_url);
        if(fs::exists(P))
        {
            return P;
        }
        return get(_url);
    }
    /**
     * @brief get
     * @param Pd
     * @return
     *
     * Calls the curl command to get the file from _url
     * and returns the file path of where it's stored.
     *
     * This is a blocking call.
     */
    fs::path get(gul::uri const & _url) const
    {
        auto P = cache_file(_url);

        fs::create_directories( P.parent_path() );

        std::string cmd = CURL_PATH + " "
                          + CURL_ADDITIONAL_FLAGS +
                          + " -o " + P.string() + " "
                          + _url.toString();

        std::system(cmd.c_str());
        return P;
    }

    /**
     * @brief cache_file
     * @param _uri
     * @return
     *
     * Returns the path to where this uri will be stored
     */
    fs::path cache_file(gul::uri const & _uri) const
    {
        std::hash<std::string> H;
        std::string urlPath = _uri.toString();
        auto h = H(urlPath);

        auto fn = fs::path(_uri.path).filename();
        if(fn.empty())
        {
            fn = _uri.host;
        }

        return fs::path(CACHE_PATH) / _uri.host / std::to_string(h) / fn;
    }

};
}

#endif
