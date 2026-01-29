module ppnt.log;

import std;

namespace ppnt::log {
    
    static auto global_level = Level::Debug;

    auto set_level(Level level) -> void {
        global_level = level;
    }

    auto get_level() -> Level {
        return global_level;
    }
}
