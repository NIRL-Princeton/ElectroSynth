//
// Created by Davis Polito on 2/20/25.
//
// Base class
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

#include "catch2/benchmark/catch_benchmark_all.hpp"
#include "catch2/catch_test_macros.hpp"

class A {
public:
    virtual ~A() = default; // Polymorphic base class
    virtual void show() const = 0; // Pure virtual function
};

// Five derived classes
class B : public A {
public:
    void show() const override { std::cout << "B\n"; }
    void specificB() const { std::cout << "Specific to B\n"; }
};

class C : public A {
public:
    void show() const override { std::cout << "C\n"; }
    void specificC() const { std::cout << "Specific to C\n"; }
};

class D : public A {
public:
    void show() const override { std::cout << "D\n"; }
    void specificD() const { std::cout << "Specific to D\n"; }
};

class E : public A {
public:
    void show() const override { std::cout << "E\n"; }
    void specificE() const { std::cout << "Specific to E\n"; }
};

class F : public A {
public:
    void show() const override { std::cout << "F\n"; }
    void specificF() const { std::cout << "Specific to F\n"; }
};

// Utility to clean up dynamic memory for std::vector<A*>
void cleanup(std::vector<A*>& objects) {
    for (auto* obj : objects) {
        delete obj;
    }
    objects.clear();
}
void dynamicCastSearch(A* obj) {
    if (auto* b = dynamic_cast<B*>(obj)) {
        b->specificB();
    } else if (auto* c = dynamic_cast<C*>(obj)) {
        c->specificC();
    } else if (auto* d = dynamic_cast<D*>(obj)) {
        d->specificD();
    } else if (auto* e = dynamic_cast<E*>(obj)) {
        e->specificE();
    } else if (auto* f = dynamic_cast<F*>(obj)) {
        f->specificF();
    }
}
void arraySearch(
    A* obj,
    const std::vector<B*>& Bs,
    const std::vector<C*>& Cs,
    const std::vector<D*>& Ds,
    const std::vector<E*>& Es,
    const std::vector<F*>& Fs) {
    // Search in each type-specific array
    if (std::find(Bs.begin(), Bs.end(), obj) != Bs.end()) {
        static_cast<B*>(obj)->specificB();
    } else if (std::find(Cs.begin(), Cs.end(), obj) != Cs.end()) {
        static_cast<C*>(obj)->specificC();
    } else if (std::find(Ds.begin(), Ds.end(), obj) != Ds.end()) {
        static_cast<D*>(obj)->specificD();
    } else if (std::find(Es.begin(), Es.end(), obj) != Es.end()) {
        static_cast<E*>(obj)->specificE();
    } else if (std::find(Fs.begin(), Fs.end(), obj) != Fs.end()) {
        static_cast<F*>(obj)->specificF();
    }
}
template<class RandomIt>
void random_shuffle(RandomIt first, RandomIt last)
{
    typedef typename std::iterator_traits<RandomIt>::difference_type diff_t;

    for (diff_t i = last - first - 1; i > 0; --i)
    {
        using std::swap;
        swap(first[i], first[std::rand() % (i + 1)]);
        // rand() % (i + 1) is not actually correct, because the generated number is
        // not uniformly distributed for most values of i. The correct code would be
        // a variation of the C++11 std::uniform_int_distribution implementation.
    }
}
TEST_CASE("Single Object Type Resolution Benchmark") {
    // Base class array (polymorphic container)
    std::vector<A*> allObjects;

    // Separate arrays for each derived type
    std::vector<B*> Bs;
    std::vector<C*> Cs;
    std::vector<D*> Ds;
    std::vector<E*> Es;
    std::vector<F*> Fs;

    // Generate test objects
    for (int i = 0; i < 1000; ++i) {
        if (i % 5 == 0) {
            auto* obj = new B();
            allObjects.push_back(obj);
            Bs.push_back(obj);
        } else if (i % 5 == 1) {
            auto* obj = new C();
            allObjects.push_back(obj);
            Cs.push_back(obj);
        } else if (i % 5 == 2) {
            auto* obj = new D();
            allObjects.push_back(obj);
            Ds.push_back(obj);
        } else if (i % 5 == 3) {
            auto* obj = new E();
            allObjects.push_back(obj);
            Es.push_back(obj);
        } else {
            auto* obj = new F();
            allObjects.push_back(obj);
            Fs.push_back(obj);
        }
    }

    // Randomize access to the objects for fair benchmarking
    random_shuffle(allObjects.begin(), allObjects.end());
    // Benchmark dynamic cast resolution
    BENCHMARK("Resolve Type with Dynamic Cast") {
        for (auto* obj : allObjects) {
            dynamicCastSearch(obj);
        }
    };

    // Benchmark array search resolution
    BENCHMARK("Resolve Type with Array Search") {
        for (auto* obj : allObjects) {
            arraySearch(obj, Bs, Cs, Ds, Es, Fs);
        }
    };

    // Clean up dynamically allocated memory
    cleanup(allObjects);
}