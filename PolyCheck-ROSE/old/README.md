Prerequisites
--------------

- Please build ROSE from https://github.com/rose-compiler/rose-develop.git

- Build PolyOpt as follows:

	```
	git clone https://github.com/pouchet/rose-develop.git rose-polyopt
	git checkout polyopt-sync (make sure to use the polyopt-sync branch)

	export ROSE_SRC=/opt/rose-polyopt
	export ROSE_ROOT=/opt/rose-install
	export BOOST_ROOT=/opt/boost-install

	cd $ROSE_SRC/projects/PolyOpt2 (or) cd /opt/polyopt-build
	$ROSE_SRC/projects/PolyOpt2/install.sh
	```

- Build NTL-10.5 and Barvinok-0.41 as shown in [PolyCheck-Clang/README.md](../PolyCheck-Clang/README.md)

Building PolyCheck-ROSE
------------------------
- Makefile based build: Edit the following lines in Makefile and run make  
	```
	ROSE_INSTALL_PATH = /opt/compilers/ROSE
	BOOST_INSTALL_PATH = /opt/libraries/BOOST
	POLYOPT_INSTALL_PATH = /opt/compilers/ROSE/rose-polyopt/projects/PolyOpt2
	BARVINOK_INSTALL_PATH = /opt/libraries/barvinok-0.41
	NTL_INSTALL_PATH = /opt/libraries/ntl-10.5
	```

- Using CMake
	```
	cd PolyCheck-ROSE
	mkdir build && cd build
	cmake .. 
	-DROSE_INSTALL_PATH=/opt/compilers/ROSE 
	-DBOOST_INSTALL_PATH=/opt/libraries/BOOST 
	-DPOLYOPT_INSTALL_PATH=/opt/compilers/ROSE/rose-polyopt/projects/PolyOpt2
    -DNTL_INSTALL_PATH=/opt/libraries/ntl-10.5
    -DBARVINOK_INSTALL_PATH=/opt/libraries/barvinok-0.41 	
	make
	```

Running PolyCheck-ROSE
-----------------------
- Sample input files are in `tests/polybench`
	```	
	cd PolyCheck-ROSE
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:\
		$POLYOPT_INSTALL_PATH/polyopt/.libs:\
		$BARVINOK_INSTALL_PATH/lib:\
		$POLYOPT_INSTALL_PATH/pocc/math/install-isl/lib

	./build/PolyCheck -rose:skipfinalCompileStep --polyopt-safe-math-func --polyopt-scop-pragmas-only  ../../tests/polybench/gemm.c -target=../../tests/polybench/gemm.c -I../../tests/polybench/
	```

