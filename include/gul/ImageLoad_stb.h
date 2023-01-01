#ifndef GUL_STB_IMAGE_LOAD_H
#define GUL_STB_IMAGE_LOAD_H

#include <stb_image.h>
#include <gul/Image.h>
#include <cstring>

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
inline gul::Image loadImage(fs::path const & p, int desiredChannels=4)
{
    int x,y,n;
    unsigned char *data = stbi_load(p.c_str(), &x, &y, &n, desiredChannels);

    gul::Image I( static_cast<uint32_t>(x), static_cast<uint32_t>(y), static_cast<uint32_t>(desiredChannels) );
    std::memcpy(I.data(), data, I.byteSize());

    stbi_image_free(data);
    return I;
}

inline gul::Image loadImage(void const* data, int len, int desiredChannels=4)
{
    int x,y,channels;

    auto imgData = stbi_load_from_memory( static_cast<stbi_uc const*>(data), len, &x, &y, &channels, desiredChannels);

    gul::Image I( static_cast<uint32_t>(x), static_cast<uint32_t>(y), static_cast<uint32_t>(desiredChannels) );
    std::memcpy(I.data(), imgData, I.byteSize());

    stbi_image_free(imgData);
    return I;
}

}


#endif
