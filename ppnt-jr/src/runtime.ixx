
export module ppnt.jr.runtime;

import std;
import v8;
import kknt.jr.event_loop;
import ppnt.jr.builtin;
import ppnt.common;

namespace {
    auto make_v8_string(v8::Isolate *isolate, std::string_view text) -> v8::Local<v8::String> {
        return v8::String::NewFromUtf8(isolate, text.data()).ToLocalChecked();
    }

    auto event_loop_from_isolate(v8::Isolate *isolate) -> ppnt::jr::EventLoop * {
        return static_cast<ppnt::jr::EventLoop *>(isolate->GetData(ppnt::jr::kEventLoopIsolateDataSlot));
    }

    auto parse_delay(const v8::FunctionCallbackInfo<v8::Value> &info, int index) -> long {
        auto *isolate = info.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if (info.Length() <= index || !info[index]->IsNumber()) {
            return 0;
        }
        auto maybe_value = info[index]->IntegerValue(context);
        if (maybe_value.IsNothing()) {
            return 0;
        }
        auto value = maybe_value.FromJust();
        if (value > std::numeric_limits<long>::max()) {
            return std::numeric_limits<long>::max();
        }
        if (value < std::numeric_limits<long>::min()) {
            return std::numeric_limits<long>::min();
        }
        return static_cast<long>(value);
    }

    auto throw_type_error(v8::Isolate *isolate, std::string_view message) -> void {
        isolate->ThrowException(v8::Exception::TypeError(make_v8_string(isolate, message)));
    }

    auto js_cpp_delay(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
        auto *isolate = info.GetIsolate();
        auto *event_loop = event_loop_from_isolate(isolate);
        if (event_loop == nullptr) {
            throw_type_error(isolate, "EventLoop is not available");
            return;
        }

        auto context = isolate->GetCurrentContext();
        auto delay_ms = parse_delay(info, 0);
        auto should_resolve = true;
        if (info.Length() > 1) {
            should_resolve = info[1]->BooleanValue(isolate);
        }

        auto maybe_resolver = v8::Promise::Resolver::New(context);
        if (maybe_resolver.IsEmpty()) {
            throw_type_error(isolate, "failed to create Promise resolver");
            return;
        }
        auto resolver = maybe_resolver.ToLocalChecked();
        auto promise = resolver->GetPromise();
        auto resolver_holder = std::make_shared<v8::Global<v8::Promise::Resolver>>(isolate, resolver);

        event_loop->set_timeout_task(
            [isolate, resolver_holder, should_resolve](v8::Local<v8::Context> ctx) {
                auto handle_scope = v8::HandleScope{isolate};
                auto local_resolver = v8::Local<v8::Promise::Resolver>::New(isolate, *resolver_holder);
                if (should_resolve) {
                    auto value = make_v8_string(isolate, "cpp resolved");
                    local_resolver->Resolve(ctx, value).Check();
                } else {
                    auto message = make_v8_string(isolate, "cpp rejected");
                    auto error = v8::Exception::Error(message);
                    local_resolver->Reject(ctx, error).Check();
                }
                resolver_holder->Reset();
            },
            delay_ms
        );

        info.GetReturnValue().Set(promise);
    }
}

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
            isolate_->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);
            event_loop_ = std::make_unique<EventLoop>(isolate_);
            reset_context();
        }

        ~Runtime() {
            context_.Reset();
            event_loop_.reset();
            isolate_->Dispose();
            delete allocator_;
        }

    private:
        auto bind_host_functions(v8::Local<v8::Context> context) -> void {
            auto global = context->Global();
            auto cpp_delay = v8::Function::New(context, js_cpp_delay).ToLocalChecked();
            global->Set(context, make_v8_string(isolate_, "cppDelay"), cpp_delay).Check();
        }

    public:
        auto reset_context() -> void {
            auto isolate_scope = v8::Isolate::Scope{isolate_};
            auto handle_scope = v8::HandleScope{isolate_};
            context_.Reset();
            event_loop_->clear_all();
            isolate_->SetData(kEventLoopIsolateDataSlot, event_loop_.get());
            auto ctx = builtin::register_all(isolate_);
            {
                auto context_scope = v8::Context::Scope{ctx};
                bind_host_functions(ctx);
            }
            context_.Reset(isolate_, ctx);
            std::println("[Runtime] Context reset");
        }

        auto execute_script(
            std::string_view code,
            bool wait_loop = true
        ) -> void {
            auto isolate_scope = v8::Isolate::Scope{isolate_};
            auto handle_scope = v8::HandleScope{isolate_};
            auto context = v8::Local<v8::Context>::New(isolate_, context_);
            auto context_scope = v8::Context::Scope{context};

            auto try_catch = v8::TryCatch{isolate_};
            auto source = make_v8_string(isolate_, code);
            auto maybe_script = v8::Script::Compile(context, source);
            if (maybe_script.IsEmpty()) {
                auto message = try_catch.Message();
                if (!message.IsEmpty()) {
                    auto text = v8::String::Utf8Value{isolate_, message->Get()};
                    std::println("[JS Compile Error]: {}", *text != nullptr ? *text : "<unknown>");
                } else {
                    std::println("[JS Compile Error]: <unknown>");
                }
                return;
            }

            auto maybe_result = maybe_script.ToLocalChecked()->Run(context);
            if (maybe_result.IsEmpty()) {
                auto message = try_catch.Message();
                if (!message.IsEmpty()) {
                    auto text = v8::String::Utf8Value{isolate_, message->Get()};
                    log::error({"[JS Runtime Error]: {}"}, *text != nullptr ? *text : "<unknown>");
                } else {
                    log::error({"[JS Runtime Error]: <unknown>"});
                }
                return;
            }

            isolate_->PerformMicrotaskCheckpoint();
            if (wait_loop) {
                event_loop_->run_until_empty(context);
            }
        }
    };
}
