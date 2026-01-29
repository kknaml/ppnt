
import std;
import ppnt.log;
import ppnt.traits;
import ppnt.match;
import ppnt.err;


using namespace ppnt;

struct Boo {

    auto to_string() const -> const char * {
        return "boo";
    }
};

template<ppnt::FixedString S>
void func() {
    log::info({"Func name: {}"}, S.c_str());
}


auto main() -> int {

    log::info({"dsada: {}"}, 456);
    log::error({"wuwuwu: {}"}, Boo{});

    auto ec = std::make_error_code(std::errc::connection_refused);
    auto err = Error(ec);

    ec = std::make_error_code(std::errc::invalid_argument);
    auto err2 = Error(ec, "hahaha");
    err2.with_cause(err);

    log::warn({"err: {}"}, err2);

    func<"Huaq">();
    return 0;
}

