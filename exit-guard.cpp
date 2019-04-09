#include <iostream>
#include <functional>
#include <memory>

// Keyword: scope_exit scope guard unique_ptr

class CleanUp {
    std::function<void (void)> m_cleanup;
 public:
  CleanUp(std::function<void (void)> f) : m_cleanup(f) {}
  ~CleanUp() { m_cleanup(); }

};

int func1() { std::cout << __FUNCTION__ << std::endl; return 0;}
void func2(int i) { std::cout << __FUNCTION__ << " " << i << std::endl; }
void func3(const char *name, int line) {
    std::cout << __FUNCTION__
              << " called by " << name 
              << " @ line << line"
              << std::endl;
}

int main(int argc, char **argv) {
    //CleanUp c([] () { func1(); func2(10); });
    //auto c = [] () { func1(); func2(10); };
    //auto p = [&] () { func1(); };

    // smart pointer and delete function
    std::unique_ptr<int, std::function<void (int*)>> c {
        (int *)1, [&] (int *) { func2(3); }
     };
    std::cout << __FUNCTION__ << std::endl;
    return 0;
}
