
export module ppnt.jr.runtime;

import std;
import v8;
import kknt.jr.event_loop;

export namespace ppnt::jr {

    class Runtime {
    private:
        v8::Isolate *isolate_;
        v8::ArrayBuffer::Allocator *allocator_;
        v8::Global<v8::Context> context_;
        std::unique_ptr<EventLoop> event_loop_;

    public:
        Runtime() {
            auto create_param = v8::Isolate::CreateParams{};
            create_param.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
            allocator_ = create_param.array_buffer_allocator;
            isolate_ = v8::Isolate::New(create_param);
            event_loop_ = std::make_unique<EventLoop>(isolate_);
            reset_context();
        }

        ~Runtime() {
            context_.Reset();
            event_loop_.reset();
            isolate_->Dispose();
        }

        auto reset_context() -> void {
            auto isolate_scope = v8::Isolate::Scope{isolate_};
            auto handle_scope = v8::HandleScope{isolate_};
            context_.Reset();
            event_loop_->clear_all();
            auto global = v8::ObjectTemplate::New(isolate_);
            auto ctx = v8::Context::New(isolate_, nullptr, global);

            // TODO binding functions
            context_.Reset(isolate_, ctx);
            std::println("[Runtime] Context reset");
        }

        auto execute_script(
            std::string_view code,
            bool wait_loop = true
        ) -> void {

        }
    };
}