export module ppnt.io:binary_util;

import std;
import ppnt.traits;

export namespace ppnt::io {

    template<typename T>
    [[nodiscard]]
    constexpr auto byteswap(T value) noexcept -> T {
        if constexpr (sizeof(T) == 1) {
            return value;
        }
        else if constexpr (sizeof(T) == 2) {
            return static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(value)));
        }
        else if constexpr (sizeof(T) == 4) {
            return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(value)));
        }
        else if constexpr (sizeof(T) == 8) {
            return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(value)));
        }
        else {
            static_assert(sizeof(T) == 0, "byteswap: Unsupported type size (must be 1, 2, 4, or 8 bytes)");
            return value;
        }
    }

    class BinartWriter {
    private:
        std::vector<uint8_t> buffer_{};

    public:
        auto write_u8(uint8_t b) -> void {
            buffer_.push_back(b);
        }

        auto write_bytes(std::span<const uint8_t> bytes) -> void {
            buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
        }

        auto write_bytes(const void *data, size_t len) -> void {
            const auto* p = static_cast<const uint8_t*>(data);
            buffer_.insert(buffer_.end(), p, p + len);
        }

        auto write_u16(uint16_t v, std::endian endian = std::endian::big) -> void {
            v = byteswap_if_needed(v, endian);
            write_bytes(&v, sizeof(v));
        }

        auto write_u32(uint32_t v, std::endian endian = std::endian::big) -> void {
            v = byteswap_if_needed(v, endian);
            write_bytes(&v, sizeof(v));
        }

        auto write_u64(uint64_t v, std::endian endian = std::endian::big) -> void {
            v = byteswap_if_needed(v, endian);
            write_bytes(&v, sizeof(v));
        }

        auto write_u24(uint32_t v, std::endian endian = std::endian::big) -> void {
            if (endian == std::endian::big) {
                buffer_.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
                buffer_.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
                buffer_.push_back(static_cast<uint8_t>(v & 0xff));
            } else {
                buffer_.push_back(static_cast<uint8_t>(v & 0xff));
                buffer_.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
                buffer_.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
            }
        }

        auto write_u8_length_prefixed(std::span<const uint8_t> data) -> void {
            write_u8(static_cast<uint8_t>(data.size()));
            write_bytes(data);
        }

        auto write_u16_length_prefixed(std::span<const uint8_t> data,
                                       std::endian endian = std::endian::big) -> void {
            write_u16(static_cast<uint16_t>(data.size()), endian);
            write_bytes(data);
        }

        template<typename Func>
        auto write_u16_length_prefixed(Func &&block_writer) -> void {
            size_t len_pos = buffer_.size();
            write_u16(0);
            size_t start_pos = buffer_.size();
            block_writer();
            size_t data_len = buffer_.size() - start_pos;
            buffer_[len_pos] = static_cast<uint8_t>(data_len >> 8);
            buffer_[len_pos + 1] = static_cast<uint8_t>(data_len);
        }

        auto write_u24_length_prefixed(std::span<const uint8_t> data) -> void {
            write_u24(static_cast<uint32_t>(data.size()));
            write_bytes(data);
        }

        auto write_string(std::string_view s) -> void {
            if (s.empty()) return;
            write_bytes(s.data(), s.size());
        }

        auto write_u16_length_prefixed_string(std::string_view s) -> void {
            write_u16(static_cast<uint16_t>(s.size()));
            write_string(s);
        }

        auto clear() -> void {
            buffer_.clear();
        }

        auto reserve(size_t n) -> void {
            buffer_.reserve(n);
        }

        auto size() const -> size_t {
            return buffer_.size();
        }

        auto data() const -> const uint8_t* {
            return buffer_.data();
        }

        auto data() -> uint8_t* {
            return buffer_.data();
        }

        auto buffer() const -> const std::vector<uint8_t> & {
            return buffer_;
        }

        auto buffer() -> std::vector<uint8_t> & {
            return buffer_;
        }

        auto leak() && -> std::vector<uint8_t> {
            return std::move(buffer_);
        }

    private:
        template <typename T>
        static T byteswap_if_needed(T v, std::endian endian) {
            if (endian == std::endian::native) {
                return v;
            }
            if constexpr (sizeof(T) == 2) {
                return byteswap(v);
            } else if constexpr (sizeof(T) == 4) {
                return byteswap(v);
            } else if constexpr (sizeof(T) == 8) {
                return byteswap(v);
            } else {
                return v;
            }
        }
    };

    class BinaryReader {
    private:
        std::span<const uint8_t> data_;
        size_t offset_{0};

    public:
        explicit BinaryReader(std::span<const uint8_t> data)
        : data_(data) {}

        auto eof() const -> bool {
            return offset_ >= data_.size();
        }

        auto remaining() const -> size_t {
            return data_.size() - offset_;
        }

        auto position() const -> size_t {
            return offset_;
        }

        auto seek(size_t pos) -> bool {
            if (pos > data_.size()) return false;
            offset_ = pos;
            return true;
        }

        auto skip(size_t n) -> bool {
            if (!ensure(n)) return false;
            offset_ += n;
            return true;
        }

        auto read_u8() -> std::optional<uint8_t> {
            if (!ensure(1)) return std::nullopt;
            return data_[offset_++];
        }

        auto read_bytes(size_t n) -> std::optional<std::span<const uint8_t>> {
            if (!ensure(n)) return std::nullopt;
            auto s = data_.subspan(offset_, n);
            offset_ += n;
            return s;
        }

        auto read_u16(std::endian endian = std::endian::big) -> std::optional<uint16_t> {
            if (!ensure(2)) return std::nullopt;
            uint16_t v;
            std::memcpy(&v, data_.data() + offset_, sizeof(v));
            offset_ += sizeof(v);
            return byteswap_if_needed(v, endian);
        }

        auto read_u32(std::endian endian = std::endian::big) -> std::optional<uint32_t> {
            if (!ensure(4)) return std::nullopt;
            uint32_t v;
            std::memcpy(&v, data_.data() + offset_, sizeof(v));
            offset_ += sizeof(v);
            return byteswap_if_needed(v, endian);
        }

        auto read_u64(std::endian endian = std::endian::big) -> std::optional<uint64_t> {
            if (!ensure(8)) return std::nullopt;
            uint64_t v;
            std::memcpy(&v, data_.data() + offset_, sizeof(v));
            offset_ += sizeof(v);
            return byteswap_if_needed(v, endian);
        }

        auto read_u24(std::endian endian = std::endian::big) -> std::optional<uint32_t> {
            if (!ensure(3)) return std::nullopt;

            uint32_t v = 0;
            if (endian == std::endian::big) {
                v = (static_cast<uint32_t>(data_[offset_]) << 16) |
                    (static_cast<uint32_t>(data_[offset_ + 1]) << 8) |
                    (static_cast<uint32_t>(data_[offset_ + 2]));
            } else {
                v = (static_cast<uint32_t>(data_[offset_ + 2]) << 16) |
                    (static_cast<uint32_t>(data_[offset_ + 1]) << 8) |
                    (static_cast<uint32_t>(data_[offset_]));
            }
            offset_ += 3;
            return v;
        }

        auto read_u8_length_prefixed() -> std::optional<std::span<const uint8_t>> {
            auto len = read_u8();
            if (!len) return std::nullopt;
            return read_bytes(*len);
        }

        auto read_u16_length_prefixed(std::endian endian = std::endian::big) -> std::optional<std::span<const uint8_t>> {
            auto len = read_u16(endian);
            if (!len) return std::nullopt;
            return read_bytes(*len);
        }

        auto read_u24_length_prefixed()  -> std::optional<std::span<const uint8_t>> {
            auto len = read_u24();
            if (!len) return std::nullopt;
            return read_bytes(*len);
        }


        auto read_string(size_t n) -> std::optional<std::string_view> {
            auto bytes = read_bytes(n);
            if (!bytes) return std::nullopt;
            return std::string_view(
                reinterpret_cast<const char*>(bytes->data()),
                bytes->size()
            );
        }

        auto read_u16_length_prefixed_string() -> std::optional<std::string_view> {
            auto bytes = read_u16_length_prefixed();
            if (!bytes) return std::nullopt;
            return std::string_view(
                reinterpret_cast<const char*>(bytes->data()),
                bytes->size()
            );
        }

    private:
        template <typename T>
        static T byteswap_if_needed(T v, std::endian endian) {
            if (endian == std::endian::native) {
                return v;
            }
            if constexpr (sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8) {
                return byteswap(v);
            } else {
                return v;
            }
        }

        auto ensure(size_t n) const -> bool {
            return offset_ + n <= data_.size();
        }
    };
}
