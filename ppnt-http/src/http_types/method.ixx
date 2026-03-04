export module ppnt.http.http_types:method;

import std;
import ppnt.common;

export namespace ppnt::http {

    class Method : public NonInstance {
    public:

        static constexpr std::string GET = "GET";
        static constexpr std::string POST = "POST";
        static constexpr std::string CONNECT = "CONNECT";
        static constexpr std::string PUT = "PUT";
        static constexpr std::string DELETE = "DELETE";
        static constexpr std::string OPTIONS = "OPTIONS";
        static constexpr std::string PATCH = "PATCH";
        static constexpr std::string HEAD = "HEAD";
    };
}