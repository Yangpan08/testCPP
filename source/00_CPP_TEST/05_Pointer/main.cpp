/*
* 1. std::unique_ptr reset release 换绑其他指针，容器中保存unique_ptr
* 2. 局部作用域创建shared_ptr unique_ptr
* 3. 局部作用域创建的shared_ptr存储在容器中，在其他作用域将容器元素设置为nullptr
* 4. new 和 delete 作用域
* 5. std::unique_ptr reset和release的区别，释放指针对象
* 6. 智能指针离开作用域前throw也会释放
* 7. weak_ptr判断是否有效
* 8. std::shared_ptr在多个类中使用时声明的位置，以及配合weak_ptr使用
* 9. 智能指针的get()函数并不会使引用计数加1，所以如果智能指针已经释放，之前使用get()返回的裸指针也会被释放
* 10.std::shared_ptr reset() 主动释放 std::shared_ptr没有release()函数
* 11.没有默认构造函数（无参构造函数）也可以使用智能指针
* 12.weak_ptr除过解决循环引用，还可以解决悬空指针的问题 https://blog.csdn.net/whahu1989/article/details/122443129
* 13.placement new 进程间共享内存 https://www.jianshu.com/p/342e69e3b9d6
* 14.std::shared_ptr循环引用造成内存泄漏
* 15.函数返回局部智能指针，裸指针，局部变量
* 16.std::shared_ptr线程安全 引用计数并不是简单的static变量 https://www.zhihu.com/question/56836057/answer/2158966805
* 17 类中new出来的成员变量内存位置 C++内存模型 00_07_TEST12
* 18 堆和栈 优点缺点 https://www.zhihu.com/question/379456802/answer/1115546749
* 19 APR内存池 http://www.wjhsh.net/jiangzhaowei-p-10383065.html
* 20 NULL nullptr nullptr_t
* 21 自定义智能指针的释放方法
* 22 new和delete应该一对一，越界赋值，new崩溃
*/

#define TEST18

#ifdef TEST1

#include <memory>
#include <iostream>
#include <vector>

class A
{
public:
    A() { std::cout << "construct\n"; }
    ~A() { std::cout << "destruct\n"; }
};
int main()
{
    std::cout << "start\n";
    {
        std::unique_ptr<A> a1;
        //a1.reset(std::make_unique<A>());  // 错误
        //a1.reset(new A); 正确，但不建议这样使用，new对应一个delete
        a1.reset(std::make_unique<A>().release());  // 此处的release并不调用析构函数
    }


    // release的作用
    {
        auto u1 = std::make_unique<A>();
        if (u1)
        {
            std::cout << "success\n";
        }
        auto p = u1.release(); //release只是释放控制权，会将控制的指针返回，并不销毁
        if (u1 == nullptr)
        {
            std::cout << "nullptr\n";
        }
    }

    // unique_ptr换绑其他对象
    {
        std::unique_ptr<A> a1;
        a1.reset(std::make_unique<A>().release());
    }

    // 容器中保存unique_ptr
    std::vector<std::unique_ptr<A>> vecA;
    {
        auto a = std::make_unique<A>();
        //vecA.emplace_back(a);  // 报错：尝试引用已删除的函数

        // 正确用法
        vecA.emplace_back(a.release());  // 使用release返回裸指针，然后插入vector
        vecA.emplace_back(std::make_unique<A>()); // 直接在vector上构造unique_ptr
        vecA.emplace_back(std::move(a)); // 使用移动语义将unique_ptr插入vector
    }

    std::cout << "end\n";
}

#endif // TEST1

#ifdef TEST2

#include <iostream>
#include <memory>
#include <vector>

class A
{
public:
    void setA(int _a)
    {
        a = _a;
    }
    int getA()
    {
        return a;
    }
private:
    int a{ 3 };
};

class Test
{
public:
    void func1()
    {
        // 局部创建的智能指针，离开作用域就会析构
        // 所以如果想要保留，可以将智能指针放入容器（成员变量，局部容器没意义）中保存
        auto s1 = std::make_shared<A>();
        s1->setA(99);
        m_shared.emplace_back(s1);

        auto u1 = std::make_unique<A>();
        u1->setA(88);
        m_unique.emplace_back(std::move(u1)); // 容器插入unique_ptr请看TEST1
    }

    void func2()
    {
        std::cout << "shared: " << m_shared[0]->getA() << "\n";
        std::cout << "unique: " << m_unique[0]->getA() << "\n";
    }

