#include <iostream>
#include <queue>

class A
{
  public:
    const char* name = "hello";

    A() = default;

    A(const A&) = delete;

    A(A&&) = default;
};

template <typename T>
void push(T& val, std::queue<T>& q)
{
    q.push(std::move(val));
    std::cout << val.name << std::endl;
    std::cout << q.front().name << std::endl;
}

int main()
{
    A a{};
    std::queue<A> q{};
    push(a, q);
}
