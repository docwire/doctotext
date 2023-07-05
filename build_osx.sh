set -e
brew update

rm /usr/local/bin/{2to3,2to3-3.11,idle3,idle3.11,pydoc3,pydoc3.11,python3,python3-config,python3.11,python3.11-config}

brew install md5sha1sum automake autogen doxygen

brew install libiconv podofo freetype libxml2 zlib leptonica tesseract

tesseract --list-langs

wget -nc https://sourceforge.net/projects/htmlcxx/files/v0.87/htmlcxx-0.87.tar.gz
echo "ac7b56357d6867f649e0f1f699d9a4f0f03a6e80  htmlcxx-0.87.tar.gz" | shasum -c
tar -xzvf htmlcxx-0.87.tar.gz
cd htmlcxx-0.87
./configure CXXFLAGS=-std=c++17 LDFLAGS="-L/usr/local/opt/libiconv/lib" LIBS="-liconv" CPPFLAGS="-I/usr/local/opt/libiconv/include"
sed -i.bak -e "s/\(allow_undefined=\)yes/\1no/" libtool
sed -i -r -e 's/css\/libcss_parser_pp.la \\//' Makefile
sed -i -r -e 's/css\/libcss_parser.la//' Makefile
sed -i '' -r -e 's/-DDEFAULT_CSS=\"\\\"\$\{datarootdir\}\/htmlcxx\/css\/default.css\\\"\"//' Makefile
sed -i -r -e 's/css\/libcss_parser_pp.la//' Makefile
sed -i -r -e 's/css//' Makefile
sed -i -e 's/throw (Exception)//' html/CharsetConverter.h
sed -i -e 's/throw (Exception)//' html/CharsetConverter.cc
echo 'int main() {}' > htmlcxx.cc
make -j4
make install-strip
cd ..


wget -nc http://silvercoders.com/download/3rdparty/libcharsetdetect-master.tar.bz2
echo "6f9adaf7b6130bee6cfac179e3406cdb933bc83f  libcharsetdetect-master.tar.bz2" | shasum -c
tar -xjvf libcharsetdetect-master.tar.bz2
cd libcharsetdetect-master
cmake -DCMAKE_CXX_STANDARD=17 -DBUILD_SHARED_LIBS=TRUE .
cmake --build . --config Release --target install
cd ..
rm libcharsetdetect-master.tar.bz2