    int func3()
    {
        return m_shared[0]->getA();
    }

    std::vector<std::shared_ptr<A>> m_shared;
    std::vector<std::unique_ptr<A>> m_unique; // unique_ptr没必要使用容器，直接使用类型保存即可:std::unique_ptr<A> m_unique;
};

int main()
{
    A aa;
    Test test;
    test.func1();
    test.func2();
    std::cout << test.func3();
}

#endif // TEST2

#ifdef TEST3

#include <iostream>
#include <memory>
#include <array>

class A
{
public:
    A() { std::cout << "construct\n"; }
    ~A() { std::cout << "destruct\n"; }
};

class Test
{
public:
    void fun1()
    {
        auto s1 = std::make_shared<A>();
        m_shared[0] = s1;
    }
    void fun2()
    {
        m_shared[0] = nullptr;
    }
    std::array<std::shared_ptr<A>, 3> m_shared;
};

int main()
{
    std::cout << "start\n";
    {
        Test t;
        std::cout << "----------\n";
        t.fun1();
        std::cout << "----------\n";
        t.fun2(); // 此处就会释放Test的成员变量 m_shared，因为容器中的元素已被nullptr置换，shared_ptr被弹出，引用计数变为0
        std::cout << "----------\n";
    }

    std::cout << "end\n";
}

#endif // TEST3

#ifdef TEST4

#include <iostream>

class A
{
public:
    A() { std::cout << "construct\n"; }
    ~A() { std::cout << "destruct\n"; }
};

int main()
{
    std::cout << "start\n";
    A* a1 = nullptr;
    std::cout << "111\n";
    {
        a1 = new A();
    }
    std::cout << "222\n";
    delete a1;
    a1 = nullptr;
    std::cout << "end\n";
}

#endif // TEST4

#ifdef TEST5

#include <memory>
#include <iostream>

class A
{
public:
    A() { std::cout << "construct A\n"; }
    ~A() { std::cout << "destruct A\n"; }
};

class B
{
public:
    B() { std::cout << "construct B\n"; a = std::make_unique<A>(); }
    ~B() { std::cout << "destruct B\n"; }
    std::unique_ptr<A> a{ nullptr };
};

int main()
{
    {
        std::cout << "111\n";
        std::unique_ptr<B> b = std::make_unique<B>();
        // 1
        auto x = b.release(); // 调用release后不调用A的析构函数，release只是放弃控制权
        //delete x; // 调用A和B的析构函数
        //x = nullptr;

        // 2
        //b = nullptr;  // A和B都成功释放

        // 3
        //b.reset();   // A和B都成功释放

        std::cout << "222\n";
    }
}

#endif // TEST5

#ifdef TEST6

#include <iostream>
#include <memory>

class A
{
public:
    A() { std::cout << "construct\n"; }
    ~A() { std::cout << "destruct\n"; }
};

void FuncThrow()
{
    std::unique_ptr<A> a = std::make_unique<A>();
    throw "ss";
}

int main()
{
    try
    {
        FuncThrow();
    }
    catch (...)
    {
        std::cout << "catch an error\n";
    }
}
#endif // TEST6

#ifdef TEST7

#include <memory>
#include <vector>
#include <iostream>

int main()
{


    return 0;
}

#endif // TEST7

#ifdef TEST8

#include <iostream>
#include <memory>

class A
{
public:
    A()
    {
        std::cout << "AAAAA\n";
    }
    ~A()
    {
        std::cout << "aaaaa\n";
    }
};

class B
{
public:
    B()
    {
        std::cout << "BBBBB\n";
    }
    ~B()
    {
        std::cout << "bbbbb\n";
        std::cout << m_a.use_count() << "\tB A\n";
    }

    void SetA(const std::shared_ptr<A>& a)
    {
        m_a = a;
    }

    std::shared_ptr<A> m_a{ nullptr };
};

