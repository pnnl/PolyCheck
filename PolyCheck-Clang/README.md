
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
   - `MACOSX NOTE:` barvinok does not currently build with brew installed gcc, please use the default gcc or brew installed llvm
    ```
    wget http://barvinok.gforge.inria.fr/barvinok-0.41.2.tar.gz
    tar xf barvinok-0.41.2.tar.gz
    cd barvinok-0.41.2
    
    export BARVINOK_INSTALL_PATH=$HOME/barvinok-0.41.2
    ./configure --prefix=$BARVINOK_INSTALL_PATH NTL_GMP_LIP=on \
    --with-gmp-prefix=/usr/local/opt/gmp --with-ntl-prefix=/usr/local/opt/ntl \
    --with-pet=bundled  --with-clang-prefix=/usr/local/opt/llvm \
    CXXFLAGS='-fno-rtti' \
    [CPPFLAGS="-isysroot `xcrun --show-sdk-path`"] (for `MACOSX`)

  ```

Building PolyCheck-Clang  
------------------------
   
- Makefile based build: Edit the following lines in Makefile and run make  
    ```
    NTL_INSTALL_PATH = $HOME/ntl  
    BARVINOK_INSTALL_PATH = $HOME/barvinok  
    LLVM_INSTALL_PATH = /usr/local/opt/llvm
    (Optional) GMP_INSTALL_PATH = /usr/local/opt/gmp
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
    export POLYCHECK_SYSROOT=`xcrun --show-sdk-path`
    export POLYCHECK_PET_ARGS="-I`xcrun --show-sdk-path`/usr/include"
    ./PolyCheck gemm.c gemm.c 
    ```
