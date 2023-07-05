FROM amd64/debian:bullseye

RUN apt-get update #20211001

RUN echo "Europe/Warsaw" > /etc/timezone && dpkg-reconfigure -f noninteractive tzdata

RUN apt-get install -y locales
RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
	sed -i -e 's/# pl_PL.UTF-8 UTF-8/pl_PL.UTF-8 UTF-8/' /etc/locale.gen && \
	dpkg-reconfigure -f noninteractive locales && \
	update-locale LANG=pl_PL.UTF-8
ENV LANG pl_PL.UTF-8

RUN apt-get install -y g++
RUN apt-get install -y make
RUN apt-get install -y doxygen
RUN apt-get install -y graphviz
RUN apt-get install -y bzip2
RUN apt-get install -y unzip
RUN apt-get install -y pkg-config
RUN apt-get install -y file
RUN apt-get install -y wget
RUN apt-get install -y time
RUN apt-get install -y patch
RUN apt-get install -y git
RUN apt-get install -y headache
RUN apt-get install -y gdb
RUN apt-get install -y valgrind
RUN apt-get install -y cmake # for libchardetect

### Linux 64-bit ###

RUN apt-get install -y zlib1g-dev
RUN apt-get install -y libxml2-dev
RUN apt-get install -y libmimetic-dev
RUN apt-get install -y tesseract-ocr

RUN apt-get install -y libleptonica-dev
RUN apt-get install -y libjpeg62-turbo-dev libjpeg62-turbo libwebp-dev

RUN apt-get install -y autotools-dev
RUN apt-get install -y automake
RUN apt-get install -y autopoint
RUN apt-get install -y libtool