class C
{
public:
    C()
    {
        std::cout << "CCCCC\n";
        m_b = std::make_shared<B>();
        //m_a = std::make_shared<A>();
        auto pa = std::make_shared<A>();
        m_a = pa;
        m_b->SetA(pa);
    }
    ~C()
    {
        std::cout << "ccccc\n";
        std::cout << m_a.use_count() << "\tC A\n";
        std::cout << m_b.use_count() << "\tC B\n";
    }
private:
    // std::shared_ptr<A> m_a和std::shared_ptr<B> m_b这两个成员的声明顺序不一样，打印的结果也不一样（即使都会成功析构）
    // 合理的用法是，此处的m_a应该声明为std::weak_ptr，且std::weak_ptr应该在类C，而不是类B
    // 因为如果将std::shared_ptr<A>放在C中，B中的是std::weak_ptr<A>，且c中的m_a声明在m_b之后，就会先析构A，导致B的m_a无效
    // 如果还需要在B中的析构函数中使用m_a就会出问题

    //std::shared_ptr<A> m_a{ nullptr }; 
    std::shared_ptr<B> m_b{ nullptr };

    // m_a在类C中的声明顺序无所谓，它只是弱指针，析构时不会使A的引用计数减一也不会调用A的析构
    std::weak_ptr<A> m_a;
};

int main()
{
    {

        std::cout << "=========\n";
        std::unique_ptr<C> c = std::make_unique<C>();
        std::cout << "=========\n";
    }
    std::cout << "**********\n";
}


#endif // TEST8

#ifdef TEST9

#include <iostream>
#include <memory>

class A
{
public:
    A() { std::cout << " === construct\n"; }
    ~A() { std::cout << "*** destruct\n"; }

    void Set(int a)
    {
        m_int = a;
    }

    int Get()
    {
        return m_int;
    }
private:
    int m_int{ 0 };
};

class B
{
public:
    void SetPointerA(A* a)
    {
        m_a = a;
    }
    A* GetPointerA()
    {
        return m_a;
    }

private:
    A* m_a{ nullptr };
};

int main()
{
    // 智能指针和裸指针最好不要混合使用
    // 本例中的B::SetPointerA(A*)参数和成员变量修改为智能指针就不会有如下问题
    {
        B b;
        {
            auto pa = std::make_unique<A>();
            pa->Set(9);
            b.SetPointerA(pa.get());
            std::cout << b.GetPointerA()->Get() << "\n";
        }
        auto a = b.GetPointerA();
        std::cout << a->Get() << std::endl; // a对应的智能指针已经被析构，所以打印的是随机值
    }
    std::cout << "-------------------\n";
    {
        B b;
        {
            auto pa = std::make_shared<A>();
            pa->Set(8);
            b.SetPointerA(pa.get());
            std::cout << b.GetPointerA()->Get() << "\n";
        }
        auto a = b.GetPointerA();
        std::cout << a->Get() << std::endl; // a对应的智能指针已经被析构，所以打印的是随机值
    }
}


#endif // TEST9

#ifdef TEST10

#include <memory>
#include <iostream>
#include <vector>

class A
{
public:
    A() { std::cout << "AAA\n"; }
    ~A() { std::cout << "aaa\n"; }
};

int main()
{
    {
        std::vector<std::shared_ptr<A>> vec1;
        std::vector<std::shared_ptr<A>> vec2;

        auto pa = std::make_shared<A>();
        std::cout << pa.use_count() << '\n';
        vec1.emplace_back(pa);
        std::cout << pa.use_count() << '\n';
        vec2.emplace_back(pa);
        std::cout << pa.use_count() << '\n';

        // shared_ptr没有release函数
        pa.reset(); //不会调用析构函数

        std::cout << "11111111111\n";
    }
    std::cout << "============\n";
    {
        auto pa = std::make_shared<A>();
        std::weak_ptr<A> wa = pa;
        std::cout << wa.use_count() << '\n';
        //wa.lock().reset();
        pa.reset(); // 只有pa引用了智能指针，所以此处会调用析构函数
        std::cout << "222222222222\n";
    }
    std::cout << "============\n";
    {
        auto pa = std::make_shared<A>();
        std::weak_ptr<A> wa = pa;
        std::cout << wa.use_count() << '\n';
        wa.lock().reset(); // 不会调用析构函数
        std::cout << "222222222222\n";
    }
    std::cout << "============\n";
    {
        auto pa = std::make_shared<A>();
        auto ppa = pa;
        pa = nullptr;
        ppa = nullptr;
        std::cout << pa.use_count() << '\n';
        // 将所有引用到shared_ptr的变量置为nullptr才会调用析构
        std::cout << "3333333333333\n";
    }
    std::cout << "==============\n";
}

#endif // TEST10

#ifdef TEST11

#include <iostream>
#include <memory>

