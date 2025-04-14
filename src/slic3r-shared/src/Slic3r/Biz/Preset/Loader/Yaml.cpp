
#include "Yaml.hpp"

namespace Yaml {
Parser create_parser()
{
    fy_parse_cfg cfg = {
        .search_path = "",
        .flags = FYPCF_QUIET,
        .userdata = nullptr,
        .diag = nullptr,
    };
    return Parser{fy_parser_create(&cfg)};
}

void parse_all_documents(const Parser& parser, const std::function<void(const Document&)>& parse_doc, const char* file)
{
    while (true) {
        Document doc{DocumentPtr{fy_parse_load_document(parser.get())}, file};
        if (!doc.doc)
            return;
        parse_doc(doc);
    }
}

Document parse_file(const char* file_name)
{
    Parser parser{create_parser()};
    fy_parser_set_input_file(parser.get(), file_name);
    return Document{DocumentPtr{fy_parse_load_document(parser.get())}, file_name};
}

Document parse_string(std::string_view yaml)
{
    Parser parser{create_parser()};
    fy_parser_set_string(parser.get(), yaml.data(), yaml.length());
    return Document{DocumentPtr{fy_parse_load_document(parser.get())}, "<string>"};
}

void parse_all_documents_in_file(const char* file_name, const std::function<void(const Document&)>& parse_doc)
{

    Parser parser{create_parser()};
    fy_parser_set_input_file(parser.get(), file_name);
    parse_all_documents(parser, parse_doc, file_name);
}

void parse_all_documents_in_string(std::string_view yaml, const std::function<void(const Document&)>& parse_doc)
{
    Parser parser{create_parser()};
    fy_parser_set_string(parser.get(), yaml.data(), yaml.length());
    parse_all_documents(parser, parse_doc, "<string>");
}

} // namespace Yaml
