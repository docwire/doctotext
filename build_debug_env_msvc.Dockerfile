# escape=`

# Use the latest Windows Server Core 2019 image.
FROM mcr.microsoft.com/windows/servercore:ltsc2019

# Restore the default Windows shell for correct batch processing.
SHELL ["cmd", "/S", "/C"]

RUN echo "build image"

RUN `
    # Download the Build Tools bootstrapper.
    curl -SL --output vs_buildtools.exe https://aka.ms/vs/17/release/vs_buildtools.exe `
    `
    # Install Build Tools with the Microsoft.VisualStudio.Workload.AzureBuildTools workload, excluding workloads and components with known issues.
    && (start /w vs_buildtools.exe --quiet --wait --norestart --nocache `
        --installPath "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools" `
        --add Microsoft.VisualStudio.Workload.AzureBuildTools `
        --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended `
        --remove Microsoft.VisualStudio.Component.Windows10SDK.10240 `
        --remove Microsoft.VisualStudio.Component.Windows10SDK.10586 `
        --remove Microsoft.VisualStudio.Component.Windows10SDK.14393 `
        --remove Microsoft.VisualStudio.Component.Windows81SDK `
        || IF "%ERRORLEVEL%"=="3010" EXIT 0) `
    `
    # Cleanup
    && del /q vs_buildtools.exe

RUN powershell Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; $env:chocolateyVersion='1.4.0'; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

RUN powershell choco install cmake -y --installargs 'ADD_CMAKE_TO_PATH=System'
RUN powershell choco install git -y

RUN call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat"

RUN git clone https://github.com/Microsoft/vcpkg.git

RUN cd vcpkg`
    && git config --global --add safe.directory C:/vcpkg`
    && git checkout tags/2022.08.15`
    && call ".\bootstrap-vcpkg.bat"

RUN vcpkg\vcpkg install zlib:x64-windows

RUN powershell choco install archiver -y

RUN vcpkg\vcpkg install freetype:x64-windows

RUN vcpkg\vcpkg install podofo:x64-windows

RUN vcpkg\vcpkg install boost-filesystem:x64-windows

RUN vcpkg\vcpkg install boost-system:x64-windows

RUN vcpkg\vcpkg install pthreads:x64-windows

RUN powershell choco install doxygen.install -y

RUN powershell choco install graphviz -y

RUN powershell choco install python -y

RUN mkdir include `
    && mkdir lib

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\zlib_x64-windows\include\*.h' -Destination 'C:\include' -Recurse

RUN vcpkg\vcpkg install boost-signals2:x64-windows

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\boost-signals2_x64-windows\include\boost\*' -Destination 'C:\include\boost' -Recurse

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\pthreads_x64-windows\include\*.h' -Destination 'C:\include'

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\boost-system_x64-windows\include\boost\*' -Destination 'C:\include\boost' -Recurse

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\boost-filesystem_x64-windows\include\boost\*' -Destination 'C:\include\boost' -Recurse

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\podofo_x64-windows\include\podofo' -Destination 'C:\include' -Recurse

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\freetype_x64-windows\include\*.h' -Destination 'C:\include'

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\freetype_x64-windows\include\freetype' -Destination 'C:\include' -Recurse

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\zlib_x64-windows\include\*.h' -Destination 'C:\include'

RUN powershell vcpkg\vcpkg install boost-config:x64-windows

RUN powershell vcpkg\vcpkg install boost-dll:x64-windows

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\boost-config_x64-windows\include\boost\*' -Destination 'C:\include\boost' -Recurse -Force

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\boost-dll_x64-windows\include\boost\*' -Destination 'C:\include\boost' -Recurse

RUN powershell vcpkg\vcpkg install boost-assert:x64-windows

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\boost-assert_x64-windows\include\boost\*' -Destination 'C:\include\boost' -Recurse

RUN powershell vcpkg\vcpkg install boost-smart-ptr:x64-windows

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\boost*\include\boost\*' -Destination 'C:\include\boost' -Recurse -Force

RUN powershell vcpkg\vcpkg install libpff:x64-windows

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\libpff_x64-windows\include\libpff' -Destination 'C:\include' -Recurse

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\libpff_x64-windows\include\libpff.h' -Destination 'C:\include'

RUN git clone https://github.com/docwire/mimetic.git `
    && powershell Invoke-WebRequest -Uri https://raw.githubusercontent.com/richiware/mimetic/master/CMakeLists.txt -OutFile C:\mimetic\CMakeLists.txt `
    && powershell Invoke-WebRequest -Uri https://raw.githubusercontent.com/richiware/mimetic/master/win-vs140.cmake -OutFile C:\mimetic\win-vs140.cmake `
    && cd mimetic `
    && mkdir cmake `
    && powershell Invoke-WebRequest -Uri https://raw.githubusercontent.com/richiware/mimetic/master/cmake/config.cmake.in -OutFile C:\mimetic\cmake\config.cmake.in `
    && mkdir build `
    && cd build `
    && cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_INSTALL_PREFIX:PATH="C:\\" `
    && cmake --build . --config Debug `
    && cmake --build . --config Debug --target install `
    && cd ..\..

# libs
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\podofo_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse `
    && powershell Copy-Item -Path 'C:\vcpkg\packages\freetype_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force`
    && powershell Copy-Item -Path 'C:\vcpkg\packages\pthreads_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force`
    && powershell Copy-Item -Path 'C:\vcpkg\packages\zlib_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force`
    && powershell Copy-Item -Path 'C:\vcpkg\packages\boost-filesystem_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force`
    && powershell Copy-Item -Path 'C:\vcpkg\packages\boost-system_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force`
    && powershell Copy-Item -Path 'C:\vcpkg\packages\libpff_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force