class A
{
public:
    A(int _a) { std::cout << " === construct\n"; }
    ~A() { std::cout << "*** destruct\n"; }
private:
};

int main()
{
    std::unique_ptr<A> ua1;
    std::shared_ptr<A> sa1;
    auto ua = std::make_unique<A>(1);
    //auto ua = std::make_unique<A>(); // error 必须有参数
    auto sa = std::make_shared<A>(1);
    //auto sa = std::make_shared<A>(1);
}

#endif // TEST11

#ifdef TEST12

#include <iostream>
#include <memory>

int main()
{
    {
        int* p1 = new int(2);
        int* p2 = p1;
        std::cout << *p1 << '\t' << *p2 << '\t';
        delete p1;
        // 对p1进行delete后，p1和p2都是野指针（悬空指针），指向未知的内存
        std::cout << *p1 << '\t' << *p2 << '\t';

    }
    std::cout << "------------------------\n";

    {
        int* p1 = new int(2);
        int* p2 = p1;
        std::cout << *p1 << '\t' << *p2 << '\t';
        delete p1;
        p1 = nullptr;
        // delete p1之后，再将p1置为nullptr，如果再对nullptr解引用就会中断
        //std::cout << *p1 << '\t' << *p2 << '\t';
    }
    std::cout << "------------------------\n";

    // 1111111111111
    {
        std::shared_ptr<int> s1 = std::make_shared<int>(3);
        std::weak_ptr<int> w1 = s1;

        if (w1.expired())
        {
            std::cout << " w1 expired\n";
        }
        else
        {
            std::cout << " w1 !expired\t";
            std::cout << *w1.lock() << '\n';
        }

        s1 = std::make_shared<int>(4);

        if (w1.expired())
        {
            // std::weak_ptr弱引用的std::shared_ptr如果指向了其他地方，原来的std::weak_ptr就会失效
            std::cout << " w1 expired\n";
        }
        else
        {
            std::cout << " w1 !expired\t";
            std::cout << *w1.lock() << '\n';
        }

        // 将新的std::shared_ptr重新绑定到std::weak_ptr就会继续有效
        w1 = s1;

        if (w1.expired())
        {
            std::cout << " w1 expired\n";
        }
        else
        {
            std::cout << " w1 !expired\t";
            std::cout << *w1.lock() << '\n';
        }
    }

    std::cout << "-----------------------------\n";

    // 22222222222
    //代码中weak1先是和sptr指向相同的内存空间，后面sptr释放了那段内存空间然后指向新的内存空间，
    //此时通过weak1的expired()方法就可以知道本来指向的那段内存空间已经被释放。因为不增加引用计数，所以不会影响内存的释放。非常方便。

    //以往使用老的方法，判断一个指针是否是NULL，但是如果这段内存已经被释放却没有赋一个NULL，
    //那么就会引发未知错误，这需要程序员手动赋值，而且出问题还很难排查。有了weak_ptr，就方便多了，直接使用其expired()方法看下就行了。
    {
        // OLD, problem with dangling pointer
        // PROBLEM: ref will point to undefined data!

        int* ptr = new int(10);
        int* ref = ptr;
        delete ptr;

        // NEW
        // SOLUTION: check expired() or lock() to determine if pointer is valid

        // empty definition
        std::shared_ptr<int> sptr;

        // takes ownership of pointer
        sptr.reset(new int);
        *sptr = 10;

        // get pointer to data without taking ownership
        std::weak_ptr<int> weak1 = sptr;

        // deletes managed object, acquires new pointer
        sptr.reset(new int);
        *sptr = 5;

        // get pointer to new data without taking ownership
        std::weak_ptr<int> weak2 = sptr;

        // weak1 is expired!
        if (weak1.expired())
        {
            std::cout << "weak1 is expired\n";
        }
        else
        {
            auto temp = weak1.lock();
            std::cout << "value (weak1 point to): " << *temp << '\n';
        }

        // weak2 points to new data (5)
        if (weak2.expired())
        {
            std::cout << "weak2 is expired\n";
        }
        else
        {
            auto temp = weak2.lock();
            std::cout << "value (weak2 point to): " << *temp << '\n';
        }
    }

    return 0;
}

#endif // TEST12

#ifdef TEST13

#include <iostream>

