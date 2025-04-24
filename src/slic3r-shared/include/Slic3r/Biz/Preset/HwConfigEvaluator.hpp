#pragma once
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Biz/Expr/Eval.hpp"
#include "Slic3r/Domain/Preset/SourceLocatedExpr.hpp"

namespace Slic3r::Biz::Preset {

template <typename T>
concept Conditional = requires(T a)
{
    {a.condition} -> std::same_as<std::optional<Domain::Preset::SourceLocatedExpr>&>;
};

template <Conditional T>
class ConfigIterator;

template <typename T>
ConfigIterator<T> begin(const ConfigIterator<T>& it);
template <typename T>
ConfigIterator<T> end(const ConfigIterator<T>& it);

template <Conditional T>
class ConfigIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = const value_type*;
    using reference = const value_type&;

    ConfigIterator(const ConfigIterator&) = default;

    ConfigIterator& operator++()
    {
        ASSERT(is_valid());
        advance_filtered();
        return *this;
    }

    ConfigIterator operator++(int)
    {
        ASSERT(is_valid());
        ConfigIterator tmp = *this;
        advance_filtered();
        return tmp;
    }

    bool operator==(const ConfigIterator& other) const
    {
        return m_it == other.m_it;
    }

    bool operator!=(const ConfigIterator& other) const
    {
        return m_it != other.m_it;
    }

    reference operator*() const { return m_it->second; }
    pointer operator->() const { return &m_it->second; }

    bool is_valid() const { return m_it != m_container.end(); }
private:
    using Container = std::map<std::string, T>;
    friend class HwConfigEvaluator;
    friend ConfigIterator Slic3r::Biz::Preset::begin<>(const ConfigIterator& it);
    friend ConfigIterator Slic3r::Biz::Preset::end<>(const ConfigIterator& it);

    ConfigIterator(const Container& container, const Expr::Eval& eval, Expr::ValueMap&& values)
        : m_vars(std::move(values)), m_eval(eval), m_container(container), m_it(m_container.begin())
    {
        ensure_condition_met();
    }

    void reset()
    {
        m_it = m_container.begin();
        ensure_condition_met();
    }
    void invalidate() { m_it = m_container.end(); }

    void advance()
    {
        ASSERT(is_valid());
        ++m_it;
    }

    void advance_filtered()
    {
        advance();
        ensure_condition_met();
    }

    void ensure_condition_met()
    {
        for (;;) {
            if (!is_valid()) {
                return;
            }
            const auto& el = m_it->second;
            if (!el.condition.has_value()) {
                return;
            }
            const auto result = m_eval.eval(el.condition.value().value, m_vars);
            if (boost::get<bool>(result)) {
                return;
            }
            advance();
        }
    }
private:
    Expr::ValueMap m_vars;
    const Expr::Eval& m_eval;
    const Container& m_container;
    typename Container::const_iterator m_it;
};

template <typename T>
ConfigIterator<T> begin(const ConfigIterator<T>& it)
{
    ConfigIterator<T> ret = it;
    ret.reset();
    return ret;
}

template <typename T>
ConfigIterator<T> end(const ConfigIterator<T>& it)
{
    ConfigIterator<T> ret = it;
    ret.invalidate();
    return ret;
}

using HwToolConfigIterator = ConfigIterator<Slic3r::Domain::Preset::HwToolConfigDef>;
using HwFeederConfigIterator = ConfigIterator<Slic3r::Domain::Preset::HwFeederConfigDef>;

extern template ConfigIterator<Domain::Preset::HwToolConfigDef> begin<Domain::Preset::HwToolConfigDef>(const ConfigIterator<Domain::Preset::HwToolConfigDef>&);
extern template ConfigIterator<Domain::Preset::HwToolConfigDef> end<Domain::Preset::HwToolConfigDef>(const ConfigIterator<Domain::Preset::HwToolConfigDef>&);
extern template ConfigIterator<Domain::Preset::HwFeederConfigDef> begin<Domain::Preset::HwFeederConfigDef>(const ConfigIterator<Domain::Preset::HwFeederConfigDef>&);
extern template ConfigIterator<Domain::Preset::HwFeederConfigDef> end<Domain::Preset::HwFeederConfigDef>(const ConfigIterator<Domain::Preset::HwFeederConfigDef>&);


class HwConfigEvaluator
{
public:
    HwToolConfigIterator iterate_tools(const Domain::Preset::HwPrinterConfig& printer, const HwToolConfigIterator::Container& tools) const;
    HwFeederConfigIterator iterate_feeders(const Domain::Preset::HwPrinterConfig& printer, const Domain::Preset::HwToolConfig& tool, const HwFeederConfigIterator::Container& feeders) const;
    Domain::Preset::HwPrinterConfig create_printer_config(const Domain::Preset::HwPrinterConfigTemplate& templ, const Domain::Preset::VendorData& vendor_data) const;
private:
    Expr::Eval m_eval;
};

}
