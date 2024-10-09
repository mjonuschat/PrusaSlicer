#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace Slic3r::App::Render {

template <typename K, typename R>
class ResourceManager {
public:
    R* get_or_create(const K& name, const std::function<std::unique_ptr<R>()>& builder)
    {
        auto it = m_resources.find(name);
        if (it != m_resources.end())
            return it->second.get();
        auto ret = m_resources.emplace(name, builder());
        return ret.first->second.get();
    }

    void set(const K& name, R* resource)
    {
        m_resources[name] = std::make_unique<R>(resource);
    }


    bool release(const K& name) { return m_resources.erase(name) > 0; }
    void release_all() { m_resources.clear(); }

private:
    using ResourceMap = std::unordered_map<K, std::unique_ptr<R>>;

    ResourceMap m_resources;
};

}