int main()
{
    int* p1 = new int(2);
    int* p2 = new int;
    int* p3 = new (p1)int; // p3和p1指向的是同一块内存，因此对p3和p1解引用得到的值相同
    *p3 = 5; // p3和p1解引用得到的值都是5

    int* p4 = new (reinterpret_cast<int*>(0x000001e570616340))int;  //可以运行，但是最好不要这样使用
    //int* p5 = new (reinterpret_cast<int*>(0x000001e570616341))int(5); // 错误，不能修改该内存地址对应的值
    int* p5 = new (reinterpret_cast<int*>(0x000001e570616341))int;
}

#endif // TEST13

#ifdef TEST14

#include <iostream>
#include <memory>

class B;

class A
{
public:
    A() = default;
    ~A() { std::cout << "~A\n"; }

    std::weak_ptr<B> _b; // 用weak_ptr替换share_ptr，避免循环引用造成内存泄漏
    //std::shared_ptr<B> _b;
};

class B
{
public:
    B() = default;
    ~B() { std::cout << "~B\n"; }

    std::weak_ptr<A> _a;
    //std::shared_ptr<A> _a;
};


int main()
{
    auto pa = std::make_shared<A>();
    auto pb = std::make_shared<B>();

    pa->_b = pb;
    pb->_a = pa;

    //auto ret = pa->_b.lock();  // weak_ptr必须使用lock()函数转为shared_ptr才能使用

    // 如果类A和类B中都定义了一个对方的shared_ptr，此处引用计数就会输出为2，导致内存泄漏
    std::cout << "A use count:" << pa.use_count() << std::endl;
    std::cout << "B use count:" << pb.use_count() << std::endl;
}

#endif // TEST14

#ifdef TEST15

#include <iostream>
#include <memory>

class Test
{
public:
    int m_a{ 999 };
    int* m_p{ nullptr };
    ~Test() { std::cout << "~Test\n"; }
};

void display(Test* p)
{
    std::cout << p << "," << p->m_a << "\n";
}

Test* create1()
{
    // sp是局部智能指针，函数返回后会立即释放，函数外不可以访问智能指针对应的裸指针
    auto sp = std::make_shared<Test>();
    return sp.get();
}

std::unique_ptr<Test> create2()
{
    // p是一个对象（值），函数外可以正常访问
    auto p = std::make_unique<Test>();
    //auto p = std::make_shared<Test>();
    return p;
}

int* create3()
{
    // arr是一个栈内存的指针（局部数组变量）。函数返回后，指向栈区的指针就会释放内存，函数外不可以正常访问
    int arr[3]{ 1,2,3 };
    return arr;
}

int* create4()
{
    // p是一个堆内存的指针，只要不显式delete，其生命周期一直到程序结束，函数外可以正常访问
    int* p = new int[3] { 1, 2, 3 };
    return p;
}

Test create5()
{
    // 异常
    int arr[]{ 1,2,3 };
    Test t;
    t.m_p = arr;
    return t;
}

Test create6()
{
    // 异常
    int arr[]{ 1,2,3 };
    Test t;
    t.m_p = arr;
    return std::move(t);
}

Test& create7()
{
    // 异常
    int arr[]{ 1,2,3 };
    Test t;
    t.m_p = arr;
    return t;
    //return std::ref(t);
}

Test create8()
{
    // 正常
    return Test();
}

int main()
{
    {
        auto t = std::make_unique<Test>();
        t->m_a = 3;
        std::cout << t.get() << "\n";
        display(t.get());
        // 调用display可以看作把display的代码块放在该作用域内，所以可以正常访问
    }

    std::cout << "========\n";

    {
        auto ret = create1();
        std::cout << "smart_point.get()" << ret->m_a << "\n"; //异常值
    }

    std::cout << "========\n";

    {
        auto ret = create2();
        std::cout << "smart_point" << ret->m_a << "\n";//正常值
    }

    std::cout << "=======\n";

    {
        auto ret = create3();
        std::cout << "stack" << ret[0] << "," << ret[1] << "," << ret[2] << "\n"; //异常值
    }

    std::cout << "=======\n";

    {
        auto ret = create4();
        std::cout << "heap" << ret[0] << "," << ret[1] << "," << ret[2] << "\n"; //正常值
    }

    std::cout << "=======\n";

    {
        auto ret = create5();
        if (ret.m_p)
            std::cout << "stack Object" << ret.m_a << "," << ret.m_p[0] << "," << ret.m_p[1] << "," << ret.m_p[2] << "\n";
    }

    std::cout << "=======\n";

    {
        auto ret = create6();
        if (ret.m_p)
            std::cout << "std::move" << ret.m_a << "," << ret.m_p[0] << "," << ret.m_p[1] << "," << ret.m_p[2] << "\n";
    }

    std::cout << "=======\n";

    {
        auto ret = create7();
        if (ret.m_p)
            std::cout << "ref" << ret.m_a << "," << ret.m_p[0] << "," << ret.m_p[1] << "," << ret.m_p[2] << "\n";
    }

    std::cout << "=======\n";

    {
        auto ret = create8();
        if (ret.m_p)
            std::cout << "Object" << ret.m_a << "," << ret.m_p[0] << "," << ret.m_p[1] << "," << ret.m_p[2] << "\n";
    }


    return 0;
}

