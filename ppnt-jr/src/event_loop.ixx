
export module kknt.jr.event_loop;

import std;
import v8;
import ppnt.traits;

export namespace ppnt::jr {
    inline constexpr int kEventLoopIsolateDataSlot = 0;

    struct TimerKey {
        std::chrono::steady_clock::time_point deadline;
        int id;

        auto operator<=>(const TimerKey &other) const noexcept -> std::strong_ordering {
            return this->deadline <=> other.deadline;
        }
    };

    struct TimerData {
        v8::Global<v8::Function> callback;
        std::vector<v8::Global<v8::Value>> args;
        std::function<void(v8::Local<v8::Context>)> native_task;
        bool is_native{false};
        bool is_interval;
        long interval_ms;
    };


    class EventLoop final : public NonCopy  {
    private:
        v8::Isolate *isolate_;
        int next_id_{1};
        std::priority_queue<TimerKey, std::vector<TimerKey>, std::greater<>> timer_heap_;
        std::unordered_map<int, TimerData> timer_map_;
    public:
        explicit EventLoop(v8::Isolate *isolate) : isolate_{isolate} {}

        auto clear_all() -> void {
            timer_map_.clear();
            timer_heap_ = {};
        }

        auto run_until_empty(v8::Local<v8::Context> context) -> void {
            while (tick(context)) {
                wait_for_next();
            }
        }

        auto cancel(int id) -> void {
            timer_map_.erase(id);
        }

        auto set_timeout(
            v8::Local<v8::Function> callback,
            long delay_ms,
            const std::vector<v8::Local<v8::Value>> &args = {}
        ) -> int {
            return schedule_js(callback, delay_ms, false, args);
        }

        auto set_interval(
            v8::Local<v8::Function> callback,
            long interval_ms,
            const std::vector<v8::Local<v8::Value>> &args = {}
        ) -> int {
            auto normalized = std::max(1L, interval_ms);
            return schedule_js(callback, normalized, true, args);
        }

        auto set_timeout_task(
            std::function<void(v8::Local<v8::Context>)> task,
            long delay_ms
        ) -> int {
            return schedule_task(std::move(task), delay_ms, false);
        }

        auto set_interval_task(
            std::function<void(v8::Local<v8::Context>)> task,
            long interval_ms
        ) -> int {
            auto normalized = std::max(1L, interval_ms);
            return schedule_task(std::move(task), normalized, true);
        }

        auto run_once(v8::Local<v8::Context> context) -> bool {
            return tick(context);
        }

        auto has_pending() const -> bool {
            return !timer_map_.empty();
        }

    private:

        auto normalize_delay(long delay_ms) const -> long {
            return std::max(0L, delay_ms);
        }

        auto schedule_js(
            v8::Local<v8::Function> callback,
            long delay_ms,
            bool is_interval,
            const std::vector<v8::Local<v8::Value>> &args
         ) -> int {
            auto id = next_id_++;
            auto now = std::chrono::steady_clock::now();
            auto normalized = normalize_delay(delay_ms);
            auto deadline = now + std::chrono::milliseconds(normalized);
            timer_heap_.push({deadline, id});
            auto data = TimerData{};
            data.callback.Reset(isolate_, callback);
            data.args.reserve(args.size());
            for (auto value: args) {
                v8::Global<v8::Value> stored{isolate_, value};
                data.args.emplace_back(std::move(stored));
            }
            data.is_native = false;
            data.is_interval = is_interval;
            data.interval_ms = is_interval ? std::max(1L, normalized) : normalized;
            timer_map_.emplace(id, std::move(data));
            return id;
        }

        auto schedule_task(
            std::function<void(v8::Local<v8::Context>)> task,
            long delay_ms,
            bool is_interval
        ) -> int {
            auto id = next_id_++;
            auto now = std::chrono::steady_clock::now();
            auto normalized = normalize_delay(delay_ms);
            auto deadline = now + std::chrono::milliseconds(normalized);
            timer_heap_.push({deadline, id});
            auto data = TimerData{};
            data.native_task = std::move(task);
            data.is_native = true;
            data.is_interval = is_interval;
            data.interval_ms = is_interval ? std::max(1L, normalized) : normalized;
            timer_map_.emplace(id, std::move(data));
            return id;
        }

        auto tick(v8::Local<v8::Context> context) -> bool {
            isolate_->PerformMicrotaskCheckpoint();
            auto now = std::chrono::steady_clock::now();
            while (!timer_heap_.empty()) {
                auto key = timer_heap_.top();
                if (key.deadline > now) {
                    return true;
                }
                timer_heap_.pop();
                auto it = timer_map_.find(key.id);
                if (it == timer_map_.end()) {
                    continue;
                }
                auto is_native = it->second.is_native;
                auto is_interval = it->second.is_interval;
                auto cb_copy = v8::Global<v8::Function>{};
                auto args_copy = std::vector<v8::Global<v8::Value>>{};
                auto native_task = std::function<void(v8::Local<v8::Context>)>{};
                if (is_native) {
                    native_task = it->second.native_task;
                } else {
                    cb_copy = v8::Global<v8::Function>(isolate_, it->second.callback);
                    args_copy.reserve(it->second.args.size());
                    for (auto &arg: it->second.args) {
                        v8::Global<v8::Value> copied{isolate_, arg};
                        args_copy.emplace_back(std::move(copied));
                    }
                }

                if (!is_interval) {
                    timer_map_.erase(it);
                }
                {
                    auto handle_scope = v8::HandleScope{isolate_};
                    auto try_catch = v8::TryCatch{isolate_};
                    if (is_native) {
                        native_task(context);
                    } else {
                        auto func = v8::Local<v8::Function>::New(isolate_, cb_copy);
                        auto receiver = context->Global();
                        auto argv = std::vector<v8::Local<v8::Value>>{};
                        argv.reserve(args_copy.size());
                        for (auto &arg: args_copy) {
                            argv.push_back(v8::Local<v8::Value>::New(isolate_, arg));
                        }
                        auto _ = func->Call(context, receiver, static_cast<int>(argv.size()), argv.data()).IsEmpty();
                    }
                    if (try_catch.HasCaught()) {
                        auto message = try_catch.Message();
                        if (!message.IsEmpty()) {
                            auto err = v8::String::Utf8Value{isolate_, message->Get()};
                            std::println("[JS Error]: {}", *err != nullptr ? *err : "<unknown>");
                        } else {
                            auto err = v8::String::Utf8Value{isolate_, try_catch.Exception()};
                            std::println("[JS Error]: {}", *err != nullptr ? *err : "<unknown>");
                        }
                    }
                }
                isolate_->PerformMicrotaskCheckpoint();

                if (is_interval) {
                    auto refresh_it = timer_map_.find(key.id);
                    if (refresh_it != timer_map_.end()) {
                        auto new_deadline = key.deadline + std::chrono::milliseconds(refresh_it->second.interval_ms);
                        timer_heap_.push({new_deadline, key.id});
                    }
                }
            }
            return !timer_map_.empty();
        }

        auto wait_for_next() -> void {
            if (timer_heap_.empty()) return;
            auto key = timer_heap_.top();
            auto now = std::chrono::steady_clock::now();
            if (key.deadline > now) {
                std::this_thread::sleep_for(key.deadline - now);
            }
        }
    };
}
