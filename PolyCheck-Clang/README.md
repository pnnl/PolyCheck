
Prerequisites
--------------

- LLVM/Clang >= 7.0 (currently tested with versions 7 & 8 only)
    - `MACOSX`: brew install llvm

- GNU GMP from: https://gmplib.org
    - `MACOSX`: brew install gmp

- NTL: A Library for doing Number Theory
    - `MACOSX`: brew install ntl
    ```
    wget http://www.shoup.net/ntl/ntl-10.5.0.tar.gz
    tar xvf ntl-10.5.0.tar.gz
    cd ntl-10.5.0/src
    ./configure NTL_GMP_LIP=on PREFIX=$HOME/ntl
    make
    make install
    ```

- Barvinok (will automatically install remaining prerequisites: pet, isl, polylib):  
    ```
    wget http://barvinok.gforge.inria.fr/barvinok-0.41.2.tar.gz
    tar xf barvinok-0.41.2.tar.gz
    cd barvinok-0.41.2
    
    export PATH="/usr/local/opt/llvm@7/bin":$PATH
    CC=clang CXX=clang++ \
    ./configure --prefix=$HOME/barvinok NTL_GMP_LIP=on --with-gmp-prefix=/usr/local/opt/gmp \
    --with-ntl-prefix=/usr/local/opt/ntl --with-pet=bundled CXXFLAGS='-fno-rtti' \
    [CPPFLAGS="-isysroot `xcrun --show-sdk-path`"] (for `MACOSX` build)

  ```

Building PolyCheck-Clang  
------------------------

`export PATH="/usr/local/opt/llvm@7/bin":$PATH`  

- `Makefile` based build.  
    Edit the following lines in `Makefile` accordingly and run `make`:    
    ```
    NTL_INSTALL_PATH = /usr/local/opt/ntl  
    BARVINOK_INSTALL_PATH = $HOME/barvinok  
    LLVM_INSTALL_PATH = /usr/local/opt/llvm@7
    (Optional) GMP_INSTALL_PATH = /usr/local/opt/gmp
    ```

- Using `CMake`
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
    export POLYCHECK_SYSROOT=`xcrun --show-sdk-path`
    export POLYCHECK_PET_ARGS="-I`xcrun --show-sdk-path`/usr/include"
    ./PolyCheck gemm.c gemm.c 
    ```