#endif // TEST15

#ifdef TEST16


#endif // TEST16

#ifdef TEST17

/*

1.类的成员变量并不能决定自身的存储空间位置。决定存储位置的是对象的创建方式。
即：https://blog.csdn.net/qq_28584889/article/details/117037810

如果对象是函数内的非静态局部变量，则对象，对象的成员变量保存在栈区。
如果对象是全局变量，则对象，对象的成员变量保存在静态区。
如果对象是函数内的静态局部变量，则对象，对象的成员变量保存在静态区。
如果对象是new出来的，则对象，对象的成员变量保存在堆区。
下面是一个示例：当对象是new出来的时，其对象地址和成员变量、成员变量的成员变量、成员变量存储的数据都在堆区存储。

2.通过内存地址判断是在堆上还是栈上：
https://blog.csdn.net/tulingwangbo/article/details/79729548

*/


#include <iostream>
#include <memory>

class Test
{
public:
    Test() :
        m_c1('x'),
        m_c2('y'),
        m_p1(new char[3] {0}),
        m_p2(new char[3] {0})
    {
    }
    ~Test()
    {
        if (m_p1)
        {
            delete[] m_p1;
            m_p1 = nullptr;
        }

        if (m_p2)
        {
            delete[] m_p2;
            m_p2 = nullptr;
        }
    }

    char m_c1;
    char m_c2;
    char* m_p1;
    char* m_p2;
};

#define PRINT_ADDRESS(x) printf("%p\t%s\n",x,#x);

int main()
{
    std::cout << "=======================================\n";
    {
        int a{ 1 };
        int b{ 2 };
        char c{ 'x' };
        char d{ 'y' };

        int* pa = new int(2);
        int* pb = new int(3);
        char* pc = new char('c');
        char* pd = new char('d');

        // 栈
        PRINT_ADDRESS(&a);
        PRINT_ADDRESS(&b);
        PRINT_ADDRESS(&c);
        PRINT_ADDRESS(&d);
        std::cout << "-----------------------\n";
        // 堆
        PRINT_ADDRESS(pa);
        PRINT_ADDRESS(pb);
        PRINT_ADDRESS(pc);
        PRINT_ADDRESS(pd);
    }
    std::cout << "=======================================\n";
    {
        Test t;
        char c1 = 'c';
        char c2 = 'c';
        char* p1 = new char[2] {0};
        char* p2 = new char[2] {0};

        // 栈
        PRINT_ADDRESS(&t);
        PRINT_ADDRESS(&t.m_c1);
        PRINT_ADDRESS(&t.m_c2);
        PRINT_ADDRESS(&c1);
        PRINT_ADDRESS(&c2);
        std::cout << "-----------------------\n";
        // 堆
        PRINT_ADDRESS(t.m_p1);
        PRINT_ADDRESS(t.m_p2);
        PRINT_ADDRESS(p1);
        PRINT_ADDRESS(p2);
    }
    std::cout << "=======================================\n";
    {
        Test* t = new Test();
        char c1 = 'c';
        char c2 = 'c';
        char* p1 = new char[2] {0};
        char* p2 = new char[2] {0};

        // 堆
        PRINT_ADDRESS(t);
        PRINT_ADDRESS(&t->m_c1);
        PRINT_ADDRESS(&t->m_c2);
        PRINT_ADDRESS(t->m_p1);
        PRINT_ADDRESS(t->m_p2);
        PRINT_ADDRESS(p1);
        PRINT_ADDRESS(p2);
        std::cout << "-----------------------\n";
        // 栈
        PRINT_ADDRESS(&c1);
        PRINT_ADDRESS(&c2);
    }
    std::cout << "=======================================\n";
    {
        std::unique_ptr<Test> t = std::make_unique<Test>();
        char c1 = 'c';
        char c2 = 'c';
        char* p1 = new char[2] {0};
        char* p2 = new char[2] {0};

        // 堆
        PRINT_ADDRESS(t.get());
        PRINT_ADDRESS(&t->m_c1);
        PRINT_ADDRESS(&t->m_c2);
        PRINT_ADDRESS(t->m_p1);
        PRINT_ADDRESS(t->m_p2);
        PRINT_ADDRESS(p1);
        PRINT_ADDRESS(p2);
        std::cout << "-----------------------\n";
        // 栈
        PRINT_ADDRESS(&c1);
        PRINT_ADDRESS(&c2);
    }
    std::cout << "=======================================\n";
    {
        std::shared_ptr<Test> t = std::make_shared<Test>();
        char c1 = 'c';
        char c2 = 'c';
        char* p1 = new char[2] {0};
        char* p2 = new char[2] {0};

        // 堆
        PRINT_ADDRESS(t.get());
        PRINT_ADDRESS(&t->m_c1);
        PRINT_ADDRESS(&t->m_c2);
        PRINT_ADDRESS(t->m_p1);
        PRINT_ADDRESS(t->m_p2);
        PRINT_ADDRESS(p1);
        PRINT_ADDRESS(p2);
        std::cout << "-----------------------\n";
        // 栈
        PRINT_ADDRESS(&c1);
        PRINT_ADDRESS(&c2);
    }

    return 0;
}

