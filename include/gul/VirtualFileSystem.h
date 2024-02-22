#ifndef GUL_VIRTUAL_FILESYSTEM_H
#define GUL_VIRTUAL_FILESYSTEM_H

#include <variant>
#include <filesystem>
#include <map>
#include <iostream>
#include <vector>

namespace gul
{

static bool is_base_of(std::filesystem::path const & base, std::filesystem::path const & full)
{
    //(void)base;
    //(void)full;
    //auto relPath = base.lexically_relative(full);
    auto relPath2 = full.lexically_relative(base);
    //std::cout << relPath << std::endl;
    //std::cout << relPath2 << std::endl;
    if(!relPath2.empty())
    {
        if(*relPath2.begin() == "..")
        {
            return false;
        }
    }
    return true;
}

struct File
{
    std::vector<uint8_t> data;
};

struct Directory
{
    std::vector<std::shared_ptr<void>> descriptors;
};

struct Mount
{
    std::filesystem::path host;
};

using FileDescriptor = std::variant<File, Directory, Mount>;


struct VFS
{
    using vfs_path_type  = std::filesystem::path;
    using host_path_type = std::filesystem::path;

    VFS()
    {
        desc["/"] = Directory{};
    }

    void mkdir(vfs_path_type const pf)
    {
        if(is_directory(pf.parent_path()) )
        {
            desc[pf] = Directory{};
        }
        else
        {
            mkdir(pf.parent_path());
            mkdir(pf);
        }
    }

    void mount(vfs_path_type const & pf, host_path_type const & host)
    {
        Mount m{host};
        desc[pf] = m;
    }
    void unmount(vfs_path_type const & pf)
    {
        desc.erase(pf);
    }


    void list(vfs_path_type const & p) const
    {
        for_each(p, [](auto v)
        {
            std::cout << v << std::endl;
        });
    }

    template<typename callable_t>
    void for_each(vfs_path_type const & p, callable_t && CC) const
    {
        auto [mnt, stem] = splitMount(p);

        if(mnt.empty())
        {
            // The path, p, does not contain a mount point
            // check if p exists in the descriptors
            auto it = desc.find(p);
            if(it != desc.end())
            {
                ++it;
                while(it != desc.end())
                {
                    if(is_base_of(p, it->first))
                    {
                        auto removeBase = it->first.lexically_relative(p);
                        if(1==std::distance(removeBase.begin(), removeBase.end()))
                        {
                            CC(removeBase);
                        }
                        //CC(it->first);
                    }
                    ++it;
                }
                // we found a descriptor with that
            }
            return;
        }
        auto & M = desc.at(mnt);

        if(std::holds_alternative<Mount>(M))
        {
            auto & host = std::get<Mount>(M).host;
            for(auto & A : std::filesystem::directory_iterator(host / stem))
            {
                CC(A.path().lexically_relative(host / stem));
            }

        }


    }

    /**
     * @brief exists
     * @param pf
     * @return
     *
     * Returns true if the file exists in the VFS.
     */
    bool exists(vfs_path_type const & pf) const
    {
        assert(pf.is_absolute());

        auto it = desc.find(pf);
        // exists in the Virtual files
        if(it != desc.end())
        {
            return true;
        }

        auto [mnt, stem] = splitMount(pf);
        if(!mnt.empty())
        {
            auto & M = std::get<Mount>(desc.at(mnt));
            return std::filesystem::exists( M.host / stem);
        }
        return false;
    }

    bool is_mount(vfs_path_type const & pf) const
    {
        auto it = desc.find(pf);
        if(it == desc.end())
            return false;
        return std::holds_alternative<Mount>(it->second);
    }

    std::pair<vfs_path_type, vfs_path_type> splitMount(vfs_path_type const & pf) const
    {
        vfs_path_type left = pf.relative_path();
        vfs_path_type right;
        while(!left.empty())
        {
            // if 'left' is a mount point to the host
            // then check if right exists the filesystem
            if(is_mount(vfs_path_type("/") / left))
            {
                return {vfs_path_type("/") / left, right};
            }

            // remove the filename from 'left' and
            // append it to the front of 'right'

            // left = path/to/some
            // right = file.txt
            right = right.empty() ? left.filename() : (left.filename() / right);
            left = left.parent_path();
        }
        return {};
    }

    bool is_directory(vfs_path_type const & pf) const
    {
        assert(pf.is_absolute());

        auto it = desc.find(pf);
        // exists in the Virtual files
        if(it != desc.end())
        {
            return std::holds_alternative<Directory>(it->second)
                   || std::holds_alternative<Mount>(it->second);
        }

        auto [mnt, stem] = splitMount(pf);
        if(!mnt.empty())
        {
            auto & M = std::get<Mount>(desc.at(mnt));
            return std::filesystem::is_directory( M.host / stem);
        }
        return false;
    }

    bool is_file(vfs_path_type const & pf) const
    {
        assert(pf.is_absolute());

        auto it = desc.find(pf);
        // exists in the Virtual files
        if(it != desc.end())
        {
            return std::holds_alternative<File>(it->second);
        }
        auto [mnt, stem] = splitMount(pf);
        if(!mnt.empty())
        {
            auto & M = std::get<Mount>(desc.at(mnt));
            return std::filesystem::is_regular_file(M.host / stem);
        }
        return false;
    }

    uintmax_t file_size(vfs_path_type const & pf) const
    {
        assert(pf.is_absolute());

        auto it = desc.find(pf);
        // exists in the Virtual files
        if(it != desc.end())
        {
            if(std::holds_alternative<File>(it->second))
            {
                return std::get<File>(it->second).data.size();
            }
        }

        auto [mnt, stem] = splitMount(pf);
        if(!mnt.empty())
        {
            auto & M = std::get<Mount>(desc.at(mnt));
            return std::filesystem::file_size(M.host / stem);
        }
        return 0;
    }

    void print()
    {
        for(auto &[a,b] : desc)
        {
            std::cout << a << std::endl;
        }
    }

    host_path_type host_path(vfs_path_type const pf) const
    {
        assert(pf.is_absolute());

        auto it = desc.find(pf);
        // exists in the Virtual files
        if(it != desc.end())
        {
            return {};
            //if(std::holds_alternative<File>(it->second))
            //{
            //    auto & F = std::get<File>(it->second);
            //    std::istringstream() F.data
            //}
        }
        auto [mnt, stem] = splitMount(pf);
        if(!mnt.empty())
        {
            auto & M = std::get<Mount>(desc.at(mnt));
            return M.host / stem;
        }
        return {};
    }
    std::map<vfs_path_type, FileDescriptor> desc;
};

}


#endif
