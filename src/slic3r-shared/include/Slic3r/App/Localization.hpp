#pragma once

#include "I18N/Translation.hpp"
#include "ILanguageChangedListener.hpp"
#include "Slic3r/Biz/Platform/ListenerList.hpp"

// ! Singleton Class  

namespace Slic3r::App {

class Localization {

public:
    Localization(const Localization& obj) = delete;

    static Localization& instance();

    bool                set_language(const std::string& language);
    bool                is_alternative_language() const;
    const std::string   active_language() const;
    const std::vector<LanguageShortInfo>& languages() const;

    void add_language_changed_listener(ILanguageChangedListener* l);
    void remove_language_changed_listener(ILanguageChangedListener* l);

private:
    Localization();

private:
    Translations                                m_translations;
    Biz::ListenerList<ILanguageChangedListener> m_language_changed_listener;
};

Localization& localization();

} // Slic3r::App

