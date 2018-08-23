
Prerequisites
--------------

- LLVM/clang >= 5.0 (http://clang.llvm.org/get_started.html)
    - MACOSX: brew install --with-toolchain llvm

- GNU GMP from: https://gmplib.org

- NTL: A Library for doing Number Theory
    ```
    wget http://www.shoup.net/ntl/ntl-10.5.0.tar.gz
    tar xvf ntl-10.5.0.tar.gz
    cd ntl-10.5.0/src
    ./configure NTL_GMP_LIP=on PREFIX=/opt/libraries/ntl
    make
    make install
    ```

- Barvinok (will automatically install remaining prerequisites: pet, isl, polylib):
    ```
    [OPTIONAL]: git clone git://repo.or.cz/barvinok.git
    [OPTIONAL]: cd barvinok && ./get_submodules.sh

    wget http://barvinok.gforge.inria.fr/barvinok-0.41.tar.gz
    tar xf barvinok-0.41.tar.gz
    cd barvinok-0.41
    
    ./autogen.sh
    export BARVINOK_INSTALL=/opt/libraries/barvinok
    ./configure --prefix=$BARVINOK_INSTALL NTL_GMP_LIP=on 
    --with-gmp-prefix=/opt/libraries/gmp --with-ntl-prefix=/opt/libraries/ntl
    --with-pet=bundled --with-clang-prefix=/opt/compilers/llvm5 CXXFLAGS='-fno-rtti'
    make install

    cp pet/context.h $BARVINOK_INSTALL/include/
    cp pet/summary.h $BARVINOK_INSTALL/include/
    cp pet/expr.h $BARVINOK_INSTALL/include/
    cp pet/expr_access_type.h $BARVINOK_INSTALL/include/
  ```

Building PolyCheck-Clang  
------------------------
   
- Makefile based build: Edit the following lines in Makefile and run make  
    ```
    NTL_INSTALL = /opt/libraries/ntl  
    BARVINOK_INSTALL = /opt/libraries/barvinok  
    LLVM_INSTALL = /opt/compilers/llvm5
    (Optional) GMP_INSTALL = ...
    ```

- Using CMake
  	```
	cd PolyCheck-Clang
	mkdir build && cd build
	cmake .. 
    -DNTL_INSTALL_PATH=/opt/ntl 
    -DLLVM_INSTALL_PATH=/opt/llvm
    -DBARVINOK_INSTALL_PATH=/opt/barvinok-0.41 
	make
	```

Running PolyCheck-Clang
-----------------------
- Sample input files are in `tests/polybench`  
	`Syntax: PolyCheck source.c transformed.c`
    ```	
	cd PolyCheck-Clang
    ./PolyCheck gemm.c gemm.c
    ```