RUN wget -nc https://kumisystems.dl.sourceforge.net/project/wvware/wv2/0.2.3/wv2-0.2.3.tar.bz2 && \
	echo "8378f43806bc2614ccea2b265159ce0548192601  wv2-0.2.3.tar.bz2" | sha1sum -c && \
	tar -xjvf wv2-0.2.3.tar.bz2 && \
	wget -nc http://silvercoders.com/download/3rdparty/wv2-0.2.3-patches_4.tar.bz2 && \
	echo "60efd20e0374c021d8eb145b6cede46b6c996689  wv2-0.2.3-patches_4.tar.bz2" | sha1sum -c && \
	tar -xjvf wv2-0.2.3-patches_4.tar.bz2 && \
	cd wv2-0.2.3 && \
	patch -d src -i ../../wv2-0.2.3-patches/wv2_ustring_expotential_growth.patch && \
	patch -p0 -i ../wv2-0.2.3-patches/wv2_parentstyle_upechpx_fix.patch && \
	sed -i 's/#include <algorithm>/#include <algorithm>\n#include <string.h>/' src/styles.cpp && \
	sed -i 's|#include <glib/giochannel.h>|#include <glib.h>|' src/olestream.h && \
	patch -p1 -i ../wv2-0.2.3-patches/abstract_ole_classess_and_fixed_ustring.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/ustring_doesnt_share_memory_any_more.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/no_more_glib_nor_libgsf.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/simple_fix_for_encrypted_files.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/wv2_vector_fix.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/fixed_infinite_loop.patch && \
	./configure --enable-static --disable-debug CPPFLAGS=-std=c++17 && \
	make -j4 && \
	make install-strip && \
	cd .. && \
	rm -rf wv2-0.2.3 && \
	wget http://silvercoders.com/download/3rdparty/wv2-0.2.3_patched_4-private_headers.tar.bz2 && \
	echo "6bb3959d975e483128623ee3bff3fba343f096c7  wv2-0.2.3_patched_4-private_headers.tar.bz2" | sha1sum -c && \
	tar -xjvf wv2-0.2.3_patched_4-private_headers.tar.bz2 && \
	mv wv2-0.2.3_patched_4-private_headers/*.h /usr/local/include/wv2/ && \
        rm -rf wv2-0.2.3_patched_4-private_headers

RUN wget -nc http://silvercoders.com/download/3rdparty/libcharsetdetect-master.tar.bz2 && \
	echo "6f9adaf7b6130bee6cfac179e3406cdb933bc83f  libcharsetdetect-master.tar.bz2" | sha1sum -c && \
	tar -xjvf libcharsetdetect-master.tar.bz2 && \
	cd libcharsetdetect-master && \
	./configure -DCMAKE_CXX_STANDARD=17 && \
	make -j4 && \
	cd .. && \
	mv ./libcharsetdetect-master/charsetdetect.h /usr/local/include/ && \
	mv ./libcharsetdetect-master/build/* /usr/local/lib/ && \
	rm -rf libcharsetdetect-master

RUN apt-get install -y libfreetype6-dev
RUN apt-get install -y libfontconfig-dev

RUN wget -nc https://netcologne.dl.sourceforge.net/project/podofo/podofo/0.9.7/podofo-0.9.7.tar.gz && \
	echo "7fe8d5cd60ca00d05f17eb11bedfd6a3db758fdf  podofo-0.9.7.tar.gz" | sha1sum -c && \
	tar -xzvf podofo-0.9.7.tar.gz && \
	cd podofo-0.9.7 && \
	cmake -DPODOFO_BUILD_STATIC=FALSE -DPODOFO_BUILD_SHARED=TRUE -DCMAKE_CXX_STANDARD=17 . && \
	make -j4 && \
	make install && \
	cd .. && \
	rm -rf podofo-0.9.7

RUN wget -nc https://sourceforge.net/projects/htmlcxx/files/v0.87/htmlcxx-0.87.tar.gz && \
	echo "ac7b56357d6867f649e0f1f699d9a4f0f03a6e80 htmlcxx-0.87.tar.gz" | sha1sum -c && \
	tar -xzvf htmlcxx-0.87.tar.gz && \
	cd htmlcxx-0.87 && \
	./configure LDFLAGS="-Wl,-no-undefined" CPPFLAGS=-std=c++17 && \
    sed -i.bak -e "s/\(allow_undefined=\)yes/\1no/" libtool && \
    sed -i -r 's/css\/libcss_parser_pp.la \\//' Makefile && \
    sed -i -r 's/css\/libcss_parser.la//' Makefile && \
    sed -i -r 's/-DDEFAULT_CSS=\"\\\"\$\{datarootdir\}\/htmlcxx\/css\/default.css\\\"\"//' Makefile && \
    sed -i -r 's/css\/libcss_parser_pp.la//' Makefile && \
    sed -i -r 's/css//' Makefile && \
	sed -i 's/throw (Exception)//' html/CharsetConverter.h && \
	sed -i 's/throw (Exception)//' html/CharsetConverter.cc && \
	echo 'int main() {}' > htmlcxx.cc && \
	make -j4 && \
	make install-strip && \
	cd .. && \
	rm -rf htmlcxx-0.87

RUN git clone https://github.com/libyal/libbfio.git
RUN cd libbfio && \
    git checkout 3bb082c && \
    ./synclibs.sh && \
    ./autogen.sh && \
    ./configure && \
    make -j4 && \
    make install && \
    cd .. && \
    rm -rf libbfio


RUN git clone https://github.com/libyal/libpff.git
RUN cd libpff && \
    git checkout 99a86ef && \
    ./synclibs.sh && \
    sed -i "s/2.71/2.69/" configure.ac && \
    ./autogen.sh && \
    ./configure && \
    make -j4 && \
    make install && \
    cd .. && \
    rm -rf libpff

### Win 64-bit ###

RUN apt-get install -y g++-mingw-w64-x86-64
RUN apt-get install -y wine
RUN apt-get install -y libz-mingw-w64-dev

RUN git clone https://github.com/libyal/libbfio.git
RUN cd libbfio && \
    git checkout 3bb082c && \
    ./synclibs.sh && \
    ./autogen.sh && \
    ./configure --host=x86_64-w64-mingw32 --prefix=/usr/x86_64-w64-mingw32 && \
    make -j4 && \
    make install && \
    cd .. && \
    rm -rf libbfio


RUN git clone https://github.com/libyal/libpff.git
RUN cd libpff && \
    git checkout 99a86ef && \
    ./synclibs.sh && \
    sed -i "s/2.71/2.69/" configure.ac && \
    ./autogen.sh && \
    ./configure libbfio_LIBS='-L/usr/x86_64-w64-mingw32/bin/libbfio-1.dll -lbfio' \
			--host=x86_64-w64-mingw32 \
			--prefix=/usr/x86_64-w64-mingw32 && \
    make -j4 && \
    make install && \
    cd ..


RUN wget -nc ftp://xmlsoft.org/libxml2/libxml2-2.9.10.tar.gz && \
	echo "db6592ec9ca9708c4e71bf6bfd907bbb5cd40644 libxml2-2.9.10.tar.gz" | sha1sum -c && \
	tar -xzvf libxml2-2.9.10.tar.gz && \
	cd libxml2-2.9.10 && \
	sed -i 's/pei\*-i386(\.\*architecture: i386)\?/(pei*-i386(.*architecture: i386)?|pe-arm-wince|pe-x86-64)/' configure && \
	sed -i 's/pe-i386(\.\*architecture: i386)\?/(pei*-i386(.*architecture: i386)?|pe-arm-wince|pe-x86-64)/' ltmain.sh && \
	CPPFLAGS="-DIN_LIBXML" ./configure --host=x86_64-w64-mingw32 --without-python \
		--prefix=/usr/x86_64-w64-mingw32 CPPFLAGS=-std=c++17 && \
	make -j4 && \
	make install-strip && \
	cd .. && \
	rm -rf libxml2-2.9.10

RUN wget -nc http://silvercoders.com/download/3rdparty/libcharsetdetect-master.tar.bz2 && \
	echo "6f9adaf7b6130bee6cfac179e3406cdb933bc83f  libcharsetdetect-master.tar.bz2" | sha1sum -c && \
	tar -xjvf libcharsetdetect-master.tar.bz2 && \
	wget -nc http://silvercoders.com/download/3rdparty/libcharsetdetect-1.0-win_files.tar.bz2 && \
	echo "2e8cc4b11d9e957b17278e16dff00793c91f8c79  libcharsetdetect-1.0-win_files.tar.bz2" | sha1sum -c && \
	tar -xjvf libcharsetdetect-1.0-win_files.tar.bz2 && \
	cp ./libcharsetdetect-1.0-win_files/win64.toolchain ./libcharsetdetect-master/cmake-toolchains/ && \
	cd libcharsetdetect-master && \
	patch -p0 -i ../libcharsetdetect-1.0-win_files/win64.patch && \
	{ \
		cmake -DCMAKE_TOOLCHAIN_FILE=cmake-toolchains/win64.toolchain -DCMAKE_CXX_STANDARD=17 . || \
		{ \
			rm -rf CMakeFiles && \
			cmake -DCMAKE_TOOLCHAIN_FILE=cmake-toolchains/win64.toolchain -DCMAKE_CXX_STANDARD=17 . ; \
		} \
	} && \
	make -j4 && \
	cd .. && \
	mv ./libcharsetdetect-master/charsetdetect.h /usr/x86_64-w64-mingw32/include/ && \
	mv ./libcharsetdetect-master/build/* /usr/x86_64-w64-mingw32/lib/ && \
	rm -rf libcharsetdetect-master && \
	rm -rf libcharsetdetect-1.0-win_files

RUN wget -nc http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.16.tar.gz && \
	echo "2d9d1d8fa9f7859b181de984d60eacd2499a5701 libiconv-1.16.tar.gz" | sha1sum -c && \
	tar -xzvf libiconv-1.16.tar.gz && \
	cd libiconv-1.16 && \
	./configure --host=x86_64-w64-mingw32 \
		--prefix=/usr/x86_64-w64-mingw32 CPPFLAGS=-std=c++17 && \
	make -j4 && \
	x86_64-w64-mingw32-strip bin/* && \
	make install && \
	cd .. && \
	rm -rf libiconv-1.16

RUN wget -nc http://downloads.sourceforge.net/project/libpng/zlib/1.2.7/zlib-1.2.7.tar.bz2 && \
	echo "858818fe6d358ec682d54ac5e106a2dd62628e7f zlib-1.2.7.tar.bz2" | sha1sum -c && \
	tar -xjvf zlib-1.2.7.tar.bz2 && \
	cd zlib-1.2.7 && \
	make -j4 -f win32/Makefile.gcc PREFIX=x86_64-w64-mingw32- && \
	make -j4 -f win32/Makefile.gcc PREFIX=x86_64-mingw32- DESTDIR="/usr/x86_64-w64-mingw32" \
		SHARED_MODE=1 INCLUDE_PATH=/include LIBRARY_PATH=/lib BINARY_PATH=/bin install && \
	cd .. && \
	rm -rf zlib-1.2.7

RUN wget -nc https://sourceforge.net/projects/htmlcxx/files/v0.87/htmlcxx-0.87.tar.gz && \
	echo "ac7b56357d6867f649e0f1f699d9a4f0f03a6e80 htmlcxx-0.87.tar.gz" | sha1sum -c && \
	tar -xzvf htmlcxx-0.87.tar.gz && \
	cd htmlcxx-0.87 && \
	./configure LDFLAGS="-Wl,-no-undefined" --host=x86_64-w64-mingw32 LIBS="-liconv" --prefix=/usr/x86_64-w64-mingw32 CPPFLAGS=-std=c++17 && \
    sed -i.bak -e "s/\(allow_undefined=\)yes/\1no/" libtool && \
    sed -i -r 's/css\/libcss_parser_pp.la \\//' Makefile && \
    sed -i -r 's/css\/libcss_parser.la//' Makefile && \
    sed -i -r 's/-DDEFAULT_CSS=\"\\\"\$\{datarootdir\}\/htmlcxx\/css\/default.css\\\"\"//' Makefile && \
    sed -i -r 's/css\/libcss_parser_pp.la//' Makefile && \
    sed -i -r 's/css//' Makefile && \
	sed -i 's/throw (Exception)//' html/CharsetConverter.h && \
	sed -i 's/throw (Exception)//' html/CharsetConverter.cc && \
	echo 'int main() {}' > htmlcxx.cc && \
	make -j4 && \
	make install-strip && \
	cd .. && \
	rm -rf htmlcxx-0.87


RUN wget -nc http://www.codesink.org/download/mimetic-0.9.7.tar.gz && \
	echo "568557bbf040be2b17595431c9b0992c32fae6ed  mimetic-0.9.7.tar.gz" | sha1sum -c && \
	tar -zxvf mimetic-0.9.7.tar.gz && \
	sed -i mimetic-0.9.7/mimetic/tokenizer.h -e "150s/setDelimList(delims);/this->setDelimList(delims);/" && \
	cd mimetic-0.9.7 && \
	./configure --host=x86_64-w64-mingw32 \
		--prefix=/usr/x86_64-w64-mingw32 CPPFLAGS=-std=c++17 && \
	sed -i 's/am_libmimeticos_la_OBJECTS = file_iterator.lo fileop.lo mmfile.lo/am_libmimeticos_la_OBJECTS = file_iterator.lo fileop.lo/g' mimetic/os/Makefile && \
	sed -i 's/SUBDIRS = mimetic doc examples test win32/SUBDIRS = mimetic doc examples win32/g' Makefile && \
	make -j4 && \
	make install-strip && \
	cp mimetic/config_win32.h /usr/x86_64-w64-mingw32/include/mimetic/ && \
	cd .. && \
	rm -rf mimetic-0.9.7

RUN wget -nc https://kumisystems.dl.sourceforge.net/project/wvware/wv2/0.2.3/wv2-0.2.3.tar.bz2 && \
	echo "8378f43806bc2614ccea2b265159ce0548192601  wv2-0.2.3.tar.bz2" | sha1sum -c && \
	tar -xjvf wv2-0.2.3.tar.bz2 && \
	wget -nc http://silvercoders.com/download/3rdparty/wv2-0.2.3-patches_4.tar.bz2 && \
	echo "60efd20e0374c021d8eb145b6cede46b6c996689  wv2-0.2.3-patches_4.tar.bz2" | sha1sum -c && \
	tar -xjvf wv2-0.2.3-patches_4.tar.bz2 && \
	cd wv2-0.2.3 && \
	patch -i ../wv2-0.2.3-patches/wv2_configure_no_try_run.patch && \
	patch -d src -i ../../wv2-0.2.3-patches/wv2_ustring_expotential_growth.patch && \
	patch -p0 -i ../wv2-0.2.3-patches/wv2_parentstyle_upechpx_fix.patch && \
	sed -i 's/finite/finitef/' src/ustring.cpp && \
	sed -i 's/#include <algorithm>/#include <algorithm>\n#include <string.h>/' src/styles.cpp && \
	sed -i 's|#include <glib/giochannel.h>|#include <glib.h>|' src/olestream.h && \
	sed -i '/const wvnodebugstream& operator<<( const std::string& ) const { return \*this; }/ aconst wvnodebugstream& operator<<( size_t ) const { return *this; }' src/wvlog.h && \
	sed -i 's/pe-i386(\.\*architecture: i386)\?/(pei*-i386(.*architecture: i386)?|pe-arm-wince|pe-x86-64)/' ltmain.sh && \
	sed -i 's/-no-undefined)/-Wl,--no-undefined)/' ltmain.sh && \
	patch -p1 -i ../wv2-0.2.3-patches/abstract_ole_classess_and_fixed_ustring.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/ustring_doesnt_share_memory_any_more.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/no_more_glib_nor_libgsf.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/simple_fix_for_encrypted_files.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/wv2_vector_fix.patch && \
	patch -p1 -i ../wv2-0.2.3-patches/fixed_infinite_loop.patch && \
	sed -i 's/pei\*-i386(\.\*architecture: i386)\?/(pei*-i386(.*architecture: i386)?|pe-arm-wince|pe-x86-64)/' configure && \
	PKG_CONFIG_SYSROOT_DIR='!disabled' lt_cv_deplibs_check_method="pass_all" ./configure --host=x86_64-w64-mingw32 \
		LDFLAGS="-Wl,--no-undefined" \
	        --enable-static --disable-debug \
		--prefix=/usr/x86_64-w64-mingw32 CPPFLAGS=-std=c++17 && \
	make -j4 && \
	make install-strip && \
	cd .. && \
	rm -rf wv2-0.2.3 && \
	wget http://silvercoders.com/download/3rdparty/wv2-0.2.3_patched_4-private_headers.tar.bz2 && \
	echo "6bb3959d975e483128623ee3bff3fba343f096c7  wv2-0.2.3_patched_4-private_headers.tar.bz2" | sha1sum -c && \
	tar -xjvf wv2-0.2.3_patched_4-private_headers.tar.bz2 && \
	mv wv2-0.2.3_patched_4-private_headers/*.h /usr/x86_64-w64-mingw32/include/wv2/ && \
        rm -rf wv2-0.2.3_patched_4-private_headers

RUN wget -nc https://download.savannah.gnu.org/releases/freetype/freetype-2.7.tar.bz2 && \
	echo "f251029d5ab542a96dd4531605fe577185077560  freetype-2.7.tar.bz2" | sha1sum -c && \
	tar -xjvf freetype-2.7.tar.bz2 && \
	cd freetype-2.7 && \
	./configure --host=x86_64-w64-mingw32 \
		--prefix=/usr/x86_64-w64-mingw32 --with-png=no CPPFLAGS=-std=c++17 && \
	make -j4 && \
	make install && \
	cd .. && \
	rm -rf freetype-2.7

RUN wget -nc https://netcologne.dl.sourceforge.net/project/podofo/podofo/0.9.7/podofo-0.9.7.tar.gz && \
	echo "f251029d5ab542a96dd4531605fe577185077560  freetype-2.7.tar.bz2" | sha1sum -c && \
	tar -xzvf podofo-0.9.7.tar.gz && \
	cd podofo-0.9.7 && \
	cmake -DPODOFO_BUILD_STATIC=FALSE -DPODOFO_BUILD_SHARED=TRUE -DCMAKE_VERBOSE_MAKEFILE=TRUE \
		-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
		-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_RC_COMPILER="x86_64-w64-mingw32-windres" \
		-DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
		-DCMAKE_INSTALL_PREFIX=/usr/x86_64-w64-mingw32 -DCMAKE_CXX_STANDARD=17 . && \
	make -j4 && \
	make install && \
	cd .. && \
	rm -rf podofo-0.9.7

RUN wget https://sourceforge.net/projects/libpng/files/libpng16/1.6.37/libpng-1.6.37.tar.xz && \
	echo '3ab93fabbf4c27e1c4724371df408d9a1bd3f656  libpng-1.6.37.tar.xz' | sha1sum -c && \
	tar -xf libpng-1.6.37.tar.xz && \
	ls && \
	cd libpng-1.6.37 && \
	./configure --host=x86_64-w64-mingw32  \
			--prefix=/usr/x86_64-w64-mingw32 && \
	mkdir build && \
	cd build && \
	cmake -DCMAKE_VERBOSE_MAKEFILE=TRUE \
		-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
		-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_RC_COMPILER="x86_64-w64-mingw32-windres" \
		-DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
		-DCMAKE_INSTALL_PREFIX=/usr/x86_64-w64-mingw32 .. && \
	make -j4 && \
	make install && \
	ldconfig && \
	cd ../.. && \
	rm -rf libpng-1.6.37

RUN wget https://www.ijg.org/files/jpegsrc.v9.tar.gz && \
	echo '79a04628ae49b59f561928962a011447e72c3817 jpegsrc.v9.tar.gz' | sha1sum -c && \
	tar -xzvf jpegsrc.v9.tar.gz && \
	cd jpeg-9 && \
	./configure --host=x86_64-w64-mingw32 \
			--prefix=/usr/x86_64-w64-mingw32 && \
	make -j4 && \
	make install && \
	ldconfig && \
	cd .. && \
	rm -rf jpegsrc.v9

RUN wget https://download.osgeo.org/libtiff/tiff-4.3.0.tar.gz && \
	echo 'c03f6cb05b64ff496b3a5c733a0af6296aead051  tiff-4.3.0.tar.gz' | sha1sum -c && \
	tar -xzvf tiff-4.3.0.tar.gz && \
	cd tiff-4.3.0 && \
	./autogen.sh && \
	./configure --host=x86_64-w64-mingw32 \
			--prefix=/usr/x86_64-w64-mingw32 && \
	make -j4 && \
	make install && \
	cd .. && \
	rm -rf tiff-4.3.0

RUN apt-get install -y libtool autotools-dev automake

RUN wget https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-1.2.1.tar.gz && \
	echo '9bbc3cb6f90dccafbd3e39325c28f24ac3f6e041  libwebp-1.2.1.tar.gz' | sha1sum -c && \
	tar -xzvf libwebp-1.2.1.tar.gz && \
	cd libwebp-1.2.1 && \
	./autogen.sh && \
	./configure -host=x86_64-w64-mingw32 \
			--prefix=/usr/x86_64-w64-mingw32 && \
	make -j4 && \
	make install && \
	ldconfig && \
	cd .. && \
	rm -rf libwebp-1.2.1

RUN wget http://www.leptonica.org/source/leptonica-1.82.0.tar.gz && \
	echo '0566b433f760e886617a25ac94c52ebc954c06a7  leptonica-1.82.0.tar.gz' | sha1sum -c && \
	tar -xzvf leptonica-1.82.0.tar.gz && \
	cd leptonica-1.82.0 && \
	./configure --host=x86_64-w64-mingw32  \
			--prefix=/usr/x86_64-w64-mingw32 && \
	make -j4 && \
	make install && \
	ldconfig && \
	cd .. && \
	rm -rf leptonica-1.82.0

RUN apt-get remove -y libarchive-dev


RUN update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
RUN update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
RUN wget https://github.com/tesseract-ocr/tesseract/archive/refs/tags/5.1.0.tar.gz && \
	echo '9e91b535e58849b465fcd7c4b2129be082a9d8e2  5.1.0.tar.gz' | sha1sum -c && \
	tar -xzvf 5.1.0.tar.gz && \
	cd tesseract-5.1.0 && \
	ln -s /usr/x86_64-w64-mingw32/lib/libws2_32.a /usr/x86_64-w64-mingw32/lib/libWs2_32.a && \
	./autogen.sh && \
	./configure LEPTONICA_LIBS='-L/usr/x86_64-w64-mingw32/bin/liblept-5.dll -llept' --host=x86_64-w64-mingw32 \
			--prefix=/usr/x86_64-w64-mingw32 CPPFLAGS=-std=c++17 && \
	make -j4 && \
	make install && \
	ldconfig && \
	cd .. && \
	rm -rf  tesseract-5.1.0

RUN update-alternatives --remove x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
RUN update-alternatives --remove x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix

RUN wget https://github.com/tesseract-ocr/tesseract/archive/refs/tags/5.1.0.tar.gz && \
	echo '9e91b535e58849b465fcd7c4b2129be082a9d8e2  5.1.0.tar.gz' | sha1sum -c && \
	tar -xzvf 5.1.0.tar.gz && \
	cd tesseract-5.1.0 && \
	./autogen.sh && \
	./configure CPPFLAGS=-std=c++17 && \
	make -j4 && \
	make install && \
	ldconfig && \
	cd .. && \
	rm -rf tesseract-5.1.0

RUN apt-get install -y libpng-dev libtiff-dev

RUN wget https://github.com/tesseract-ocr/tessdata_fast/raw/main/eng.traineddata && \
    echo "31abd495e0f719db4f524c447e9d855124a0b0d6  eng.traineddata" | sha1sum -c && \
    mv eng.traineddata /usr/local/share/tessdata
RUN wget https://github.com/tesseract-ocr/tessdata_fast/raw/main/osd.traineddata && \
    echo "b095cb28b6c868b99d19e1c64b48a626bc4cb944  osd.traineddata" | sha1sum -c && \
    mv osd.traineddata /usr/local/share/tessdata
RUN wget https://github.com/tesseract-ocr/tessdata_fast/raw/main/pol.traineddata && \
    echo "17d17f2b807317106c0ff15c9cf8cf7f97b561e5  pol.traineddata" | sha1sum -c && \
    mv pol.traineddata /usr/local/share/tessdata

RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.tar.bz2 && \
    echo "7ccc47e82926be693810a687015ddc490b49296d  boost_1_78_0.tar.bz2" | sha1sum -c && \
    tar -xvf boost_1_78_0.tar.bz2 && \
    cd boost_1_78_0 && \
    ./bootstrap.sh --prefix=/usr/ && \
    ./b2 install link=shared runtime-link=shared --with-filesystem --with-system && \
    cd ..

RUN cd boost_1_78_0 && \
    ./bootstrap.sh mingw toolset=gcc target-os=windows --prefix=/usr/x86_64-w64-mingw32/ && \
    sed -i 's/using gcc ;/using gcc : : x86_64-w64-mingw32-g++-posix ;/' project-config.jam && \
    ./b2 install toolset=gcc target-os=windows address-model=64 variant=profile link=shared runtime-link=shared --with-filesystem --with-system && \
    cd .. && \
    rm -rf boost_1_78_0

RUN wget http://www.winimage.com/zLibDll/unzip101e.zip && \
	unzip -d unzip101e unzip101e.zip && \
	cd unzip101e && \
	echo \
		'cmake_minimum_required(VERSION 3.7)\n' \
		'project(Unzip)\n' \
		'set(UNZIP_SRC ioapi.c unzip.c)\n' \
		'set(FLAGS -fPIC)\n' \
		'add_library(unzip STATIC ${UNZIP_SRC})\n' \
		'install(FILES unzip.h ioapi.h DESTINATION include)\n' \
		'install(TARGETS unzip DESTINATION lib)\n' \
		'target_compile_options(unzip PRIVATE -fPIC)\n' \
		> CMakeLists.txt && \
	cat CMakeLists.txt && \
	cmake \
		-DCMAKE_CXX_STANDARD=17 \
		. && \
	cmake --build . && \
	cmake --install . && \
	cd .. && \
	rm -rf unzip101e

RUN wget http://www.winimage.com/zLibDll/unzip101e.zip && \
	unzip -d unzip101e unzip101e.zip && \
	cd unzip101e && \
	echo \
		'cmake_minimum_required(VERSION 3.7)\n' \
		'project(Unzip)\n' \
		'set(UNZIP_SRC ioapi.c unzip.c)\n' \
		'set(FLAGS -fPIC)\n' \
		'add_library(unzip STATIC ${UNZIP_SRC})\n' \
		'install(FILES unzip.h ioapi.h DESTINATION include)\n' \
		'install(TARGETS unzip DESTINATION lib)\n' \
		'target_compile_options(unzip PRIVATE -fPIC)\n' \
		> CMakeLists.txt && \
	cat CMakeLists.txt && \
	cmake \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
		-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
		-DCMAKE_INSTALL_PREFIX=/usr/x86_64-w64-mingw32 \
		-DCMAKE_CXX_STANDARD=17 \
		. && \
	cmake --build . && \
	cmake --install . && \
	cd .. && \
	rm -rf unzip101e

RUN wget http://silvercoders.com/download/3rdparty/cmapresources_korean1-2.tar.z && \
	echo "e4e36995cff0331d8bd5ad00c1c1453c24ab4c07  cmapresources_korean1-2.tar.z" | sha1sum -c - && \
	tar -xvf cmapresources_korean1-2.tar.z && \
	mv ak12 /usr/local/share/

RUN wget http://silvercoders.com/download/3rdparty/cmapresources_japan1-6.tar.z && \
	echo "9467d7ed73c16856d2a49b5897fc5ea477f3a111  cmapresources_japan1-6.tar.z" | sha1sum -c - && \
	tar -xvf cmapresources_japan1-6.tar.z && \
	mv aj16 /usr/local/share/

RUN wget http://silvercoders.com/download/3rdparty/cmapresources_gb1-5.tar.z && \
	echo "56e6cbd9e053185f9e00118e54fd5159ca118b39  cmapresources_gb1-5.tar.z" | sha1sum -c - && \
	tar -xvf cmapresources_gb1-5.tar.z && \
	mv ag15 /usr/local/share/

RUN wget http://silvercoders.com/download/3rdparty/cmapresources_cns1-6.tar.z && \
	echo "80c92cc904c9189cb9611741b913ffd22bcd4036  cmapresources_cns1-6.tar.z" | sha1sum -c - && \
	tar -xvf cmapresources_cns1-6.tar.z && \
	mv ac16 /usr/local/share/

RUN wget http://silvercoders.com/download/3rdparty/mappingresources4pdf_2unicode_20091116.tar.Z && \
	echo "aaf44cb1e5dd2043c932e641b0e41432aee2ca0d  mappingresources4pdf_2unicode_20091116.tar.Z" | sha1sum -c - && \
	tar -xvf mappingresources4pdf_2unicode_20091116.tar.Z && \
	mv ToUnicode /usr/local/share/

RUN apt-get install -y libarchive-dev

RUN wget https://github.com/libarchive/bzip2/archive/1ea1ac188ad4b9cb662e3f8314673c63df95a589.zip && \
	unzip 1ea1ac188ad4b9cb662e3f8314673c63df95a589.zip && \
	cd bzip2-1ea1ac188ad4b9cb662e3f8314673c63df95a589 && \
	cmake \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
		-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
		-DCMAKE_INSTALL_PREFIX=/usr/x86_64-w64-mingw32 \
		-DCMAKE_CXX_STANDARD=17 \
		. && \
	cmake --build . --config Release && \
	cmake --install . && \
	cp /usr/x86_64-w64-mingw32/lib/libbz2.dll /usr/x86_64-w64-mingw32/lib/bz2-1.dll && \
	cd .. && \
	rm -rf bzip2-1ea1ac188ad4b9cb662e3f8314673c63df95a589

RUN wget https://github.com/libarchive/xz/archive/refs/tags/v5.2.6.tar.gz && \
	tar -xzvf v5.2.6.tar.gz && \
	cd xz-5.2.6 && \
	#cmake \
	#	-DCMAKE_SYSTEM_NAME=Windows \
	#	-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
	#	-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
	#	-DCMAKE_INSTALL_PREFIX=/usr/x86_64-w64-mingw32 \
	#	-DCMAKE_CXX_STANDARD=17 \
	#	. && \
	#cmake --build . --config Release && \
	#cmake --install . && \
	./autogen.sh --no-po4a && \
	./configure --host=x86_64-w64-mingw32 --prefix=/usr/x86_64-w64-mingw32 && \
	make -j4 && \
	make install-strip && \
	cd .. && \
	rm -rf xz-5.2.6

RUN wget https://github.com/libarchive/libarchive/releases/download/v3.6.2/libarchive-3.6.2.tar.gz && \
	tar -xzvf libarchive-3.6.2.tar.gz && \
	cd libarchive-3.6.2 && \
	./configure --host=x86_64-w64-mingw32 --prefix=/usr/x86_64-w64-mingw32 --without-iconv --without-openssl --without-cng --disable-static --enable-dynamic && \
	make && \
	make install-strip && \
	cd .. && \
	rm -rf libarchive-3.6.2