#endif // TEST17

#ifdef TEST18

/*
3.C++内存模型
https://blog.csdn.net/weixin_43340455/article/details/124786128

4.C++自由存储区和堆
https://blog.csdn.net/great_emperor_/article/details/123261184
*/

/*
* 全局/静态存储区
* 1.BSS段
* 这个区域存放的是未曾初始化的静态变量、全局变量，但是不会存放常量，因为常量在定义的时候就一定会赋值了。未初始化的全局变量和静态变量，编译器在编译阶段都会将其默认为0
* 2.DATA段
* 这个区域主要用于存放编译阶段（非运行阶段时）就能确定的数据，也就是初始化的静态变量、全局变量和常量，这个区域是可读可写的。这也就是我们通常说的静态存储区
* 
*/

#include <iostream>

int g_a;
static int g_b;

int g_c = 7;
static int g_d = 8;

const int g_e = 9;
const static int g_f = 9;

const char* g_g = "abcd";

#define PRINT_ADDRESS(x) printf("address : %p\tvalue : %d\t arg : %s\n", &x, x, #x);
#define PRINT_ADDRESS_S(x) printf("address : %p\tvalue : %s\t arg : %s\n", &x, x, #x);

int main()
{
    //int m_a;
    static int m_b;

    int m_c = 7;
    static int m_d = 8;

    const int m_e = 9;
    const static int m_f = 9;

    const char* m_g = "abcd";
    static const char* m_h = "abcd";
    const static char* m_i = "abcd";








    PRINT_ADDRESS(g_a);
    PRINT_ADDRESS(g_b);
    PRINT_ADDRESS(m_b);

    PRINT_ADDRESS(g_c);
    PRINT_ADDRESS(g_d);
    PRINT_ADDRESS(m_d);

    PRINT_ADDRESS(g_e);
    PRINT_ADDRESS(g_f);
    std::cout << "------------------\n";
    //PRINT_ADDRESS(m_a);


    PRINT_ADDRESS(m_f);
    std::cout << "------------------\n";
    PRINT_ADDRESS(m_c);
    PRINT_ADDRESS(m_e);
    std::cout << "------------------\n";
    PRINT_ADDRESS_S(m_g);
    PRINT_ADDRESS_S(m_h);
    PRINT_ADDRESS_S(m_i);
    PRINT_ADDRESS_S(g_g);

    return 0;
}

#endif // TEST18

#ifdef TEST19


#endif // TEST19

#ifdef TEST20

#include <iostream>

void func(void*)
{
    std::cout << "func1\n";
}
void func(int)
{
    std::cout << "func2\n";
}

/*
* NULL只是一个宏，有如下定义
* #define NULL 0
*/

