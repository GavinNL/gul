#ifndef GUL_VIRTUAL_FILESYSTEM_H
#define GUL_VIRTUAL_FILESYSTEM_H

#include <filesystem>
#include <map>
#include<fstream>
#include <iostream>
#include <vector>

namespace gul
{

struct HostPath
{
    std::vector<std::filesystem::path> pathOnHost;

    HostPath() = default;

    HostPath(std::filesystem::path const &p)
    {
        append(p);
    }

    HostPath(std::vector<std::filesystem::path> const &p)
    {
        for(auto & x : p)
            append(x);
    }

    void append(std::filesystem::path const & p)
    {
        if( !std::filesystem::exists(p) || !std::filesystem::is_directory(p))
        {
            return;//throw std::runtime_error("The Host Path does not exist or is not a folder");
        }
        pathOnHost.push_back(p);
    }


    bool exists(std::filesystem::path relPath) const
    {
        return !host_path(relPath).empty();
    }

    std::filesystem::path host_path(std::filesystem::path relPath) const
    {
        for(auto it = pathOnHost.rbegin(); it!=pathOnHost.rend(); ++it)
        {
            auto p = *it / relPath.relative_path();
            if(std::filesystem::exists(p))
            {
                return p;
            }
        }
        return {};
    }

    template<typename callable_t>
    void for_each(callable_t && c) const
    {
        for(auto it = pathOnHost.rbegin(); it!=pathOnHost.rend(); ++it)
        {
            auto & b = *it;
            for(auto & A : std::filesystem::directory_iterator(b))
            {
                auto actualPath = A.path().lexically_relative(b);
                c(actualPath);
            }
        }
    }
};

struct VirtualFileSystem
{
    using vfs_path_type  = std::filesystem::path;
    using host_path_type = std::filesystem::path;

    VirtualFileSystem()
    {
       // _rootMounts["/"] = {};
    }
    void mount(vfs_path_type mountPoint, HostPath p)
    {
        if(_rootMounts.count(mountPoint))
            throw std::runtime_error("Mount point already exists");
        _rootMounts[mountPoint] = p;
    }

    void umount(vfs_path_type mountPoint)
    {
        _rootMounts.erase(mountPoint);
    }

    /**
     * @brief exists
     * @param vfsPath
     * @return
     *
     * Returns true if the file exists in the VFS (and on the host)
     */
    bool exists(vfs_path_type const &vfsPath) const
    {
        auto strPath = vfsPath.generic_string();

        for(auto & [a,b] : _rootMounts)
        {
            auto rootStr = a.generic_string();
            if(rootStr == strPath)
                return true;
            auto n = strPath.find(rootStr);
            if(n==0)
            {
                //fs::path("/resources/.bashrc").lexically_relative("/resources");
                auto relPath = vfsPath.lexically_relative(a);
                if(b.exists(relPath))
                    return true;
            }
        }
        return false;
    }

    static bool is_base_of(vfs_path_type const & base, vfs_path_type const & full)
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
    template<typename callable_t>
    void for_each(vfs_path_type const &vfsPath, callable_t && CC) const
    {
        assert(vfsPath.is_absolute());
        //std::cout << "Listing dir: " << vfsPath << std::endl;

        if(is_mount(vfsPath))
        {
            // if this is a mount point
            // Its possible that the

            // eg: mnt = /A/B
            //     vfsPath = /
            // vfsPath is a base of mnt
            for(auto & [mnt, M] : _rootMounts)
            {
                if(mnt == vfsPath) continue;
                if(is_base_of(vfsPath, mnt))
                {
                    auto file = mnt.lexically_relative(vfsPath);
                    if(!file.has_parent_path())
                        CC(file);
                }
            }
            auto & M  = _rootMounts.at(vfsPath);
            M.for_each([&](auto const & file)
            {
                CC(file);
            });
        }
        else
        {
            // not a mount point, so
        }
        return;
        for(auto & [a,b] : _rootMounts)
        {
            auto relPath  = a.lexically_relative(vfsPath);
            auto relPath2 = vfsPath.lexically_relative(a);

            if(relPath == ".") // this is a mount point
            {
                // loop through all items in the mount point's host
                // directory
                std::cout << "Is Mount point: " << vfsPath << std::endl;
                if(!b.pathOnHost.empty())
                {
                    // for each in side the mount point
                    b.for_each([&CC](auto const & _file)
                               {
                                   CC(_file);
                               });
                    // We now have to check if any of the current
                    // mount points is a base of vfs
                    for(auto & [A,B] : _rootMounts)
                    {
                        if(is_base_of(vfsPath, A))
                        {
                            auto lr = A.lexically_relative(vfsPath);
                            if(lr != ".")
                                CC(lr);
                        }
                    }
                }
                else
                {
                    for(auto & [A,B] : _rootMounts)
                    {
                        if(is_base_of(a, A))
                        {
                            auto S2 = A.relative_path();
                            if(!S2.empty())
                                CC(S2);
                        }
                    }
                }
            }
            else
            {
                if(is_base_of(a, vfsPath))
                {
                    auto host_path = b.host_path(relPath2);
                    if(std::filesystem::is_directory(host_path))
                    {
                        for(auto & A : std::filesystem::directory_iterator(host_path))
                        {
                            auto actualPath = A.path().lexically_relative(host_path);
                            CC(actualPath);
                        }

                    }
                }
            }
        }
    }

    void listDir(vfs_path_type const &vfsPath) const
    {
        std::cout << "Listing dir: " << vfsPath << std::endl;
        for_each(vfsPath, [](auto const & rel)
        {
            std::cout << rel << std::endl;
        });
    }

    bool is_mount(vfs_path_type const & vfsPath) const
    {
        for(auto & [a,b] : _rootMounts)
        {
            if(a == vfsPath)
                return true;
        }
        return false;
    }

    bool is_directory(vfs_path_type const & vfsPath) const
    {
        if(_rootMounts.count(vfsPath))
            return true;
        return std::filesystem::is_directory(host_path(vfsPath));
    }
    bool is_regular_file(vfs_path_type const & vfsPath) const
    {
        if(_rootMounts.count(vfsPath))
            return false;
        return std::filesystem::is_regular_file(host_path(vfsPath));
    }

    /**
     * @brief resolvedPath
     * @param vfsPath
     * @return
     *
     * Given a path in the VFS, return the actual path on the
     * host file system
     */
    host_path_type host_path(vfs_path_type vfsPath) const
    {
        auto strPath = vfsPath.generic_string();

        for(auto & [a,b] : _rootMounts)
        {
            auto rootStr = a.generic_string();
            auto n = strPath.find(rootStr);

            if(n==0)
            {
                //fs::path("/resources/.bashrc").lexically_relative("/resources");
                auto relPath = vfsPath.lexically_relative(a);
                auto real_path = b.host_path(relPath);
                if(!real_path.empty())
                    return real_path;
            }
        }
        return {};
    }

    auto const& mount_points() const
    {
        return _rootMounts;
    }


protected:
    std::map<vfs_path_type, HostPath> _rootMounts;
};
class ivfstream : public std::ifstream
{
public:
    ivfstream(VirtualFileSystem * P, VirtualFileSystem::vfs_path_type path)
        : std::ifstream(P->host_path(path), ios_base::in)
    {
    }
};

class ovfstream : public std::ofstream
{
public:
    ovfstream(VirtualFileSystem * P, VirtualFileSystem::vfs_path_type path)
        : std::ofstream(P->host_path(path), ios_base::in)
    {
    }
};

}


#endif
