///|/ Copyright (c) Prusa Research 2023 Tomáš Mészáros @tamasmeszaros, Pavel Mikuš @Godrak
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <set>
#include <memory>

#include "SL1.hpp"
#include "SL1_SVG.hpp"
#include "libslic3r/I18N.hpp"
#include "SLAArchiveFormatRegistry.hpp"
#include "libslic3r/Format/SLAArchiveReader.hpp"
#include "libslic3r/libslic3r.h"

namespace Slic3r {

class Registry {
    static std::unique_ptr<Registry> registry;

    std::set<ArchiveEntry> entries;

    Registry ()
    {
        entries = {
            {
                "SL1",                      // id
                L("SL1 archive"),    // description
                "sl1",                      // main extension
                {"sl1s", "zip"},            // extension aliases

                // Reader factory
                [] (const std::string &fname, SLAImportQuality quality, const ProgrFn &progr) {
                    return std::make_unique<SL1Reader>(fname, quality, progr);
                }
            }/*,
            {
                "SL1SVG",
                L("SL1 SVG archive"),
                "sl1_svg",
                {"zip"},
                [] (const auto &cfg) { return std::make_unique<SL1_SVGArchive>(cfg); },
                [] (const std::string &fname, SLAImportQuality quality, const ProgrFn &progr) {
                    return std::make_unique<SL1_SVGReader>(fname, quality, progr);
                }
            },
            */
        };
    }

public:

    static const Registry& get_instance()
    {
        if (!registry)
            registry.reset(new Registry());

        return *registry;
    }

    static const std::set<ArchiveEntry>& get()
    {
        return get_instance().entries;
    }
};

std::unique_ptr<Registry> Registry::registry = nullptr;

const std::set<ArchiveEntry>& registered_sla_archives()
{
    return Registry::get();
}

std::vector<std::string> get_extensions(const ArchiveEntry &entry)
{
    auto ret = reserve_vector<std::string>(entry.ext_aliases.size() + 1);

    ret.emplace_back(entry.ext);
    for (const char *alias : entry.ext_aliases)
        ret.emplace_back(alias);

    return ret;
}

ArchiveReaderFactory get_reader_factory(const char *formatid)
{

    ArchiveReaderFactory ret;
    auto entry = Registry::get().find(ArchiveEntry{formatid});
    if (entry != Registry::get().end())
        ret = entry->rdfactoryfn;

    return ret;
}

const char *get_default_extension(const char *formatid)
{
    static constexpr const char *Empty = "";

    const char * ret = Empty;

    auto entry = Registry::get().find(ArchiveEntry{formatid});
    if (entry != Registry::get().end())
        ret = entry->ext;

    return ret;
}

const ArchiveEntry * get_archive_entry(const char *formatid)
{
    const ArchiveEntry *ret = nullptr;

    auto entry = Registry::get().find(ArchiveEntry{formatid});
    if (entry != Registry::get().end())
        ret = &(*entry);

    return ret;
}

} // namespace Slic3r::sla
