# 1. Pangolin

```bash
https://github.com/stevenlovegrove/Pangolin.git
cd Pangolin
git checkout tags/v0.6 # 必须使用0.6， 0.5， 0.8等版本都会报错
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=./Pangolin ..
make -j8
sudo make install
```

# 2. 