module;

#include <v8.h>
#include <libplatform/libplatform.h>

export module v8;

export namespace v8 {
    using ::v8::FunctionCallbackInfo;
    using ::v8::FunctionCallback;
    using ::v8::EscapableHandleScope;
    using ::v8::PropertyCallbackInfo;

}

// region "v8-array-buffer.h"
export namespace v8 {
    using ::v8::ArrayBufferCreationMode;
    using ::v8::BackingStoreInitializationMode;
    using ::v8::BackingStoreOnFailureMode;
    using ::v8::BackingStore;
    using ::v8::BackingStoreDeleterCallback;
    using ::v8::ArrayBuffer;
    using ::v8::ArrayBufferView;
    using ::v8::DataView;
    using ::v8::SharedArrayBuffer;
}

// endregion

// region "v8-container.h"
export namespace v8 {
    using ::v8::Array;
    using ::v8::Map;
    using ::v8::Set;
}

// endregion

// region "v8-context.h"
export namespace v8 {
    using ::v8::ExtensionConfiguration;
    using ::v8::Context;
}

// endregion

// region "v8-data.h"
export namespace v8 {
    using ::v8::Data;
    using ::v8::FixedArray;
}

// endregion

// region "v8-date.h"
export namespace v8 {
    using ::v8::Date;
}

// endregion

// region "v8-debug.h"
export namespace v8 {
    using ::v8::StackFrame;
    using ::v8::StackTrace;
}

// endregion

// region "v8-exception.h"
export namespace v8 {
    using ::v8::Exception;
    using ::v8::ExceptionContext;
    using ::v8::ExceptionPropagationMessage;
    using ::v8::ExceptionPropagationCallback;
    using ::v8::TryCatch;
}

// endregion

// region "v8-extension.h"
export namespace v8 {
    using ::v8::Extension;
    using ::v8::RegisterExtension;
}

// endregion

// region "v8-external.h"
export namespace v8 {
    using ::v8::ExternalPointerTypeTag;
    using ::v8::External;
}

// endregion

// region "v8-function.h"
export namespace v8 {
    using ::v8::Function;
}

// endregion

// region "v8-initialization.h"
export namespace v8 {
    using ::v8::EntropySource;
    using ::v8::ReturnAddressLocationResolver;
    using ::v8::DcheckErrorCallback;
    using ::v8::V8FatalErrorCallback;
    using ::v8::V8;
}

// endregion

// region "v8-internal.h"
export namespace v8::internal {
    using ::v8::internal::PlatformSmiTagging;
    using ::v8::internal::SandboxedPointer_t;
    using ::v8::internal::ExternalPointerHandle;
    using ::v8::internal::ExternalPointer_t;
    using ::v8::internal::CppHeapPointerHandle;
    using ::v8::internal::CppHeapPointer_t;
    using ::v8::internal::TagRange;
    using ::v8::internal::ExternalPointerTag;
    using ::v8::internal::ExternalPointerTagRange;
    using ::v8::internal::IndirectPointerHandle;
    using ::v8::internal::TrustedPointerHandle;
    using ::v8::internal::CodePointerHandle;
    using ::v8::internal::IsolateFromNeverReadOnlySpaceObject;
    using ::v8::internal::ShouldThrowOnError;
    using ::v8::internal::HandleScopeData;
    using ::v8::internal::Internals;
    using ::v8::internal::CastCheck;
    using ::v8::internal::BackingStoreBase;
    using ::v8::internal::StrongRootAllocatorBase;
    using ::v8::internal::StrongRootAllocator;
    using ::v8::internal::MaybeDefineIteratorConcept;
    using ::v8::internal::WrappedIterator;
    using ::v8::internal::ValueHelper;
    using ::v8::internal::HandleHelper;
    using ::v8::internal::VerifyHandleIsNonEmpty;
}

// endregion

// region "v8-isolate.h"
export namespace v8 {
    using ::v8::ResourceConstraints;
    using ::v8::MemoryPressureLevel;
    using ::v8::ContextDependants;
    using ::v8::StackState;
    using ::v8::IsolateGroup;
    using ::v8::Isolate;
}

// endregion

// region "v8-json.h"
export namespace v8 {
    using ::v8::JSON;
}

// endregion

// region "v8-local-handle.h"
export namespace v8::api_internal {
    using ::v8::api_internal::ToLocalEmpty;
}

export namespace v8 {
    using ::v8::LocalBase;
    using ::v8::LocalVector;
    using ::v8::Handle;
    using ::v8::MaybeLocal;
}

// endregion

// region "v8-locker.h"
export namespace v8 {
    using ::v8::Unlocker;
    using ::v8::Locker;
}

// endregion

// region "v8-maybe.h"
export namespace v8::internal {
    using ::v8::internal::NullMaybeType;
}

export namespace v8::api_internal {
    using ::v8::api_internal::FromJustIsNothing;
}

export namespace v8 {
    using ::v8::Maybe;
}

// endregion

// region "v8-memory-span.h"
export namespace v8 {
    using ::v8::MemorySpan;
    using ::v8::MemorySpan;
}

