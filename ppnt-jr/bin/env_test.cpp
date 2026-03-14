import std;
import ppnt.common;
import yyjson;

auto main() -> int {
    std::string body = R"({"status": 200, "message": "OK"})";
    auto *document = yyjson_read(body.c_str(), body.length(), 0);
    auto *root = yyjson_doc_get_root(document);

    auto *status_val = yyjson_obj_get(root, "status");
    auto status_code = yyjson_get_int(status_val);

    ppnt::log::info({"status: {}"}, status_code);

    yyjson_doc_free(document);
}
