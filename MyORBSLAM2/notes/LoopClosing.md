# 一、分析`LoopClosing::RequestReset()`方法

## 1.1 `RequestReset`调用

- RequestReset只在一个地方被调用，是在`Tracking::Reset()`中`mpLoopClosing->RequestReset()`

## 1.2 `RequestReset`逻辑

- 1.首先在RequestReset中将mbResetRequested变量置为true，表示请求对线程进行重置；
- 2.RequestReset中，对mbResetRequested变量置为true后，再进入循环，直到检测到mbResetRequested变量被重新置为false，检测到为false
之后就完成了。
- 3.所以mbResetRequested变量会在哪里被置为false，它是一个protected变量，所以仅考虑在LoopClosing内部对它的修改，可以看到是在`ResetIfRequested()`中被置为false的，在置为false之前，会把`mlpLoopKeyFrameQueue`清空，mLastLoopKFid置为0。
- 4.`ResetIfRequested()`是在`LoopClosing`的线程主函数`Run`函数中被调用的，它的调用逻辑是每做一次循环都调用一次，检查是否需要重置线程；
- 5.`ResetIfRequested()`会判断`mbResetRequested`是否为true，也就是在步骤1的`RequestReset`中将其置为true，如果为true，才会改成false，并清理`mlpLoopKeyFrameQueue`。
- 所以，RequestReset中，对mbResetRequested变量置为true后，再进入循环，直到检测到mbResetRequested变量被重新置为false，其实就是在等待主线程调用`ResetIfRequested()`，也就是要清空资源，否则就一直等待。

