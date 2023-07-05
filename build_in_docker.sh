#!/bin/bash
set -e

docker_image_id=$(sha1sum  build_env.dockerfile | awk '{print $1}')
image_exists_ret=$(docker manifest inspect ghcr.io/docwire/doctotext_build_env:$docker_image_id > /dev/null; echo $?)
if [ $image_exists_ret -eq 0 ]; then
	docker pull ghcr.io/docwire/doctotext_build_env:$docker_image_id
else
	docker build -t ghcr.io/docwire/doctotext_build_env:$docker_image_id -f build_env.dockerfile --label "org.opencontainers.image.source=https://github.com/docwire/doctotext" .
	if [[ -v ghcr_login ]]; then
		echo "$ghcr_password" | docker login ghcr.io -u "$ghcr_login" --password-stdin
		docker push ghcr.io/docwire/doctotext_build_env:$docker_image_id || echo "Pushing docker image failed, but building will continue."
	fi
fi

test -t 0 && USE_TTY="-t"
echo USE_TTY=$USE_TTY

arch_arg=$1
build_type=$2

if [ "$arch_arg" = "ARCH=win64" ]; then
	arch="win64"
	BUILD_COMMAND="set -e && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/mingw-w64-x86_64.cmake .. && cmake --build . && cmake --build . --target doxygen install && cd .."
	TEST_COMMAND="set -e && cd build/tests/ && mkdir /tmp/wine && WINEARCH=win64 WINEPREFIX=/tmp/wine WINEPATH=.. ctest -j4 -V --timeout 30 --repeat until-pass:3"

	declare -a LIB_PATHS=("/usr/x86_64-w64-mingw32/bin/libiconv-2.dll"
						  "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll"
						  "/usr/x86_64-w64-mingw32/bin/zlib1.dll"
						  "/usr/x86_64-w64-mingw32/bin/libxml2-2.dll"
						  "/usr/x86_64-w64-mingw32/bin/libwv2-1.dll"
						  "/usr/x86_64-w64-mingw32/bin/libfreetype-6.dll"
						  "/usr/x86_64-w64-mingw32/bin/libpodofo.dll"
						  "/usr/x86_64-w64-mingw32/bin/libhtmlcxx-3.dll"
						  "/usr/x86_64-w64-mingw32/bin/libpff-1.dll"
						  "/usr/x86_64-w64-mingw32/bin/libbfio-1.dll"
						  "/usr/x86_64-w64-mingw32/bin/libtiff-5.dll"
						  "/usr/x86_64-w64-mingw32/bin/liblept-5.dll"
						  "/usr/x86_64-w64-mingw32/bin/libtesseract-5.dll"
						  "/usr/x86_64-w64-mingw32/bin/libjpeg-9.dll"
						  "/usr/x86_64-w64-mingw32/bin/libwebp-7.dll"
						  "/usr/x86_64-w64-mingw32/bin/libwebpmux-3.dll"
						  "/usr/lib/gcc/x86_64-w64-mingw32/10-win32/libgomp-1.dll"
						  "/usr/x86_64-w64-mingw32/bin/libpng16.dll"
						  "/usr/lib/gcc/x86_64-w64-mingw32/10-posix/libgcc_s_seh-1.dll"
						  "/usr/lib/gcc/x86_64-w64-mingw32/10-posix/libstdc++-6.dll"
						  "/usr/x86_64-w64-mingw32/lib/libboost_filesystem.dll"
						  "/usr/x86_64-w64-mingw32/lib/libboost_system.dll"
						  "/usr/x86_64-w64-mingw32/bin/libarchive-13.dll"
						  "/usr/x86_64-w64-mingw32/lib/bz2-1.dll"
						  "/usr/x86_64-w64-mingw32/bin/liblzma-5.dll"
    )

	declare -a LIBS=(	"libiconv-2.dll"
						"libwinpthread-1.dll"
						"zlib1.dll"
						"libxml2-2.dll"
						"libwv2-1.dll"
						"libfreetype-6.dll"
						"libpodofo.dll"
						"libhtmlcxx-3.dll"
						"libpff-1.dll"
						"libbfio-1.dll"
						"libtiff-5.dll"
						"liblept-5.dll"
						"libtesseract-5.dll"
						"libjpeg-9.dll"
						"libwebp-7.dll"
						"libwebpmux-3.dll"
						"libgomp-1.dll"
						"libpng16.dll"
						"libgcc_s_seh-1.dll"
						"libstdc++-6.dll"
						"libarchive-13.dll"
						"bz2-1.dll"
						"liblzma-5.dll"
	)

	STRIP_COMMAND="set -e && cd build && x86_64-w64-mingw32-objcopy --only-keep-debug doctotext_core.dll doctotext_core.dll.debug &&
					x86_64-w64-mingw32-strip --strip-unneeded doctotext_core.dll &&
					x86_64-w64-mingw32-objcopy --add-gnu-debuglink=doctotext_core.dll.debug doctotext_core.dll
					for file in plugins/*.dll;
          do
          x86_64-w64-mingw32-objcopy --only-keep-debug \$file \$file.debug
          x86_64-w64-mingw32-strip --strip-unneeded \$file
          x86_64-w64-mingw32-objcopy --add-gnu-debuglink=\$file.debug \$file
          done
					for i in ${LIBS[@]};
					do
					x86_64-w64-mingw32-objcopy --only-keep-debug \$i \$i.debug
					x86_64-w64-mingw32-strip --strip-unneeded \$i
					x86_64-w64-mingw32-objcopy --add-gnu-debuglink=\$i.debug \$i
					done"
	LIB_EXTENSION='dll'
else
	arch="x86_64_linux"

	BUILD_COMMAND='set -e && cd build && cmake .. && cmake --build . && cmake --build . --target doxygen install && cd ..'
	TEST_COMMAND='set -e && cd build/tests/ && LD_LIBRARY_PATH=.. ctest -j4 -V'
	LIB_EXTENSION='so'

	declare -a LIB_PATHS=("/usr/local/lib/libcharsetdetect.so"
						  "/usr/local/lib/libwv2.so.1"
						  "/usr/local/lib/libpff.so.1"
						  "/usr/local/lib/libbfio.so.1"
						  "/usr/local/lib/libpodofo.so.0.9.7"
						  "/usr/local/lib/libhtmlcxx.so.3"
						  "/usr/lib/x86_64-linux-gnu/libmimetic.so.0"
						  "/usr/lib/x86_64-linux-gnu/liblept.so.5"
						  "/usr/lib/x86_64-linux-gnu/libjpeg.so.62"
						  "/usr/lib/x86_64-linux-gnu/libwebp.so.6"
						  "/usr/lib/x86_64-linux-gnu/libtiff.so.5"
						  "/usr/lib/x86_64-linux-gnu/libjbig.so.0"
						  "/usr/local/lib/libtesseract.so.5"
						  "/usr/lib/x86_64-linux-gnu/libicuuc.so.67"
						  "/usr/lib/x86_64-linux-gnu/libdeflate.so.0"
						  "/usr/lib/x86_64-linux-gnu/libicudata.so.67"
						  "/usr/lib/x86_64-linux-gnu/libgif.so.7"
						  "/usr/lib/libboost_filesystem.so.1.78.0"
						  "/usr/lib/libboost_system.so.1.78.0"
    )


	declare -a LIBS=(	"libcharsetdetect.so"
						"libwv2.so.1"
						"libpff.so.1"
						"libbfio.so.1"
						"libpodofo.so.0.9.7"
						"libhtmlcxx.so.3"
						"libmimetic.so.0"
						"liblept.so.5"
						"libjpeg.so.62"
						"libwebp.so.6"
						"libtiff.so.5"
						"libjbig.so.0"
						"libtesseract.so.5"
						"libicuuc.so.67"
						"libdeflate.so.0"
						"libicudata.so.67"
						"libgif.so.7"
	)

	STRIP_COMMAND="set -e && cd build && objcopy --only-keep-debug libdoctotext_core.so libdoctotext_core.so.debug &&
					strip --strip-unneeded libdoctotext_core.so &&
					objcopy --add-gnu-debuglink=libdoctotext_core.so.debug libdoctotext_core.so &&
					for file in plugins/*.so;
					do
					objcopy --only-keep-debug \$file \$file.debug
          strip --strip-unneeded \$file
          objcopy --add-gnu-debuglink=\$file.debug \$file
          done
					for i in ${LIBS[@]};
					do
					objcopy --only-keep-debug \$i \$i.debug
					strip --strip-unneeded \$i
					objcopy --add-gnu-debuglink=\$i.debug \$i
					done"
fi

COPY_COMMAND="set -e && for i in ${LIB_PATHS[@]};
				  do
				  cp \$i build
				  done
				cp -R /usr/local/share/tessdata build
				mkdir build/resources -p
				cp /usr/local/share/ac16/CMap/* build/resources
				cp /usr/local/share/ag15/CMap/* build/resources
				cp /usr/local/share/aj16/CMap/* build/resources
				cp /usr/local/share/ak12/CMap/* build/resources
				cp /usr/local/share/ToUnicode/* build/resources"

mkdir build -p

docker run --rm \
	-i --init $USE_TTY \
	--user=$UID \
	-v /etc/passwd:/etc/passwd:ro \
	-v `pwd`:`pwd` \
	-w `pwd` \
	ghcr.io/docwire/doctotext_build_env:$docker_image_id \
	bash -c "$BUILD_COMMAND && $COPY_COMMAND && $STRIP_COMMAND"

docker build -t doctotext_test_env:latest -f test_env.dockerfile .
docker run --rm \
	-i --init $USE_TTY \
	--user=$UID \
	-v /etc/passwd:/etc/passwd:ro \
	-v `pwd`:`pwd` \
	-w `pwd` \
	doctotext_test_env:latest \
	bash -c "$TEST_COMMAND"


if [ "$build_type" = "--release" ]; then
	rm -rf build/CMakeFiles build/src build/tests/ build/examples/CMakeFiles build/doc/CMakeFiles
	rm build/Makefile build/CMakeCache.txt build/cmake_install.cmake build/install_manifest.txt build/examples/cmake_install.cmake build/examples/Makefile build/doc/Makefile build/doc/cmake_install.cmake build/doc/Doxyfile.doxygen
fi

cd build/
for i in *.$LIB_EXTENSION*; do
    [ -f "$i" ] || break
    sha1sum $i >> SHA1checksums.sha1
done
cd ..

version=`cat build/VERSION`

tar -cjvf doctotext-$version-$arch.tar.bz2 build
sha1sum doctotext-$version-$arch.tar.bz2 > doctotext-$version-$arch.tar.bz2.sha1
