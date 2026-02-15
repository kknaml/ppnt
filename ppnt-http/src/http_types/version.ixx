export module ppnt.http.http_types:version;

import std;
import ppnt.common;


export namespace ppnt::http {

    enum class Version {
        HTTP_10,
        HTTP_11,
        HTTP_2,
        HTTP_3,
    };
}
