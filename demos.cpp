//
// Created by 孙宇 on 2020/7/22.
//

#include <iostream>
#include "src/SomeTools.h"
#include "src/TestClazzSize.h"
#include <boost/timer.hpp>
#define expA(s) printf("前缀加上后的字符串为:%s\n",gc_##s)  //gc_s必须存在
// 注意事项1
#define gc_world "I am gc_world"

#define printWithPre(str) printf("带前缀:%s\n",pre_##str)

class A {
    virtual void fun() {}
};

class B {
    virtual void fun2() {}
};

class B1 {
    virtual void fun3() {}
};

class C : virtual public A, virtual public B, virtual public B1 {
public:
    //自身虚函数无论几个都只计算一个vptr
    virtual void fun4() {}
};

//位域
struct stuff {
    unsigned int field1: 30;
//    unsigned int       : 2;
    unsigned int field2: 4;
    unsigned int       : 0;
    unsigned int field3: 3;
};

//do{}while(0)各种用法
int fc()
{
    int k1 = 10;
    std::cout<<k1<<std::endl;
    do{
        int k1 = 100;
        std::cout<<k1<<std::endl;
    }while(0);
    std::cout<<k1<<std::endl;
}

//int main() {
//    boost::timer timer;
//    std::cout << "Hello, World!" << std::endl;
//    const char *helloClion = "Hello, Clion!";
//    SomeTools::printSomething(helloClion);
//    int num=0;
//    int * const ptr=&num; //const指针必须初始化！且const指针的值不能修改
//    int * t = &num;
//    *t = 1;
//    std::cout << *ptr <<std::endl;
//    std::cout << sizeof(TestClazzSize) << std::endl;

//    std::cout<<sizeof(A)<<" "<<sizeof(B)<<" "<<sizeof(C);

//    struct stuff s = {1, 3, 5};
//    std::cout << s.field1 << std::endl;
//    std::cout << s.field2 << std::endl;
//    std::cout << s.field3 << std::endl;
//    std::cout << sizeof(s) << std::endl;

    //宏定义测试
//    const char * gc_hello = "I am gc_hello";
//    expA(hello);
//    expA(world);
//    const char * pre_hello = "pre_hello";
//    printWithPre(hello);
//    fc();

//    std::cout << timer.elapsed() << std::endl;
//
//    return 0;
//}
