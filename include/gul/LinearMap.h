#ifndef GUL_LINEAR_MAP_H
#define GUL_LINEAR_MAP_H

#include <limits>
#include <stdexcept>
#include <vector>
#include <unordered_map>

namespace gul
{

/**
 * @brief The LinearMap class
 *
 * A linear map works like an unordered_map
 * except the data is stored in a linear array.
 *
 *
 */
template<typename Key_, typename Value_>
class LinearMap
{
public:
    using key_type = Key_;
    using value_type = Value_;
    using array_type = std::vector<value_type>;
    static constexpr size_t npos = std::numeric_limits<size_t>::max();

    value_type& at(key_type const &k)
    {
        return m_data[m_keyToIndex.at(k)];
    }

    value_type const & at(key_type const &k) const
    {
        return m_data[m_keyToIndex.at(k)];
    }

    value_type& operator[](key_type const &k)
    {
        auto it = findIndex(k);
        if(it != npos)
        {
            return m_data[it];
        }
        else
        {
            m_keyToIndex[k] = m_data.size();
            return m_data.emplace_back();
        }
    }

    /**
     * @brief findIndex
     * @param k
     * @return
     *
     * returns the index at which the key was stored
     * returns npos if not found
     */
    size_t findIndex(key_type const & k) const
    {
        auto it = m_keyToIndex.find(k);
        if(it == m_keyToIndex.end())
        {
            return npos;
        }
        return it->second;
    }

    /**
     * @brief insert
     * @param k
     * @param v
     * @return
     *
     * returns the index into the array where the
     * value was stored
     */
    size_t insert(key_type k, value_type const & v)
    {
        auto it = m_keyToIndex.find(k);

        if(it == m_keyToIndex.end())
        {
            if(!m_freeIndices.empty())
            {
                auto i = m_freeIndices.back();
                m_freeIndices.pop_back();
                m_keyToIndex[k] = i;
                m_data[i] = v;
                return i;
            }
            m_data.push_back(v);
            m_keyToIndex[k] = m_data.size()-1;
            return m_data.size()-1;
        }
        else
        {
            m_data[it->second] = v;
            return it->second;
        }
    }

    /**
     * @brief erase
     * @param k
     * @return
     *
     * Erases a key from the map, this does not change
     * the capacity()
     */
    bool erase(key_type const & k)
    {
        auto it = m_keyToIndex.find(k);
        if(it == m_keyToIndex.end())
        {
            return false;
        }
        auto i = it->second;
        m_data[it->second] = {};
        m_keyToIndex.erase(it);
        m_freeIndices.push_back(i);
        return true;
    }

    /**
     * @brief size
     * @return
     *
     * Returns the total number of keys
     */
    size_t size() const
    {
        return m_keyToIndex.size();
    }

    /**
     * @brief capacity
     * @return
     *
     * Returns the total number of elements in the array.
     * This value does not change erasing elements.
     */
    size_t capacity() const
    {
        return m_data.size();
    }

    /**
     * @brief array
     * @return
     *
     * Returns the underlying array data type.
     */
    array_type const & array() const
    {
        return m_data;
    }
protected:
    array_type                                m_data;
    std::unordered_map<key_type, std::size_t> m_keyToIndex;
    std::vector<size_t>                       m_freeIndices;
};

}

#endif
