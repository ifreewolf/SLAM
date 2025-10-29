#include "myslam/mappoint.h"
#include "myslam/feature.h"

namespace myslam {

MapPoint::MapPoint(long id, Vec3 position) : id_(id), pos_(position) {}

MapPoint::Ptr MapPoint::CreateNewMappoint() {
    static long factory_id = 0;
    MapPoint::Ptr new_mappoint(new MapPoint);
    new_mappoint->id_ = factory_id;
    return new_mappoint;
}

void MapPoint::RemoveObservation(std::shared_ptr<Feature> feat) { // feat是强引用，而observations_存储的是弱引用
    std::unique_lock<std::mutex> lck(data_mutex_);
    for (auto iter = observations_.begin(); iter != observations_.end(); ++iter) {
        if (iter->lock() == feat) { // feat是强引用，所以需要使用lock()方法从弱引用中获得一个强引用
            observations_.erase(iter);
            feat->map_point_.reset();
            observed_times_--;
            break;
        }
    }
}
// std::weak_ptr是一种不参与引用计数的智能指针，它只持有对由std::shared_ptr管理的对象的弱引用。
// 避免循环引用：std::weak_ptr主要用于避免std::shared_ptr之间的循环引用导致的内存泄露
// 访问共享对象：使用weak_ptr.lock()可以获取一个std::shared_ptr，如果资源已经被释放，lock()返回一个空的std::shared_ptr
// 使用场景：适用于缓存、观察者模式，或者需要弱引用的地方。

// std::unique_ptr：独占所有权，不允许复制，只能通过移动转移所有权。适用于独占资源管理。
// std::shared_ptr：共享所有权，通过引用计数管理资源，适用于多个对象共享资源的场景。
// std::weak_ptr：非所有权弱引用，防止循环引用，用于需要弱引用的场景，如观察者模式。
// std::auto_ptr：已废弃，建议使用 std::unique_ptr 代替。

// std::weak_ptr没有重载操作符*和->，因为它不共享指针，不能操作资源，所以它的构造不会增加引用计数，析构也不会减少引用计数，它的主要作用就是作为一个旁观者监视shared_ptr中管理的资源是否存在。

// 1. 通过调用std::weak_ptr类提供的use_count()方法可以获得当前所观测资源的引用计数

/*
shared_ptr<int> sp(new int);

weak_ptr<int> wp1;
weak_ptr<int> wp2(wp1);
weak_ptr<int> wp3(sp);
weak_ptr<int> wp4;
wp4 = sp;
weak_ptr<int> wp5;
wp5 = wp3;

cout << "use_count: " << endl;
cout << "wp1: " << wp1.use_count() << endl;
cout << "wp2: " << wp2.use_count() << endl;
cout << "wp3: " << wp3.use_count() << endl;
cout << "wp4: " << wp4.use_count() << endl;
cout << "wp5: " << wp5.use_count() << endl;

==================
use_count:
wp1: 0
wp2: 0
wp3: 1
wp4: 1
wp5: 1
==================

通过打印的结果可以知道，虽然弱引用智能指针wp3、wp4、wp5监测的资源是同一个，但是它的引用计数并没有发生任何的变化，也进一步证明了weak_ptr只是监测资源，并不管理资源。 
*/

// 2. 通过调用std::weak_ptr类提供的expired()方法来判断观测的资源是否已经被释放

/*
shared_ptr<int> shared(new int(10));
weak_ptr<int> weak(shared);
cout << "1. weak " << (weak.expired() ? "is" : "is not") << " expired" << endl;

shared.reset();
cout << "2. weak " << (weak.expired() ? "is" : "is not") << " expired" << endl;

==============
1. weak is not expired
2. weak is expired
==============
weak_ptr监测的就是shared_ptr管理的资源，当共享智能指针调用shared.reset();之后管理的资源被释放，因此weak.expired()函数的结果返回true，表示监测的资源已经不存在了。
*/

// 3. 通过调用std::weak_ptr类提供的lock()方法来获取管理所监测资源的shared_ptr对象

/*
shared_ptr<int> sp1, sp2;
weak_ptr<int> wp;

sp1 = std::make_shared<int>(520); // 一个强引用
wp = sp1; // 一个弱引用
sp2 = wp.lock(); // 从弱引用中获得强引用
cout << "use_count: " << wp.use_count() << endl; // 2

sp1.reset(); // 释放一个强引用
cout << "use_count: " << wp.use_count() << endl; // 1

sp1 = wp.lock(); // 又从弱引用获得强引用
cout << "use_count: " << wp.use_count() << endl; // 2

cout << "*sp1: " << *sp1 << endl;

=====================
1. sp2 = wp.lock();通过调用lock()方法得到一个用于管理weak_ptr对象所监测的资源的共享智能指针对象，使用这个对象初始化sp2，此时所监测资源的引用计数为2
2. sp1.reset();共享智能指针sp1被重置，weak_ptr对象所监测的资源的引用计数减1
3. sp1 = wp.lock();sp1重新被初始化，并且管理的还是weak_ptr对象所监测的资源，因此引用计数加1
4. 共享智能指针对象sp1和sp2管理的是同一块内存，因此最终打印的内存中的结果是相同的，都是520
=====================
*/

// 4. 通过调用std::weak_ptr类提供的reset()方法来清空对象，使其不监测任何资源

/*
shared_ptr<int> sp(new int(10));
weak_ptr<int> wp(sp);
cout << "1. wp " << (wp.expired() ? "is" : "is not") << " expired" << endl;

wp.reset();
cout << "2. wp " << (wp.expired() ? "is" : "is not") << " expired" << endl;

====================
1. wp is not expired
2. wp is expired
====================
weak_ptr对象sp被重置之后wp.reset();变成了空对象，不再监测任何资源，因此wp.expired()返回true 
*/
}