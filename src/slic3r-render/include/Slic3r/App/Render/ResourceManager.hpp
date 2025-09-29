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
        auto ret = m_resources.emplace(name, std::move(builder()));
        return ret.first->second.get();
    }

    const R* get(const K& name) const
    {
        auto it = m_resources.find(name);
        return it != m_resources.end() ? it->second.get() : nullptr;
    }

    R* get(const K& name)
    {
        auto it = m_resources.find(name);
        return it != m_resources.end() ? it->second.get() : nullptr;
    }

    void set(const K& name, R* resource)
    {
        m_resources[name] = std::make_unique<R>(resource);
    }

    void set(const K& name, std::unique_ptr<R>&& resource)
    {
        m_resources[name] = std::move(resource);
    }

    bool release(const K& name)
    {
        return m_resources.erase(name) > 0;
    }

    void release_all()
    {
        m_resources.clear();
    }

    size_t release_if(std::function<bool(const K& name, const R& resource)> predicate)
    {
        size_t count = 0;
        std::erase_if(m_resources, [&](const auto& item) {
            bool res = predicate(item.first, *item.second);
            if (res)
                ++count;
            return res;
        });
        return count;
    }

    bool is_empty() const { return m_resources.empty(); }

private:
    using ResourceMap = std::unordered_map<K, std::unique_ptr<R>>;

    ResourceMap m_resources;
};

}
