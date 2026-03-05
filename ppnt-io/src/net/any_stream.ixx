export module ppnt.net.any_stream;
import std;
import ppnt.io.task;
import ppnt.err;
import ppnt.traits;

export namespace ppnt::net {

    // 1. Abstract Interface (The "Pointer" part)
    struct AnyStream {
        virtual ~AnyStream() = default;
        virtual auto read(std::span<uint8_t> buf, uint32_t timeout_ms = 0) -> io::Task<Result<size_t>> = 0;
        virtual auto write(std::span<const uint8_t> buf, uint32_t timeout_ms = 0) -> io::Task<Result<size_t>> = 0;
        virtual auto close() -> void = 0;
        virtual auto is_alive() -> bool = 0;
    };

    // 2. Implementation Wrapper (Bridges T -> AnyStream)
    template<typename T>
    struct AnyStreamImpl : AnyStream {
        T inner;
        explicit AnyStreamImpl(T t) : inner(std::move(t)) {}

        auto read(std::span<uint8_t> buf, uint32_t timeout_ms) -> io::Task<Result<size_t>> override { co_return co_await inner.read(buf, timeout_ms); }
        auto write(std::span<const uint8_t> buf, uint32_t timeout_ms) -> io::Task<Result<size_t>> override { co_return co_await inner.write(buf, timeout_ms); }
        auto close() -> void override { inner.close(); }
        auto is_alive() -> bool override { return inner.is_alive(); }
    };

    // 3. The Value Wrapper (passed to Http1Session)
    class BoxedStream : public NonCopy {
        std::unique_ptr<AnyStream> ptr_;
    public:
        // Constructor: Takes concrete stream, erases type
        template<typename T>
        requires (!std::is_same_v<std::decay_t<T>, BoxedStream>)
        explicit BoxedStream(T stream) 
            : ptr_(std::make_unique<AnyStreamImpl<T>>(std::move(stream))) {}

        // Move-only
        BoxedStream(BoxedStream &&other) noexcept : ptr_(std::move(other.ptr_)) {};
        auto operator=(BoxedStream &&other) noexcept -> BoxedStream & {
            if (this != &other) {
                ptr_ = std::move(other.ptr_);
            }
            return *this;
        }

        // --- The "Value" Interface (Matches Connection Concept) ---
        
        auto read(std::span<uint8_t> buf, uint32_t timeout_ms = 0) -> io::Task<Result<size_t>> {
            return ptr_->read(buf, timeout_ms);
        }

        auto write(std::span<const uint8_t> buf, uint32_t timeout_ms = 0) -> io::Task<Result<size_t>> {
            return ptr_->write(buf, timeout_ms);
        }

        auto close() -> void {
            ptr_->close();
        }

        auto is_alive() -> bool {
            return ptr_ && ptr_->is_alive();
        }
    };
}
