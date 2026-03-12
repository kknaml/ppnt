module ppnt.jr.builtin.timeout;

import std;
import kknt.jr.event_loop;
import ppnt.jr.builtin.helper;

namespace ppnt::jr::builtin {

    namespace {
        auto get_event_loop(v8::Isolate *isolate) -> EventLoop * {
            return static_cast<EventLoop *>(isolate->GetData(kEventLoopIsolateDataSlot));
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
            isolate->ThrowException(v8::Exception::TypeError(str(isolate, message)));
        }

        auto js_set_timeout(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto *isolate = info.GetIsolate();
            auto *event_loop = get_event_loop(isolate);
            if (event_loop == nullptr) {
                throw_type_error(isolate, "EventLoop is not available");
                return;
            }
            if (info.Length() < 1 || !info[0]->IsFunction()) {
                throw_type_error(isolate, "setTimeout callback must be a function");
                return;
            }

            auto callback = v8::Local<v8::Function>::Cast(info[0]);
            auto delay_ms = parse_delay(info, 1);
            auto args = std::vector<v8::Local<v8::Value>>{};
            for (int i = 2; i < info.Length(); ++i) {
                args.push_back(info[i]);
            }
            auto id = event_loop->set_timeout(callback, delay_ms, args);
            info.GetReturnValue().Set(v8::Integer::New(isolate, id));
        }

        auto js_set_interval(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto *isolate = info.GetIsolate();
            auto *event_loop = get_event_loop(isolate);
            if (event_loop == nullptr) {
                throw_type_error(isolate, "EventLoop is not available");
                return;
            }
            if (info.Length() < 1 || !info[0]->IsFunction()) {
                throw_type_error(isolate, "setInterval callback must be a function");
                return;
            }

            auto callback = v8::Local<v8::Function>::Cast(info[0]);
            auto interval_ms = parse_delay(info, 1);
            auto args = std::vector<v8::Local<v8::Value>>{};
            for (int i = 2; i < info.Length(); ++i) {
                args.push_back(info[i]);
            }
            auto id = event_loop->set_interval(callback, interval_ms, args);
            info.GetReturnValue().Set(v8::Integer::New(isolate, id));
        }

        auto js_clear_timer(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto *isolate = info.GetIsolate();
            auto *event_loop = get_event_loop(isolate);
            if (event_loop == nullptr || info.Length() < 1) {
                return;
            }
            auto context = isolate->GetCurrentContext();
            auto maybe_id = info[0]->Int32Value(context);
            if (maybe_id.IsNothing()) {
                return;
            }
            event_loop->cancel(maybe_id.FromJust());
        }
    }

    auto TimeoutExtension::register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void {
        define_method_property(isolate, global, "setTimeout", js_set_timeout);
        define_method_property(isolate, global, "setInterval", js_set_interval);
        define_method_property(isolate, global, "clearTimeout", js_clear_timer);
        define_method_property(isolate, global, "clearInterval", js_clear_timer);
    }
}
