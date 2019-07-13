
Prerequisites
--------------

- LLVM/Clang >= 5.0 (currently tested with versions 5 & 6 only)
    - `MACOSX`: brew install llvm

- GNU GMP from: https://gmplib.org
    - `MACOSX`: brew install libgmp

- NTL: A Library for doing Number Theory
    ```
    wget http://www.shoup.net/ntl/ntl-10.5.0.tar.gz
    tar xvf ntl-10.5.0.tar.gz
    cd ntl-10.5.0/src
    ./configure NTL_GMP_LIP=on PREFIX=$HOME/ntl
    make
    make install
    ```

- Barvinok (will automatically install remaining prerequisites: pet, isl, polylib):  
   - `MACOSX NOTE:` barvinok does not currently build with brew installed gcc, please use the default gcc or brew installed llvm
    ```
    wget http://barvinok.gforge.inria.fr/barvinok-0.41.tar.gz
    tar xf barvinok-0.41.tar.gz
    cd barvinok-0.41
    
    export BARVINOK_INSTALL_PATH=$HOME/barvinok
    ./configure --prefix=$BARVINOK_INSTALL_PATH NTL_GMP_LIP=on 
    --with-gmp-prefix=/usr/local --with-ntl-prefix=$HOME/ntl
    --with-pet=bundled --with-clang-prefix=/usr/local/opt/llvm CXXFLAGS='-fno-rtti'
    make install

    cp pet/context.h $BARVINOK_INSTALL_PATH/include/
    cp pet/summary.h $BARVINOK_INSTALL_PATH/include/
    cp pet/expr.h $BARVINOK_INSTALL_PATH/include/
    cp pet/expr_access_type.h $BARVINOK_INSTALL_PATH/include/
  ```

Building PolyCheck-Clang  
------------------------
   
- Makefile based build: Edit the following lines in Makefile and run make  
    ```
    NTL_INSTALL_PATH = $HOME/ntl  
    BARVINOK_INSTALL_PATH = $HOME/barvinok  
    LLVM_INSTALL_PATH = /usr/local/opt/llvm
    (Optional) GMP_INSTALL_PATH = /usr/local
    ```

- Using CMake
  	```
	cd PolyCheck-Clang
	mkdir build && cd build
	cmake .. 
    -DNTL_INSTALL_PATH=$HOME/ntl 
    -DLLVM_INSTALL_PATH=$HOME/llvm
    -DBARVINOK_INSTALL_PATH=$HOME/barvinok
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