git clone https://github.com/docwire/wv2.git
cd wv2
mkdir build
cd build
cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Debug ..
make install
cd ..
wget http://silvercoders.com/download/3rdparty/wv2-0.2.3_patched_4-private_headers.tar.bz2
echo "6bb3959d975e483128623ee3bff3fba343f096c7  wv2-0.2.3_patched_4-private_headers.tar.bz2" | shasum -c
tar -xjvf wv2-0.2.3_patched_4-private_headers.tar.bz2
mv wv2-0.2.3_patched_4-private_headers/*.h /usr/local/include/wv2/
cd ..

wget -nc http://www.codesink.org/download/mimetic-0.9.7.tar.gz
echo "568557bbf040be2b17595431c9b0992c32fae6ed  mimetic-0.9.7.tar.gz" | shasum -c
tar -zxvf mimetic-0.9.7.tar.gz
wget -nc http://silvercoders.com/download/3rdparty/mimetic-0.9.7-patches.tar.gz
echo "7e203fb2b2d404ae356674e7dfc293e78fe3362b  mimetic-0.9.7-patches.tar.gz" | shasum -c
tar -xvzf mimetic-0.9.7-patches.tar.gz
cd mimetic-0.9.7
patch -p1 -i ../mimetic-0.9.7-patches/register_keyword.patch
patch -p1 -i ../mimetic-0.9.7-patches/ContTokenizer.patch
patch -p1 -i ../mimetic-0.9.7-patches/macro_string.patch
patch -p1 -i ../mimetic-0.9.7-patches/mimetic_pointer_comparison.patch
./configure CXXFLAGS=-std=c++17
make -j4
make install-strip
cd ..
wget https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.gz
echo "273f1be93238a068aba4f9735a4a2b003019af067b9c183ed227780b8f36062c  boost_1_79_0.tar.gz" | shasum -c
tar -xzvf boost_1_79_0.tar.gz
cd boost_1_79_0
./bootstrap.sh
./b2 install link=shared runtime-link=shared --with-filesystem --with-system
cd ..

git clone https://github.com/libyal/libbfio.git
cd libbfio
git checkout 3bb082c
./synclibs.sh
autoreconf -i
./configure
make -j4
make install
cd ..

git clone https://github.com/libyal/libpff.git
cd libpff
git checkout 99a86ef
./synclibs.sh
touch ../../config.rpath
autoreconf -i
./configure
make -j4
make install
cd ..

wget http://www.winimage.com/zLibDll/unzip101e.zip && \
unzip -d unzip101e unzip101e.zip && \
cd unzip101e && \
printf 'cmake_minimum_required(VERSION 3.7)\n' >> CMakeLists.txt
printf 'project(Unzip)\n' >> CMakeLists.txt
printf 'set(UNZIP_SRC ioapi.c unzip.c)\n' >> CMakeLists.txt
printf 'add_library(unzip STATIC ${UNZIP_SRC})\n' >> CMakeLists.txt
printf 'install(FILES unzip.h ioapi.h DESTINATION include)\n' >> CMakeLists.txt
printf 'install(TARGETS unzip DESTINATION lib)\n' >> CMakeLists.txt
printf 'target_compile_options(unzip PRIVATE -fPIC)\n' >> CMakeLists.txt
cmake -DCMAKE_CXX_STANDARD=17 .
cmake --build .
cmake --install .
cd ..

wget https://github.com/libarchive/libarchive/releases/download/v3.6.2/libarchive-3.6.2.tar.gz
tar -xzvf libarchive-3.6.2.tar.gz
cd libarchive-3.6.2 && \
./configure --without-iconv --without-openssl --without-cng --disable-static --enable-dynamic
make
make install-strip
cd ..
rm -rf libarchive-3.6.2

mkdir -p build
cd build
cmake -DCMAKE_CXX_STANDARD=17 ..
cmake --build .
cmake --build . --target doxygen install
cd ..


pwd
cd build/
cp -a /usr/local/share/tessdata/ tessdata/
cp /usr/local/lib/libboost_filesystem.dylib .
cp /usr/local/lib/libboost_system.dylib .
cp /usr/local/lib/libwv2.4.dylib .
cp /usr/local/opt/tesseract/lib/libtesseract.5.dylib .
cp /usr/local/opt/podofo/lib/libpodofo.0.9.8.dylib .
cp /usr/local/opt/freetype/lib/libfreetype.6.dylib .
cp /usr/local/lib/libhtmlcxx.3.dylib .
cp /usr/local/lib/libcharsetdetect.dylib .
cp /usr/local/lib/libmimetic.0.dylib .
cp /usr/local/lib/libbfio.1.dylib .
cp /usr/local/lib/libpff.1.dylib .
cp /usr/local/opt/libpng/lib/libpng16.16.dylib .
cp /usr/local/opt/jpeg-turbo/lib/libjpeg.8.dylib .
cp /usr/local/opt/giflib/lib/libgif.dylib .
cp /usr/local/opt/libtiff/lib/libtiff.6.dylib .
cp /usr/local/opt/webp/lib/libwebpmux.3.dylib .
cp /usr/local/opt/webp/lib/libwebp.7.dylib .
cp /usr/local/opt/openjpeg/lib/libopenjp2.7.dylib .
ls
cd ..

cd build/tests
DYLD_FALLBACK_LIBRARY_PATH=.. ctest -j4 -V
cd ../..

rm -rf build/CMakeFiles build/src build/tests/ build/examples/CMakeFiles build/doc/CMakeFiles

cd build/
for i in *.dylib*; do
    [ -f "$i" ] || break
    sha1sum $i >> SHA1checksums.sha1
done
cd ..

version=`cat build/VERSION`

tar -cjvf doctotext-$version-osx.tar.bz2 build
sha1sum doctotext-$version-osx.tar.bz2 > doctotext-$version-osx.tar.bz2.sha1