int main()
{
    func(0);  // 2
    func(NULL);  // 2
    func(nullptr); // 1
    func(static_cast<void*>(nullptr)); // 1

    // nullptr_t 是一个类型（指针空值类型），nullptr是nullptr_t类型的一个实例对象
    nullptr_t s = 0; // 必须初始化
    func(s); // 1
}
#endif // TEST20

#ifdef TEST21

#include <memory>
#include <iostream>

class Obj
{
public:
    Obj() { std::cout << "=== construct ===\n"; }
    ~Obj() { std::cout << "+++ destruct +++\n"; }
};

//自定义释放规则
void deleteInt(int* p) {
    std::cout << "use func pointer delete\n";
    delete[] p;
}

//自定义的释放规则
struct myDel
{
    void operator()(int* p) {
        std::cout << "use functor delete int\n";
        delete p;
    }

    void operator()(Obj* p) {
        std::cout << "use functor delete Obj\n";
        // 此处如果没有调用delete p,那么智能指针释放的时候，它所管理的Obj对象就不会调用析构
    }
};

int main()
{
    // std::shared_ptr
    {
        // 指定 default_delete 作为释放规则
        std::shared_ptr<int> p6(new int[10](), std::default_delete<int[]>());

        // 初始化智能指针，并自定义释放规则
        std::shared_ptr<int> p7(new int[10](), deleteInt);
        //std::shared_ptr<int> p5(new int[10](), myDel()); // error
        //std::shared_ptr<int, myDel> p5(new int[10](), myDel()); // error
        //std::shared_ptr<int, decltype(deleteInt)> p5(new int[10](), deleteInt); // error

        // 使用lambda指定智能指针释放规则
        std::shared_ptr<int> p8(new int[8](), [](int* p) {delete[] p; std::cout << "use lambda delete\n"; });
        //std::shared_ptr<int> p9 = std::make_shared<int>(new int[8](), [](int* p) {delete[]p; std::cout << "delete\n"; }); // error
        //auto p9 = std::make_shared<int>(new int[8](), [](int* p) {delete[]p; std::cout << "delete\n"; }); // error
    }

    std::cout << "-------------------\n";

    // std::unique_ptr只能使用函数对象的方式指定释放规则
    {
        std::unique_ptr<int, myDel> p1(new int);
        std::unique_ptr<int, myDel> p2(new int, myDel());
        //auto p3 = std::make_unique<int, myDel>(new int, myDel()); // error
        //auto p3 = std::make_unique<int>(new int, myDel()); // error
        //std::unique_ptr<int, myDel> p3 = std::make_unique<int, myDel>(new int, myDel()); // error
    }

    std::cout << "-------------------\n";

    {
        std::unique_ptr<Obj, myDel> p1(new Obj(), myDel());
        std::unique_ptr<Obj, myDel> p2(new Obj());
        std::unique_ptr<Obj> p3(new Obj());
    }

    std::cout << "-------------------\n";

    {
        std::shared_ptr<Obj> p1(new Obj(), [](Obj* p) {std::cout << "use lambda delete Obj\n"; });
        std::shared_ptr<Obj> p2(new Obj());
        //std::shared_ptr<Obj> p3 = p2;
        //p3 = p1;

        //std::cout << "1 count:\t" << p1.use_count() << '\n';
        //std::cout << "2 count:\t" << p2.use_count() << '\n';
        //std::cout << "3 count:\t" << p3.use_count() << '\n';
    }

    std::cout << "-------------------\n";

    return 0;
}

#endif // TEST21

#ifdef TEST22

#include <iostream>
#include <thread>


int main()
{
    // 一个new对应一个delete
    {
        int* p = new int[1000] {0};
        long long count = 1000000;
        while (count-- > 0)
        {
            // 【注意】重新将new的值赋给p之前，必须释放原来的p，不然内存泄露
            //  一个new对应一个delete
            delete[] p;
            p = nullptr;
            p = new int [1000] {0};
        }

        //std::this_thread::sleep_for(std::chrono::seconds(10));

        if (p)
        {
            delete[] p;
            p = nullptr;
        }
    }

    std::cout << "--------------------\n";

    {
        int* p = new int[100] {0};
        for (size_t i = 0; i < 1000; i++)
        {
            auto x = p[i];
            p[i] = (int)i; // 此处越界赋值破坏了堆，下一次new会崩溃
        }

        int* p2 = new int[100] {0};

        delete[] p;
        p = nullptr;

        delete[] p2;
        p2 = nullptr;
    }

}

#endif // TEST22