#unzip
RUN powershell Invoke-WebRequest -Uri http://www.winimage.com/zLibDll/unzip101e.zip -OutFile C:\unzip101e.zip `
    && arc unarchive unzip101e.zip `
    && cd unzip101e `
    && powershell Write-Output 'cmake_minimum_required(VERSION 3.16)' `
      'project(Unzip)' `
      'set(CMAKE_CXX_STANDARD 17)' `
      'set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")' `
      'set(UNZIP_SRC ioapi.c unzip.c)'`
      'set(FLAGS -fPIC)'`
      'add_library(unzip STATIC ${UNZIP_SRC})'`
      'target_include_directories(unzip PUBLIC C:/include)' `
      'install(FILES unzip.h ioapi.h DESTINATION include)'`
      'install(TARGETS unzip DESTINATION lib)'`
      'target_compile_options(unzip PRIVATE -fPIC)'`
      > CMakeLists.txt `
    && type CMakeLists.txt `
    && mkdir build`
    && cd build `
    && cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake  -DCMAKE_INSTALL_PREFIX:PATH="C:\\"  `
    && cmake --build . --config Debug `
    && cmake --build . --config Debug --target install `
    && cd .. `
    && cd ..

#libcharsetdetect

RUN powershell Invoke-WebRequest -Uri http://silvercoders.com/download/3rdparty/libcharsetdetect-master.tar.bz2 -OutFile C:\libcharsetdetect-master.tar.bz2 `
    && arc unarchive libcharsetdetect-master.tar.bz2 `
    && cd libcharsetdetect-master `
    && powershell Remove-Item -Path 'CMakeLists.txt' `
    && powershell Write-Output 'cmake_minimum_required(VERSION 3.16)' `
      'project(charsetdetect)' `
      'set(CMAKE_CXX_STANDARD 17)' `
      'set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")' `
      'set(FLAGS -fPIC)'`
      'include_directories(. nspr-emu mozilla/extensions/universalchardet/src/base/)'`
      'file(GLOB charsetdetect_lib_src mozilla/extensions/universalchardet/src/base/*.cpp)'`
      'set(charsetdetect_lib_src ${charsetdetect_lib_src} charsetdetect.cpp)'`
      'add_library(charsetdetect STATIC ${charsetdetect_lib_src})'`
      'set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/build)' `
      'install(TARGETS charsetdetect DESTINATION lib)'`
      'install(FILES charsetdetect.h DESTINATION include)'`
      > CMakeLists.txt `
    && type CMakeLists.txt `
    && mkdir build`
    && cd build `
    && cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_INSTALL_PREFIX:PATH="C:\\" -DCMAKE_INSTALL_PREFIX:PATH="C:\\"  `
    && cmake --build . --config Debug`
    && cmake --build . --config Debug --target install `
    && cd .. `
    && cd ..

#leptonica
RUN vcpkg\vcpkg install leptonica:x64-windows

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\leptonica_x64-windows\include\leptonica' -Destination 'C:\include' -Recurse
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\leptonica_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force

#tesseract
RUN mkdir tessdata

RUN powershell Invoke-WebRequest -Uri https://github.com/tesseract-ocr/tessdata_fast/raw/main/eng.traineddata -OutFile eng.traineddata `
    && powershell Copy-Item -Path 'eng.traineddata' -Destination 'C:\tessdata' -Recurse

