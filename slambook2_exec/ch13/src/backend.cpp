#include "myslam/backend.h"
#include "myslam/algorithm.h"
#include "myslam/feature.h"
#include "myslam/g2o_types.h"
#include "myslam/map.h"
#include "myslam/mappoint.h"

namespace myslam {

Backend::Backend() {
    backend_running_.store(true); // std::atomic<bool> backend_running_;
    backend_thread_ = std::thread(std::bind(&Backend::BackendLoop, this)); // 类成员函数需要绑定该类的指针
    // bind函数的用法和详细参考：
    /*
    auto newCallable = bind(callable, arg_list);
    bind函数看做一个通用的函数适配器，它接受一个可调用对象callable，生成一个新的可调用对象newCallable。
    它可以把原可调用对象callable的某些参数预先绑定到给定的变量中（也叫参数绑定），然后产生一个新的可调用对象newCallable。
        网络编程中， 经常要使用到回调函数。 当底层的网络框架有数据过来时，往往通过回调函数来通知业务层。 这样可以使网络层只专注于 数据的收发， 而不必关心业务
        在c语言中， 回调函数的实现往往通过函数指针来实现。 但是在c++中 ， 如果回调函数是一个类的成员函数。这时想把成员函数设置给一个回调函数指针往往是不行的
        因为类的成员函数，多了一个隐含的参数this。 所以直接赋值给函数指针肯定会引起编译报错。
    1. 普通函数
        ```cpp
        void fun1(int n1, int n2, int n3)
        { 
            cout << n1 << " " << n2 << " " << n3 << endl;
        }

        int main()
        {
            //原fun1接受三个参数，其中绑定了2个，第三个参数由新的可调用对象指定  
            auto f1 = bind(fun1, 11, 22, _1); 
            f1(33);
        }
        ```
    1.1 普通函数与_1、_2
        ```cpp
        using namespace std::placeholders;
        void fun1(int n1, int n2, int n3)
        {
            cout << n1 << " " << n2 << " " << n3 << endl;
        }

        int main()
        {
            //_1表示这个位置是新的可调用对象的第一个参数的位置
            //_2表示这个位置是新的可调用对象的第二个参数的位置  
            auto f1 = bind(fun1, _2, 22, _1);
            f1(44,55);
        }
        ```
    2. 成员函数
        ```cpp
        class A
        {
        public:
            void print(int n1, int n2, int n3)
            {
                cout << n1 << " " << n2 << " " << n3 << endl;
            }
        };


        int main()                                                                                                                                   
        {
            A a;
            //类成员函数需要绑定该类的this指针  
            auto f1 = bind(&A::print, &a, _2, 22, _1);
            f1(44,55);
        }
        ```
    */
    // this指针的用法和详解参考：
    /*
    this 实际上是成员函数的一个形参，在调用成员函数时将对象的地址作为实参传递给 this。不过 this 这个形参是隐式的，它并不出现在代码中，而是在编译阶段由编译器默默地将它添加到参数列表中。
    this 作为隐式形参，本质上是成员函数的局部变量，所以只能用在成员函数的内部，并且只有在通过对象调用成员函数时才给 this 赋值。
    */
    // std::atomic的用法和详解参考：
    /*
    1. 原子操作std::atomic相关概念
        原子操作：更小的代码片段，并且该片段必定是连续执行的，不可分割。
    1.1 原子操作std::atomic与互斥量的区别
        1)互斥量：类模板，保护一段共享代码段，可以是一段代码，也可以是一个变量。
        2)原子操作std::atomic：类模板，保护一个变量。
    1.2 为何需要原子操作std::atomic
        为何已经有互斥量了，还要引入std::atomic呢，这是因为互斥量保护的数据范围比较大，我们期望更小范围的保护。并且当共享数据为一个变量时，原子操作std::atomic效率更高。
    2. 不加锁情况
        ```cpp
        #include <thread>
        #include <atomic> 
        #include <iostream>
        #include <time.h>
        
        using namespace std;
        // 全局的结果数据 
        long total = 0;
        
        // 点击函数
        void click()
        {
            for (int i = 0; i < 10000000; ++i)
            {
                // 对全局数据进行无锁访问 
                total += 1;
            }
        }
        
        int main(int argc, char* argv[])
        {
            // 计时开始
            clock_t start = clock();
            // 创建3个线程模拟点击统计
            thread th1(click);
            thread th2(click);
            thread th3(click);
            th1.join();
            th2.join();
            th3.join();
            
            // 计时结束
            clock_t finish = clock();
            // 输出结果
            cout << "result:" << total << endl;
            cout << "duration:" << finish - start << "ms" << endl;
            return 0;
        }
        ```
        result:
        ```bash
        result:10909121
        duration:313ms
        // 速度快，但是结果不对
        ```
    3. 加锁情况
        ```cpp
        #include <thread>
        #include <atomic> 
        #include <iostream>
        #include <time.h>
        #include <mutex>
        using namespace std;
        // 全局的结果数据 
        long total = 0;
        mutex m;
        // 点击函数
        void click()
        {
            for (int i = 0; i < 10000000; ++i)
            {
                //加锁
                m.lock();
                // 对全局数据进行无锁访问 
                total += 1;
                //解锁
                m.unlock();
            }
        }
        ```
        result:
        ```bash
        result:30000000
        duration:3858ms
        # 互斥对象的使用，保证了同一时刻只有唯一的一个线程对这个共享进行访问，从执行的结果来看，互斥对象保证了结果的正确性，
        # 但是也有非常大的性能损失，从刚才的313ms变成了现在的3858，用了原来时间的10多倍的时间。这个损失够大。
        ```
    4. 原子操作
        ```cpp
        #include <thread>
        #include <atomic> 
        #include <iostream>
        #include <time.h>
        using namespace std;
        // 全局的结果数据 
        atomic<long> total = 0;
        // 点击函数
        void click()
        {
            for (int i = 0; i < 10000000; ++i)
            {
                // 对全局数据进行无锁访问 
                total += 1;
            }
        }
        ```
        result:
        ```bash
        result:30000000
        duration:538ms
        # 结果正确！耗时只是使用mutex互斥对象的六分之一！也仅仅是不采用任何保护机制的时间的近2倍。可以说这是一个非常不错的成绩了
        ```
    5. 总结
    原子操作的实现跟普通数据类型类似，但是它能够在保证结果正确的前提下，提供比mutex等锁机制更好的性能，如果我们要访问的共享资源可以用原子数据类型表示，那么在多线程程序中使用这种新的等价数据类型，是一个不错的选择。
    */
}

void Backend::UpdateMap() {
    std::unique_ptr<std::mutex> lock(data_mutex_); // 没有defer_lock的话，创建就会自动上锁了
    map_update_.notify_one(); // 条件变量，随机唤醒一个wait的线程
    // std::unque_lock:
    /*
    1. mutex, lock, lock_guard
        1.1 lock
            ```cpp
            void work1(int& sum, std::mutex& mylock) {
                for (int i = 1; i < 5000; i++) {
                    mylock.lock();
                    sum += i;
                    mylock.unlock();
                }
            }
            void work2(int& sum, std::mutex& mylock) {
                for (int i = 5000; i <= 10000; i++) {
                    mylock.lock();
                    sum += i;
                    mylock.unlock();
                }
            }
            std::mutex mylock;
            int ans = 0;
            std::thread t1(work1, std::ref(ans), std::ref(mylock)); // std::ref的作用是将一个对象转换成一个引用包装器，以便在函数模板中使用。它接受一个对象作为参数，并返回一个引用包装器。
                // std::ref的使用场景包括但不限于：
                // ①在需要将对象作为引用传递给函数或对象时，避免对象的复制或移动操作。②在模板编程中，确保类型安全和正确的引用传递。③在使用std::bind和std::thread时，实现引用传递的效果。
            std::thread t2(work2, std::ref(ans), std::ref(mylock));
            ```
        1.2 lock_guard
            lock_guard类模板，它的内部结构很简单，只有构造函数和析构函数,在实例化对象时通过构造函数实现了lock，在析构函数中实现了unlock的操作。这样就可以避免忘记unlock的情况：
            ```CPP
            void work1(int& sum, std::mutex& mylock) {
                for (int i = 1; i < 5000; i++) {
                    std::lock_guard<std::mutex> mylock_guard(mylock);
                    sum += i;
                }
            }

            void work2(int& sum, std::mutex& mylock) {
                for (int i = 5000; i <= 10000; i++) {
                    std::lock_guard<std::mutex> mylock_guard(mylock);
                    sum += i;
                }
            }
            ```
            当然为了使用的更灵活方便，我们可以通过大括号来规定实现的范围
    2. 多线程死锁解决方法
        忘记unlock，或者有两个锁a和b，一个锁a在等待锁b的解锁，锁b在等待锁a的解锁，这些情况都会造成程序的死锁
        ```CPP
        void work1(std::mutex& mylock1, std::mutex& mylock2) {
            for (int i = 0; i < 100000; i++) {
                mylock1.lock();
                mylock2.lock();
                std::cout << "work1 : " << i << std::endl;
                mylock2.unlock();
                mylock1.unlock();
            }
        }
        void work2(std::mutex& mylock1, std::mutex& mylock2) {
            for (int i = 0; i < 100000; i++) {
                mylock2.lock();
                mylock1.lock();
                std::cout << "work2 : " << i << std::endl;
                mylock1.unlock();
                mylock2.unlock();
            }
        }
        ```
        解决这个死锁的问题只是把加锁的顺序改过来就可以了，然后也可以用std::lock函数来创建多个互斥锁，用法也很简单，首先创建两个互斥锁lock1和lock2，那么std::lock(lock1,lock2)这句代码就相当于lock1.lock();lock2.lock();
        最后不要忘了对两个锁的unlock，其实也可以搭配lock_guard()来使用，因为lock_guard内部就有析构函数来unlock，所以在lock_guard中引用std::adopt_lock参数
        ```cpp
        void work1(std::mutex& mylock1, std::mutex& mylock2) {
            for (int i = 0; i < 100000; i++) {
                std::lock(mylock1, mylock2); // 同时获取两个锁
                std::lock_guard<std::mutex> lock1(mylock1, std::adopt_lock); // std::adopt_lock是告诉编译器不需要获取锁，前面已经获取了，lock_guard在这里是为了达到自动释放的目的。
                std::lock_guard<std::mutex> lock2(mylock2, std::adopt_lock);
                std::cout << "work1 : " << i << std::endl;
            }
        }
        void work2(std::mutex& mylock1, std::mutex& mylock2) {
            for (int i = 0; i < 100000; i++) {
                std::lock(mylock1, mylock2);
                std::lock_guard<std::mutex> lock1(mylock1, std::adopt_lock);
                std::lock_guard<std::mutex> lock2(mylock2, std::adopt_lock);
                std::cout << "work2 : " << i << std::endl;
            }
        }
        ```
    3. std::unique_lock
        C++11中的unique_lock使用起来要比lock_guard更灵活，但是效率会第一点，内存的占用也会大一点。同样，unique_lock也是一个类模板，但是比起lock_guard，它有自己的成员函数来更加灵活进行锁的操作。
        使用方式和lock_guard一样，不同的是unique_lock有不一样的参数和成员函数
        std::unique_lock<std::mutex> munique(mlock); // 这样定义的话和lock_guard没有什么区别，最终也是通过析构函数来unlock。
        unique_lock也可以加std::adopt_lock参数，表示互斥量已经被lock，不需要再重复lock。该互斥量之前必须已经lock，才可以使用该参数。
        3.1 std::try_to_lock
            可以避免一些不必要的等待，会判断当前mutex能否被lock，如果不能被lock，可以先去执行其他代码。这个和adopt不同，不需要自己提前加锁。
            举个例子来说就是如果有一个线程被lock，而且执行时间很长，那么另一个线程一般会被阻塞在那里，反而会造成时间的浪费。
            那么使用了try_to_lock后，如果被锁住了，它不会在那里阻塞等待，它可以先去执行其他没有被锁的代码。具体实现过程如下：
            ```cpp
            void work1(int& s) {
                for (int i = 1; i <= 5000; i++) {
                    std::unique_lock<std::mutex> munique(mlock, std::try_to_lock);
                    if (munique.owns_lock() == true) {
                        s += i;
                    }
                    else {
                        // 执行一些没有共享内存的代码
                    }
                }
            }
            void work2(int& s) {
                for (int i = 5001; i <= 10000; i++) {
                    std::unique_lock<std::mutex> munique(mlock, std::try_to_lock);
                    if (munique.owns_lock() == true) {
                        s += i;
                    }
                    else {
                        // 执行一些没有共享内存的代码
                    }
                }
            }
            ```
        3.2 std::defer_lock
            这个参数表示暂时先不lock，之后手动去lock，但是使用之前也是不允许去lock。一般用来搭配unique_lock的成员函数去使用。
            当使用了defer_lock参数时，在创建了unique_lock的对象时就不会自动加锁，那么就需要借助lock这个成员函数来进行手动加锁，当然也有unlock来手动解锁。这个和mutex的lock和unlock使用方法一样，实现代码如下：
            ```cpp
            void work1(int& s) {
                for (int i = 1; i <= 5000; i++) {
                    std::unique_lock<std::mutex> munique(mlock, std::defer_lock);
                    munique.lock();
                    s += i;
                    munique.unlock();         // 这里可以不用unlock，可以通过unique_lock的析构函数unlock
                }
            }
            void work2(int& s) {
                for (int i = 5001; i <= 10000; i++) {
                    std::unique_lock<std::mutex> munique(mlock, std::defer_lock);
                    munique.lock();
                    s += i;
                    munique.unlock();
                }
            }
            ```
        3.3 std::try_lock
            和上面的try_to_lock参数的作用差不多，判断当前是否能lock，如果不能，先去执行其他的代码并返回false，如果可以，进行加锁并返回true，代码如下：
            ```cpp
            void work1(int& s) {
                for (int i = 1; i <= 5000; i++) {
                    std::unique_lock<std::mutex> munique(mlock, std::defer_lock);
                    if (munique.try_lock() == true) {
                        s += i;
                    }
                    else {
                        // 处理一些没有共享内存的代码
                    }
                }
            }
            ```
        3.4 release函数
            release函数，解除unique_lock和mutex对象的联系，并将原mutex对象的指针返回出来。如果之前的mutex已经加锁，需在后面自己手动unlock解锁，代码如下：
            ```cpp
            void work1(int& s) {
                for (int i = 1; i <= 5000; i++) {
                    std::unique_lock<std::mutex> munique(mlock);   // 这里是自动lock
                    std::mutex *m = munique.release();
                    s += i;
                    m->unlock();
                }
            }
            ```
        3.5 unique_lock的所有权的传递
            对越unique_lock的对象来说，一个对象只能和一个mutex锁唯一对应，不能存在一对多或者多对一的情况，不然会造成死锁的出现。所以如果想要传递两个unique_lock对象对mutex的权限，需要运用到移动语义或者移动构造函数两种方法。
            3.5.1 移动语义
                ```cpp
                std::unique_lock<std::mutex> munique1(mlock);
                std::unique_lock<std::mutex> munique2(std::move(munique1));
                // 此时munique1失去mlock的权限，并指向空值，munique2获取mlock的权限
                ```
            3.5.2 移动构造
                ```cpp
                std::unique_lock<std::mutex> rtn_unique_lock()
                {
                    std::unique_lock<std::mutex> tmp(mlock);
                    return tmp;
                }

                void work1(int& s) {
                    for (int i = 1; i <= 5000; i++) {
                        std::unique_lock<std::mutex> munique2 = rtn_unique_lock();
                        s += i;
                    }
                }
                ```
    4. 条件变量(condition_variable)
        condition_variable是一个类，搭配互斥量mutex来用，这个类有它自己的一些函数，这里就主要讲wait函数和notify_*函数，wait就是有一个等待的作用，notify就是有一个通知的作用。
        程序运行到wait函数的时候会先在此阻塞，然后自动unlock，那么其他线程在拿到锁以后就会往下运行，当运行到notify_one()函数的时候，就会唤醒wait函数，然后自动lock并继续下运行。
        当然wait还有第二个参数，这个参数接收一个布尔类型的值，当这个布尔类型的值为false的时候线程就会被阻塞在这里，只有当该线程被唤醒之后，且第二参数为true才会往下运行。
        notify_one()每次只能唤醒一个线程，那么notify_all()函数的作用就是可以唤醒所有的线程，但是最终能抢夺锁的只有一个线程，或者说有多个线程在wait，但是用notify_one()去唤醒其中一个线程，
        那么这些线程就出现了去争夺互斥量的一个情况，那么最终没有获得锁的控制权的线程就会再次回到阻塞的状态，那么对于这些没有抢到控制权的这个过程就叫做虚假唤醒。那么对于虚假唤醒的解决方法就是加一个while循环，比如下面这样：
        ```cpp
        while (que.size() == 0) {
            cr.wait(lck);
        }
        ```
        这个就是当线程被唤醒以后，先进行判断，是否可以去操作，如果可以再去运行下面的代码，否则继续在循环内执行wait函数。
        ```cpp
        void producer() {
            while(true) {
                {
                    std::unique_lock<std::mutex> lck(mtx);
                    // 在这里也可以加上wait 防止队列堆积  while(que.size() >= MaxSize) que.wait();
                    que.push(cnt);
                    std::cout << "向队列中添加数据：" << cnt ++ << std::endl;
                    // 这里用大括号括起来了 为了避免出现虚假唤醒的情况 所以先unlock 再去唤醒
                }
                cr.notify_all();       // 唤醒所有wait
            }
        }
        void consumer() {
            while (true) {
                std::unique_lock<std::mutex> lck(mtx);
                while (que.size() == 0) {           // 这里防止出现虚假唤醒  所以在唤醒后再判断一次
                    cr.wait(lck);
                }
                int tmp = que.front();
                std::cout << "从队列中取出数据：" << tmp << std::endl;
                que.pop();
            }
        }
        ```
    */
}

void Backend::Stop() {
    backend_running_.store(false); // replace the contained value with "parameter" 这里的parameter就是false
    map_update_.notify_one();
    backend_thread_.join();
}

void Backend::BackendLoop() {
    while (backend_running_.load()) { // load() Read contained value
        std::unique_lock<std::mutex> lock(data_mutex_);
        map_update_.wait(lock);
        // wait():一般编程中都需要先检查一个条件才进入等待环节，因此在中间有一个检查时段，检查条件的时候是不安全的，需要lock
        // 被notify_one唤醒后，wait()函数也会自动调用data_mutex_.lock()，使得data_mutex_恢复到上锁状态
        // 后端仅优化激活的Frames和Landmarks
        Map::KeyframesType active_kfs = map_->GetActiveKeyFrames();
        Map::LandmarksType active_landmarks = map_->GetActiveMapPoints();
        Optimize(active_kfs, active_landmarks);
    }
}

void Backend::Optimize(Map::KeyframesType &keyframes, Map::LandmarksType &landmarks) {
    // setup g2o
    typedef g2o::BlockSolver_6_3 BlockSolverType; // 对于二元边来说，这里的6，3是两个顶点的维度
    // 具体的先后顺序是库内写死的，第一个是pose，第二个是point
    // g2o::BlockSolver_6_3可以整体代替g2o::BlockSolver<g2o::BlockSolverTraits<6, 3>>
    typedef g2o::LinearSolverCSparse<BlockSolverType::PoseMatrixType> LinearSolverType;
    auto solver = new g2o::OptimizationAlgorithmLevenberg(g2o::make_unique<BlockSolverType>(g2o::make_unique<LinearSolverType>()));
    // 创建稀疏优化器
    g2o::SparseOptimizer optimizer;
    optimizer.setAlgorithm(solver);
    optimizer.setVerbose(true); // 打开调试输出

    // pose顶点，使用keyframe id
    std::map<unsigned long, VertexPose *> vertices;
    unsigned long max_kf_id = 0;
    for (auto &keyframe : keyframes) {
        auto kf = keyframe.second;
        VertexPose *vertex_pose = new VertexPose(); // camera vertex_pose
        vertex_pose->setId(kf->keyframe_id_);
        vertex_pose->setEstimate(kf->Pose());
        optimizer.addVertex(vertex_pose);
        if (kf->keyframe_id_ > max_kf_id) {
            max_kf_id = kf->keyframe_id_;
        }
        vertices.insert({kf->keyframe_id_, vertex_pose});
    }

    // 路标顶点，使用路标id索引
    std::map<unsigned long, VertexXYZ *> vertices_landmarks;

    // K 和左右外参
    Mat33 K = cam_left_->K();
    SE3 left_ext = cam_left_->pose();
    SE3 right_ext = cam_right_->pose();

    // edges
    int index = 1;
    double chi2_th = 5.991; // robust kernel 阈值
    std::map<EdgeProjection *, Feature::Ptr> edges_and_features;

    for (auto &landmark : landmarks) {
        if (landmark.second->is_outlier_) continue;
        unsigned long landmark_id = landmark.second->id_;
        auto observations = landmark.second->GetObs();
        for (auto &obs : observations) {
            if (obs.lock() == nullptr) continue;
            auto feat = obs.lock();
            if (feat->is_outlier_ || feat->frame_.lock() == nullptr) continue;

            auto frame = feat->frame_.lock();
            EdgeProjection *edge = nullptr;
            if (feat->is_on_left_image_) {
                edge = new EdgeProjection(K, left_ext);
            } else {
                edge = new EdgeProjection(K, right_ext);
            }

            // 如果landmark还没有被加入优化，则新加一个顶点
            if (vertices_landmarks.find(landmark_id) == vertices_landmarks.end()) {
                VertexXYZ *v = new VertexXYZ;
                v->setEstimate(landmark.second->Pos());
                v->setId(landmark_id + max_kf_id + 1);
                v->setMarginalized(true);
                vertices_landmarks.insert({landmark_id, v});
                optimizer.addVertex(v);
            }

            if (vertices.find(frame->keyframe_id_) != vertices.end() &&
                vertices_landmarks.find(landmark_id) != vertices_landmarks.end()) {
                edge->setId(index);
                edge->setVertex(0, vertices.at(frame->keyframe_id_));
                edge->setMeasurement(toVec2(feat->position_.pt));
                edge->setInformation(Mat22::Identity());
                auto rk = new g2o::RobustKernelHuber();
                rk->setDelta(chi2_th);
                edge->setRobustKernel(rk);
                edges_and_features.insert({edge, feat});
                optimizer.addEdge(edge);
                index++;
            } else {
                delete edge;
            }
        }
    }

    // do optimization and eliminate the outliers
    optimizer.initializeOptimization();
    optimizer.optimize(10);

    int cnt_outlier = 0, cnt_inlier = 0;
    int iteration = 0;
    while (iteration < 5) {
        cnt_outlier = 0;
        cnt_inlier = 0;
        // determine if we want to adjust the outlier threshold
        for (auto &ef : edges_and_features) {
            if (ef.first->chi2() > chi2_th) {
                cnt_outlier++;
            } else {
                cnt_inlier++;
            }
        }
        double inlier_ration = cnt_inlier / double(cnt_inlier + cnt_outlier);
        if (inlier_ratio > 0.5) {
            break;
        } else {
            chi2_th *= 2;
            iteration++;
        }
    }

    for (auto &ef : edges_and_feature) {
        if (ef.first->chi2() > chi2_th) {
            ef.second->is_outlier_ = true;
            // remove the observation
            ef.second->map_point_.lock()->RemoveObservation(ef.second);
        } else {
            ef.second->is_outlier_ = false;
        }
    }

    LOG(INFO) << "Outlier/Inlier in optimization: " << cnt_outlier << "/" << cnt_inlier;

    // Set pose and landmark position
    for (auto &v : vertices) {
        keyframes.at(v.first)->SetPose(v.second->estimate());
    }
    for (auto &v : vertices_landmarks) {
        landmarks.at(v.first)->SetPos(v.second->estimate());
    }
}
 
 
}