// endregion

// region "v8-message.h"
export namespace v8 {
    using ::v8::ScriptOriginOptions;
    using ::v8::ScriptOrigin;
    using ::v8::Message;
}

// endregion

// region "v8-microtask-queue.h"
export namespace v8 {
    using ::v8::MicrotaskQueue;
}

// endregion

// region "v8-microtask.h"
export namespace v8 {
    using ::v8::MicrotasksCompletedCallbackWithData;
    using ::v8::MicrotaskCallback;
    using ::v8::MicrotasksPolicy;
}

// endregion

// region "v8-object.h"
export namespace v8 {
    using ::v8::EmbedderDataTypeTag;
    using ::v8::ToExternalPointerTag;
    using ::v8::Private;
    using ::v8::PropertyDescriptor;
    using ::v8::PropertyAttribute;
    using ::v8::AccessorNameGetterCallback;
    using ::v8::AccessorNameSetterCallback;
    using ::v8::PropertyFilter;
    using ::v8::SideEffectType;
    using ::v8::KeyCollectionMode;
    using ::v8::IndexFilter;
    using ::v8::KeyConversionMode;
    using ::v8::IntegrityLevel;
    using ::v8::Object;
}

// endregion

// region "v8-persistent-handle.h"
export namespace v8::api_internal {
    using ::v8::api_internal::Eternalize;
    using ::v8::api_internal::CopyGlobalReference;
    using ::v8::api_internal::DisposeGlobal;
    using ::v8::api_internal::MakeWeak;
    using ::v8::api_internal::ClearWeak;
    using ::v8::api_internal::AnnotateStrongRetainer;
    using ::v8::api_internal::GlobalizeReference;
    using ::v8::api_internal::MoveGlobalReference;
}

export namespace v8 {
    using ::v8::Eternal;
    using ::v8::PersistentBase;
    using ::v8::NonCopyablePersistentTraits;
    using ::v8::Persistent;
    using ::v8::Global;
    using ::v8::Local;
    using ::v8::UniquePersistent;
    using ::v8::PersistentHandleVisitor;
}

// endregion

// region "v8-primitive-object.h"
export namespace v8 {
    using ::v8::NumberObject;
    using ::v8::BigIntObject;
    using ::v8::BooleanObject;
    using ::v8::StringObject;
    using ::v8::SymbolObject;
}

// endregion

// region "v8-primitive.h"
export namespace v8 {
    using ::v8::Primitive;
    using ::v8::Boolean;
    using ::v8::PrimitiveArray;
    using ::v8::Name;
    using ::v8::NewStringType;
    using ::v8::String;
    using ::v8::ExternalResourceVisitor;
    using ::v8::Symbol;
    using ::v8::Numeric;
    using ::v8::Number;
    using ::v8::Integer;
    using ::v8::Int32;
    using ::v8::Uint32;
    using ::v8::BigInt;
}

// endregion

// region "v8-promise.h"
export namespace v8 {
    using ::v8::Promise;
    using ::v8::PromiseHookType;
    using ::v8::PromiseHook;
    using ::v8::PromiseRejectEvent;
    using ::v8::PromiseRejectMessage;
    using ::v8::PromiseRejectCallback;
}

// endregion

// region "v8-proxy.h"
export namespace v8 {
    using ::v8::Proxy;
}

// endregion

// region "v8-regexp.h"
export namespace v8 {
    using ::v8::RegExp;
}

// endregion

// region "v8-script.h"
export namespace v8 {
    using ::v8::ScriptOrModule;
    using ::v8::UnboundScript;
    using ::v8::UnboundModuleScript;
    using ::v8::Location;
    using ::v8::ModuleRequest;
    using ::v8::Module;
    using ::v8::CompileHintsCollector;
    using ::v8::Script;
    using ::v8::ScriptType;
    using ::v8::ScriptCompiler;
}

// endregion

// region "v8-snapshot.h"
export namespace v8 {
    using ::v8::StartupData;
    using ::v8::SerializeInternalFieldsCallback;
    using ::v8::SerializeContextDataCallback;
    using ::v8::SerializeAPIWrapperCallback;
    using ::v8::DeserializeInternalFieldsCallback;
    using ::v8::DeserializeContextDataCallback;
    using ::v8::DeserializeAPIWrapperCallback;
    using ::v8::SnapshotCreator;
}

// endregion

// region "v8-statistics.h"
export namespace v8 {
    using ::v8::MeasureMemoryMode;
    using ::v8::MeasureMemoryExecution;
    using ::v8::MeasureMemoryDelegate;
    using ::v8::SharedMemoryStatistics;
    using ::v8::HeapStatistics;
    using ::v8::HeapSpaceStatistics;
    using ::v8::HeapObjectStatistics;
    using ::v8::HeapCodeStatistics;
}

// endregion

