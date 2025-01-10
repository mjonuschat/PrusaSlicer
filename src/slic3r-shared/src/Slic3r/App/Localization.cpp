#include "Slic3r/App/Localization.hpp"

#include <libslic3r/Utils.hpp>

namespace Slic3r::App {

Localization& Localization::instance()
{
    static Localization inst;
    return inst;
}

Localization::Localization()
{
    m_translations.init_translations(boost::filesystem::path(localization_dir()));
}

void Localization::add_language_changed_listener(ILanguageChangedListener* l)
{
    m_language_changed_listener.add(l);
}

void Localization::remove_language_changed_listener(ILanguageChangedListener* l)
{
    m_language_changed_listener.remove(l);
}

Localization& localization()
{
    return Localization::instance();
}

bool Localization::set_language(const std::string& language)
{
    if (m_translations.set_best_translation_for_language(language)) {
        m_language_changed_listener.invoke([](auto * l) {
            l->on_language_changed(); });
        return true;
    }
    return false;
}

bool Localization::is_alternative_language() const
{
    return m_translations.is_alternative_language();
}

const std::string Localization::active_language() const
{
    return m_translations.active_language();
}

const std::vector<LanguageShortInfo>& Localization::languages() const
{
    return m_translations.languages();
}

} //namespace Slic3r::App