RUN powershell Invoke-WebRequest -Uri https://github.com/tesseract-ocr/tessdata_fast/raw/main/osd.traineddata -OutFile osd.traineddata `
    && powershell Copy-Item -Path 'osd.traineddata' -Destination 'C:\tessdata' -Recurse

RUN powershell Invoke-WebRequest -Uri https://github.com/tesseract-ocr/tessdata_fast/raw/main/pol.traineddata -OutFile pol.traineddata`
    && powershell Copy-Item -Path 'pol.traineddata' -Destination 'C:\tessdata' -Recurse

RUN git clone https://github.com/docwire/bfio.git`
    && cd bfio`
    && powershell .\synclibs.ps1 `
    && powershell .\autogen.ps1 `
    && cd msvscpp `
    && call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat" `
    && msbuild libbfio.sln /property:Configuration=VSDebug /property:Platform=x64`
    && powershell Copy-Item -Path 'C:\bfio\msvscpp\VSDebug\*.lib' -Destination 'C:\lib' -Recurse `
    && powershell Copy-Item -Path 'C:\bfio\msvscpp\VSDebug\*.dll' -Destination 'C:\lib' -Recurse `
    && powershell Copy-Item -Path 'C:\bfio\include\*.h' -Destination 'C:\include' `
    && powershell Copy-Item -Path 'C:\bfio\include\libbfio' -Destination 'C:\include' -Recurse

RUN cd vcpkg`
    && git checkout master`
    && git pull `
    && call ".\bootstrap-vcpkg.bat"

RUN cd vcpkg`
    && vcpkg install libiconv:x64-windows`
    && cd ..

RUN vcpkg\vcpkg install libxml2:x64-windows

RUN git clone https://github.com/docwire/htmlcxx.git`
    && cd htmlcxx`
    && mkdir build`
    && cd build`
    && cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_INSTALL_PREFIX:PATH="C:\\" `
    && cmake --build . --config Debug `
    && cmake --build . --config Debug --target install `
    && cd ../..

RUN git clone https://github.com/docwire/wv2.git `
    && cd wv2`
    && mkdir build`
    && cd build `
    && cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake  -DCMAKE_INSTALL_PREFIX:PATH="C:\\"  `
    && cmake --build . --config Debug `
    && cmake --build . --config Debug --target install `
    && cd ..\.. `
    && powershell Invoke-WebRequest -Uri http://silvercoders.com/download/3rdparty/wv2-0.2.3_patched_4-private_headers.tar.bz2 -OutFile C:\wv2-0.2.3_patched_4-private_headers.tar.bz2 `
    && arc unarchive wv2-0.2.3_patched_4-private_headers.tar.bz2 `
    && powershell Move-Item -Path wv2-0.2.3_patched_4-private_headers\*.h -Destination 'C:\include\wv2'

RUN powershell Copy-Item -Path 'C:\vcpkg\packages\libiconv_x64-windows\lib\*' -Destination 'C:\lib' -Recurse
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\libiconv_x64-windows\include\*.h' -Destination 'C:\include' -Recurse
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\libxml2_x64-windows\include\libxml' -Destination 'C:\include' -Recurse
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\libxml2_x64-windows\include\libxml2' -Destination 'C:\include' -Recurse
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\libxml2_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force

RUN vcpkg\vcpkg install tesseract:x64-windows
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\tesseract_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\tesseract_x64-windows\include\tesseract' -Destination 'C:\include' -Recurse

RUN vcpkg\vcpkg install libarchive:x64-windows
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\libarchive_x64-windows\debug\lib\*' -Destination 'C:\lib' -Recurse -Force
RUN powershell Copy-Item -Path 'C:\vcpkg\packages\libarchive_x64-windows\include\*.h' -Destination 'C:\include' -Recurse