// region "v8-template.h"
export namespace v8 {
    using ::v8::Intrinsic;
    using ::v8::Template;
    using ::v8::Intercepted;
    using ::v8::NamedPropertyGetterCallback;
    using ::v8::NamedPropertySetterCallback;
    using ::v8::NamedPropertyQueryCallback;
    using ::v8::NamedPropertyDeleterCallback;
    using ::v8::NamedPropertyEnumeratorCallback;
    using ::v8::NamedPropertyDefinerCallback;
    using ::v8::NamedPropertyDescriptorCallback;
    using ::v8::IndexedPropertyGetterCallbackV2;
    using ::v8::IndexedPropertySetterCallbackV2;
    using ::v8::IndexedPropertyQueryCallbackV2;
    using ::v8::IndexedPropertyDeleterCallbackV2;
    using ::v8::IndexedPropertyEnumeratorCallback;
    using ::v8::IndexedPropertyDefinerCallbackV2;
    using ::v8::IndexedPropertyDescriptorCallbackV2;
    using ::v8::AccessCheckCallback;
    using ::v8::ConstructorBehavior;
    using ::v8::FunctionTemplate;
    using ::v8::PropertyHandlerFlags;
    using ::v8::NamedPropertyHandlerConfiguration;
    using ::v8::IndexedPropertyHandlerConfiguration;
    using ::v8::ObjectTemplate;
    using ::v8::DictionaryTemplate;
    using ::v8::Signature;
}

// endregion

// region "v8-traced-handle.h"
// export namespace v8::internal {
//     using ::v8::internal::TracedReferenceStoreMode;
//     using ::v8::internal::TracedReferenceHandling;
//     using ::v8::internal::GlobalizeTracedReference;
//     using ::v8::internal::MoveTracedReference;
//     using ::v8::internal::CopyTracedReference;
//     using ::v8::internal::DisposeTracedReference;
// }
//
// export namespace v8 {
//     using ::v8::TracedReferenceBase;
//     using ::v8::BasicTracedReference;
//     using ::v8::TracedReference;
// }

// endregion

// region "v8-typed-array.h"
export namespace v8 {
    using ::v8::TypedArray;
    using ::v8::Uint8Array;
    using ::v8::Uint8ClampedArray;
    using ::v8::Int8Array;
    using ::v8::Uint16Array;
    using ::v8::Int16Array;
    using ::v8::Uint32Array;
    using ::v8::Int32Array;
    using ::v8::Float16Array;
    using ::v8::Float32Array;
    using ::v8::Float64Array;
    using ::v8::BigInt64Array;
    using ::v8::BigUint64Array;
}

// endregion

// region "v8-unwinder.h"
export namespace v8 {
    using ::v8::RegisterState;
    using ::v8::StateTag;
    using ::v8::SampleInfo;
    using ::v8::MemoryRange;
    using ::v8::JSEntryStub;
    using ::v8::JSEntryStubs;
    using ::v8::Unwinder;
}

// endregion

// region "v8-value-serializer.h"
export namespace v8 {
    using ::v8::SharedValueConveyor;
    using ::v8::ValueSerializer;
    using ::v8::ValueDeserializer;
}

// endregion

// region "v8-value.h"
export namespace v8 {
    using ::v8::Value;
    using ::v8::TypecheckWitness;
}

// endregion

// region "v8-wasm.h"
export namespace v8 {
    using ::v8::OwnedBuffer;
    using ::v8::CompiledWasmModule;
    using ::v8::WasmMemoryObject;
    using ::v8::WasmModuleObject;
    using ::v8::WasmStreaming;
    using ::v8::WasmModuleCompilation;
    using ::v8::WasmMemoryMapDescriptor;
}

// endregion

// region "v8-platform.h"
export namespace v8 {
    using ::v8::TaskPriority;
    using ::v8::Task;
    using ::v8::IdleTask;
    using ::v8::TaskRunner;
    using ::v8::JobDelegate;
    using ::v8::JobHandle;
    using ::v8::JobTask;
    using ::v8::ScopedBoostablePriority;
    using ::v8::BlockingType;
    using ::v8::ScopedBlockingCall;
    using ::v8::ConvertableToTraceFormat;
    using ::v8::TracingController;
    using ::v8::SharedMemoryHandle;
    using ::v8::PlatformSharedMemoryHandle;
    using ::v8::PageAllocator;
    using ::v8::ThreadIsolatedAllocator;
    using ::v8::PagePermissions;
    using ::v8::VirtualAddressSpace;
    using ::v8::HighAllocationThroughputObserver;
    using ::v8::Platform;
}

// endregion

// region "libplatform/libplatform.h"
export namespace v8::platform {
    using ::v8::platform::IdleTaskSupport;
    using ::v8::platform::InProcessStackDumping;
    using ::v8::platform::MessageLoopBehavior;
    using ::v8::platform::PriorityMode;
    using ::v8::platform::NewDefaultPlatform;
    using ::v8::platform::NewDefaultJobHandle;
    using ::v8::platform::PumpMessageLoop;
    using ::v8::platform::RunIdleTasks;
    using ::v8::platform::NotifyIsolateShutdown;
}

// endregion


// region "v8-local-handle.h"
export namespace v8 {
    using ::v8::HandleScope;
}

// endregion
