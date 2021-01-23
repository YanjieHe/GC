#include "GC.hpp"
#include <iostream>

using std::cout;
using std::endl;

class LinkedNode : public GCObject {
public:
  int val;
  LinkedNode *prev;
  LinkedNode *next;

  LinkedNode(int val, LinkedNode *prev, LinkedNode *next)
      : val{val}, prev{prev}, next{next} {}
  std::vector<GCObject *> get_fields() const override { return {prev, next}; }
};

void test() {
  auto n1 = gc_new<LinkedNode>(10, nullptr, nullptr);
  cout << "n1 created" << endl;
  auto n2 = gc_new<LinkedNode>(20, n1.get(), nullptr);
  cout << "n2 created" << endl;
  auto n3 = gc_new<LinkedNode>(30, n2.get(), nullptr);
  cout << "n3 created" << endl;
  cout << "try n3 copy" << endl;
  auto n3_copy{n3};
  cout << "finish n3 copy" << endl;
  n1->next = n2.get();
  n2->next = n3.get();
}

int main() {
  const size_t MEMORY_LIMIT = 536870912;
  cout << "init" << endl;
  gc_init(100);
  cout << "finish initialization" << endl;
  test();
  test();
  cout << garbage_collector->trace_info.size() << endl;
  gc_collect();
  cout << garbage_collector->trace_info.size() << endl;
  return 0;
}