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

void parse_all_documents(const Parser& parser, const std::function<void(const Document&)>& parse_doc)
{
    while (true) {
        Document doc{fy_parse_load_document(parser.get())};
        if (!doc)
            return;
        parse_doc(doc);
    }
}

Document parse_file(const char* file_name)
{
    Parser parser{create_parser()};
    fy_parser_set_input_file(parser.get(), file_name);
    return Document{fy_parse_load_document(parser.get())};
}

Document parse_string(const char* yaml)
{
    Parser parser{create_parser()};
    fy_parser_set_string(parser.get(), yaml, strlen(yaml));
    return Document{fy_parse_load_document(parser.get())};
}


void parse_all_documents_in_file(const char* file_name, const std::function<void(const Document&)>& parse_doc)
{

    Parser parser{create_parser()};
    fy_parser_set_input_file(parser.get(), file_name);
    parse_all_documents(parser, parse_doc);
}

void parse_all_documents_in_string(const char* yaml, const std::function<void(const Document&)>& parse_doc)
{
    Parser parser{create_parser()};
    fy_parser_set_string(parser.get(), yaml, strlen(yaml));
    parse_all_documents(parser, parse_doc);
}

} // namespace Yaml
