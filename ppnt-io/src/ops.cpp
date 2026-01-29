module ppnt.io.ops;

import ppnt.err;
import ppnt.traits;
import std;


namespace ppnt::io {

    static auto common_async_int_mapper(int result) -> Result<int> {
        if (result >= 0) [[likely]] {
            return result;
        }
        return std::unexpected{Error{std::error_code(-result, std::generic_category())}};
    }

    static auto common_async_unit_mapper(int result) -> Result<Unit> {
        if (result >= 0) [[likely]] {
            return Unit{};
        }
        return std::unexpected{Error{std::error_code(-result, std::generic_category())}};
    }

    namespace detail {

        auto map_async_read_result(int result) -> Result<int> {
            return common_async_int_mapper(result);
        }

        auto map_async_write_result(int result) -> Result<int> {
            return common_async_int_mapper(result);
        }

        auto map_async_connect_result(int result) -> Result<Unit> {
            return common_async_unit_mapper(result);
        }
    }


}