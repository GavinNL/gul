#ifndef GUL_VIRTUAL_FILESYSTEM_H
#define GUL_VIRTUAL_FILESYSTEM_H

#include <variant>
#include <filesystem>
#include <map>
#include<fstream>
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

    template<typename callable_t>
    void for_each3(vfs_path_type const & p, callable_t && CC) const
    {
        auto it = desc.find(p);
        while( it != desc.end())
        {
            if(std::holds_alternative<Mount>(it->second))
            {
                // this is a mount point, so loop through all the files
                // on the host
                auto & M = std::get<Mount>(it->second);
                for(auto & A : std::filesystem::directory_iterator(M.host))
                {
                    CC(A.path().lexically_relative(M.host));
                }

                // But there might also be another mount point
                // at  p / mnt
                //
            }
            else if(std::holds_alternative<Directory>(it->second))
            {
                (void)is_base_of;
                while(it != desc.end())
                {
                    auto name = it->first.lexically_relative(p);
                    auto s = std::distance(name.begin(), name.end());
                    if(s == 1 && name != ".")
                        CC(name);
                        //std::cout << name << "  " << s << std::endl;
                    ++it;
                }
                return;
            }
            ++it;
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

        // pf doesn't have an explicit location in the file descriptors
        // map
        // so that means it is likely in some folder within a mounted
        // filesystem
        //
        // So work backwards from the filename until we find
        // a mount point:
        // for example:
        // Given pf = /path/to/some/file.txt
        //
        // and a mount point "/path -> /home/bob" exists
        // then search if /home/bob/to/some/file.txt exists
        // in the host file system and

        // then left == path/to/some/file.txt
        vfs_path_type left = pf.relative_path();
        // right is empty
        vfs_path_type right;

        while(!left.empty())
        {
            // if 'left' is a mount point to the host
            // then check if right exists the filesystem
            if(is_mount(vfs_path_type("/") / left))
            {
                return std::filesystem::exists(std::get<Mount>(desc.at(vfs_path_type("/") / left)).host / right);
            }

            // remove the filename from 'left' and
            // append it to the front of 'right'

            // left = path/to/some
            // right = file.txt
            right = right.empty() ? left.filename() : (left.filename() / right);
            left = left.parent_path();
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
    void print()
    {
        for(auto &[a,b] : desc)
        {
            std::cout << a << std::endl;
        }
    }

    std::map<vfs_path_type, FileDescriptor> desc;
};

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
    template<typename callable_t>
    void for_each(std::filesystem::path const & p, callable_t && c) const
    {
        for(auto it = pathOnHost.rbegin(); it!=pathOnHost.rend(); ++it)
        {
            auto & b = *it;
            for(auto & A : std::filesystem::directory_iterator(b / p))
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
        //mount("/", {});
       // _rootMounts["/"] = {};
    }
    void mount(vfs_path_type mountPoint, HostPath p)
    {
        if(_rootMounts.count(mountPoint))
            throw std::runtime_error("Mount point already exists");

        if(_rootMounts.size() != 0 && !exists(mountPoint.parent_path()))
            throw std::runtime_error("FOlder does not exist");

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



    template<typename callable_t>
    void for_each(vfs_path_type const &vfsPath, callable_t && CC) const
    {
        auto a = vfsPath;
        //while(!vfsPath.empty())
        {
            if(is_mount(a))
            {
                auto & M = _rootMounts.at(a);
                if(M.pathOnHost.empty())
                {
                    // an empty mount
                    for(auto & [mnt, M2] : _rootMounts)
                    {
                        if(mnt.parent_path() == vfsPath)
                        {
                            CC(mnt.filename());
                        }
                    }
                }
                M.for_each(CC);
                return;
            }
            else
            {
                auto h = host_path(vfsPath);
                for(auto & A : std::filesystem::directory_iterator(h))
                {
                    auto actualPath = A.path().filename();//lexically_relative(h);
                    CC(actualPath);
                }
            }
        }
    }
    template<typename callable_t>
    void for_each1(vfs_path_type const &vfsPath, callable_t && CC) const
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
            // not a mount point, so check all other mount points
            for(auto & [mnt, M] : _rootMounts)
            {
                if(is_base_of(mnt, vfsPath ))
                {

                    if(M.exists(vfsPath.lexically_relative(mnt)))
                    {
                        //std::cout << mnt << "   " << std::endl;;
                        //std::cout << "   " <<  vfsPath.lexically_relative(mnt) << std::endl;
                        for(auto it = M.pathOnHost.rbegin(); it!=M.pathOnHost.rend(); ++it)
                        {
                            auto & b = *it;
                            for(auto & A : std::filesystem::directory_iterator(b))
                            {
                                auto actualPath = A.path();
                                CC(actualPath);
                            }
                        }
                        //M.for_each([&CC](auto const & vv)
                        //           {
                        //               CC(vv);
                        //           });
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
