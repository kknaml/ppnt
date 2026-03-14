export module ppnt.jr.env.br_env;

import std;
import ppnt.common;

export namespace ppnt::jr::env {

    struct NavigatorUAData {
        struct Brand {
            std::string brand{};
            std::string version{};
        };
        std::vector<Brand> brands{};
        bool mobile{};
        std::string platform{};
    };

    struct NavigatorClipboard {

    };

    struct NavigatorConnection {
        double downlink{7.75};
        std::string effective_type{"4g"};
        int rtt{100};
        bool save_data{false};
    };

    struct NavigatorData {
        std::string user_agent{};
        std::string app_code_name{};
        std::string app_name{};
        std::string app_version{};
        NavigatorUAData user_agent_data{};

        NavigatorClipboard clipboard{};
        bool cookie_enabled{true};
        int deviceMemory{16};
        int hardwareConcurrency{16};
        std::string language{"en-US"};
        std::vector<std::string> languages{"en-US"};

        std::string platform{};
        std::string product{};
        std::string product_stub{"20030107"};
        std::string vendor{};
        std::string vendor_stub{};
        bool web_driver{};

    };

    struct BrEnv {
        NavigatorData navigator_data{};
    };
}