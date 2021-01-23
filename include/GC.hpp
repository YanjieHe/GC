#ifndef GC_HPP
#define GC_HPP
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class GCObject;
class Objectheader;
class GarbageCollector;

inline GarbageCollector *garbage_collector = nullptr;

class ObjectHeader {
public:
  bool marked;
  size_t size;

  ObjectHeader(bool marked, size_t size) : marked{marked}, size{size} {}
};

class GarbageCollector {
public:
  std::unordered_map<GCObject *, ObjectHeader *> trace_info;
  std::unordered_map<GCObject *, size_t> on_stack;
  size_t total_size;
  size_t memory_limit;

  explicit GarbageCollector(size_t memory_limit)
      : total_size{0}, memory_limit{memory_limit} {}

  void mark();

  void sweep();

  void add(GCObject *obj, ObjectHeader *header) {
    trace_info.insert({obj, header});
    total_size = total_size + header->size;
  }
};

class GCObject {
public:
  ObjectHeader *get_header() { return garbage_collector->trace_info.at(this); }
  virtual std::vector<GCObject *> get_fields() const = 0;
  static void *operator new(size_t size) {
    void *memory_block = ::operator new(size);
    auto header = new ObjectHeader(false, size);
    garbage_collector->add(reinterpret_cast<GCObject *>(memory_block), header);
    return memory_block;
  }
};

template <typename T> class gc_ptr {
public:
  gc_ptr() : obj{nullptr} {}

  explicit gc_ptr(GCObject *obj) : obj{obj} { pin(); }

  explicit gc_ptr(const gc_ptr<T> &other) : obj{other.obj} { pin(); }

  T *get() const noexcept { return dynamic_cast<T *>(obj); }

  T *operator->() const noexcept { return get(); }

  T &operator*() const noexcept { return *get(); }

  explicit operator bool() const noexcept { return obj != nullptr; }

  gc_ptr<T> &operator=(const gc_ptr<T> &other) noexcept {
    obj = other.obj;
    pin();
    return *this;
  }

  ~gc_ptr() { unpin(); }

private:
  GCObject *obj;

  void pin() {
    if (obj != nullptr) {
      garbage_collector->on_stack[obj]++;
    }
  }

  void unpin() {
    if (obj != nullptr) {
      size_t &count = garbage_collector->on_stack[obj];
      count--;
      if (count == 0) {
        garbage_collector->on_stack.erase(obj);
      }
    }
  }
};

inline void gc_init(size_t memory_limit) {
  garbage_collector = new GarbageCollector(memory_limit);
}

inline void gc_collect() {
  if (garbage_collector->total_size >= garbage_collector->memory_limit) {
    garbage_collector->mark();
    garbage_collector->sweep();
  }
}

template <class T, class... Args> gc_ptr<T> inline gc_new(Args &&...args) {
  gc_collect();
  return gc_ptr<T>(new T(args...));
}

inline void GarbageCollector::mark() {
  std::stack<GCObject *> stack;
  for (auto &[node, header] : trace_info) {
    header->marked = false;
  }
  for (auto &[node, count] : on_stack) {
    stack.push(node);
  }
  while (!stack.empty()) {
    GCObject *obj = stack.top();
    stack.pop();

    obj->get_header()->marked = true;
    const auto &fields = obj->get_fields();
    for (const auto &field : obj->get_fields()) {
      if (field != nullptr) {
        stack.push(field);
      }
    }
  }
}

inline void GarbageCollector::sweep() {
  std::vector<GCObject *> disposed;
  for (auto &[node, header] : trace_info) {
    if (!header->marked) {
      disposed.push_back(node);
    }
  }
  for (auto &node : disposed) {
    delete trace_info.at(node);
    trace_info.erase(node);
    delete node;
  }
}

#endif // GC_HPP