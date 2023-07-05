Param (
    [string]$BuildType
)

mkdir build
if ($BuildType -eq "Debug")
{
    $COPY_COMMAND="`$LIB_PATHS=('/lib/libbfio.dll',
                            '/bin/htmlcxx.dll',
                            '/bin/wv2.dll',
                            '/vcpkg/packages/boost-filesystem_x64-windows/debug/bin/boost_filesystem-vc143-mt-gd-x64-1_79.dll',
                            '/vcpkg/packages/brotli_x64-windows/debug/bin/brotlicommon.dll',
                            '/vcpkg/packages/brotli_x64-windows/debug/bin/brotlidec.dll',
                            '/vcpkg/packages/bzip2_x64-windows/debug/bin/bz2d.dll',
                            '/vcpkg/packages/freetype_x64-windows/debug/bin/freetyped.dll',
                            '/vcpkg/packages/giflib_x64-windows/debug/bin/gif.dll',
                            '/vcpkg/packages/libiconv_x64-windows/debug/bin/iconv-2.dll',
                            '/vcpkg/packages/libjpeg-turbo_x64-windows/debug/bin/jpeg62.dll',
                            '/vcpkg/packages/leptonica_x64-windows/debug/bin/leptonica-1.82.0d.dll',
                            '/vcpkg/packages/openssl_x64-windows/debug/bin/libcrypto-3-x64.dll',
                            '/vcpkg/packages/liblzma_x64-windows/debug/bin/liblzma.dll',
                            '/vcpkg/packages/libpff_x64-windows/debug/bin/libpff.dll',
                            '/vcpkg/packages/libpng_x64-windows/debug/bin/libpng16d.dll',
                            '/vcpkg/packages/libxml2_x64-windows/debug/bin/libxml2.dll',
                            '/vcpkg/packages/openjpeg_x64-windows/debug/bin/openjp2.dll',
                            '/vcpkg/packages/podofo_x64-windows/debug/bin/podofo.dll',
                            '/vcpkg/packages/pthreads_x64-windows/debug/bin/pthreadVC3d.dll',
                            '/vcpkg/packages/tiff_x64-windows/debug/bin/tiffd.dll',
                            '/vcpkg/packages/libwebp_x64-windows/debug/bin/webp.dll',
                            '/vcpkg/packages/libwebp_x64-windows/debug/bin/webpmux.dll',
                            '/vcpkg/packages/zlib_x64-windows/debug/bin/zlibd1.dll');
                            foreach (`$PATH in `$LIB_PATHS){echo `$PATH; Copy-Item -Path `$PATH -Destination build/};
                            Copy-Item -Path c:/tessdata -Destination build -Recurse"

    $docker_image_id=Get-FileHash -Algorithm SHA1 build_debug_env_msvc.Dockerfile | Select-Object -ExpandProperty Hash
    $image_exists=0
    docker manifest inspect ghcr.io/docwire/doctotext_build_debug_env_msvc:$docker_image_id > $null; if ($?) {$image_exists=1}
    if ($image_exists -eq 1)
    {
        docker pull ghcr.io/docwire/doctotext_build_debug_env_msvc:$docker_image_id
    }
    else
    {
        docker build -t ghcr.io/docwire/doctotext_build_debug_env_msvc:$docker_image_id -f build_debug_env_msvc.Dockerfile .
        if ($ghcr_login)
        {
            echo "$ghcr_password" | docker login ghcr.io -u "$ghcr_login" --password-stdin
            docker push ghcr.io/docwire/doctotext_build_debug_env_msvc:$docker_image_id
        }
    }

    $BUILD_COMMAND = "git config --global --add safe.directory C:/t1; cd build; cmake .. -G 'Visual Studio 17 2022' -A x64 -DCMAKE_TOOLCHAIN_FILE=C://vcpkg/scripts/buildsystems/vcpkg.cmake; cmake --build . -j6 --config Debug; cmake --build . --config Debug --target doxygen install; cd .."
    docker run --rm -i --init -v ${pwd}:C:\t1 -w C:\t1 ghcr.io/docwire/doctotext_build_debug_env_msvc:$docker_image_id powershell "$BUILD_COMMAND; $COPY_COMMAND"

    $TEST_COMMAND = "cd build/; ctest -V"
    docker run --rm -i --init -v ${pwd}:C:\t1 -w C:\t1 ghcr.io/docwire/doctotext_build_debug_env_msvc:$docker_image_id powershell -c $TEST_COMMAND
}
else
{
    $COPY_COMMAND="`$LIB_PATHS=('/lib/libbfio.dll',
                            '/bin/htmlcxx.dll',
                            '/bin/wv2.dll',
                            '/vcpkg/packages/boost-filesystem_x64-windows/bin/boost_filesystem-vc143-mt-x64-1_79.dll',
                            '/vcpkg/packages/brotli_x64-windows/bin/brotlicommon.dll',
                            '/vcpkg/packages/brotli_x64-windows/bin/brotlidec.dll',
                            '/vcpkg/packages/bzip2_x64-windows/bin/bz2.dll',
                            '/vcpkg/packages/freetype_x64-windows/bin/freetype.dll',
                            '/vcpkg/packages/giflib_x64-windows/bin/gif.dll',
                            '/vcpkg/packages/libiconv_x64-windows/bin/iconv-2.dll',
                            '/vcpkg/packages/libjpeg-turbo_x64-windows/bin/jpeg62.dll',
                            '/vcpkg/packages/leptonica_x64-windows/bin/leptonica-1.82.0.dll',
                            '/vcpkg/packages/openssl_x64-windows/bin/libcrypto-3-x64.dll',
                            '/vcpkg/packages/liblzma_x64-windows/bin/liblzma.dll',
                            '/vcpkg/packages/libpff_x64-windows/bin/libpff.dll',
                            '/vcpkg/packages/libpng_x64-windows/bin/libpng16.dll',
                            '/vcpkg/packages/libxml2_x64-windows/bin/libxml2.dll',
                            '/vcpkg/packages/openjpeg_x64-windows/bin/openjp2.dll',
                            '/vcpkg/packages/podofo_x64-windows/bin/podofo.dll',
                            '/vcpkg/packages/pthreads_x64-windows/bin/pthreadVC3.dll',
                            '/vcpkg/packages/tiff_x64-windows/bin/tiff.dll',
                            '/vcpkg/packages/libwebp_x64-windows/bin/webp.dll',
                            '/vcpkg/packages/libwebp_x64-windows/bin/webpmux.dll',
                            '/vcpkg/packages/zlib_x64-windows/bin/zlib1.dll',
                            '/vcpkg/packages/libarchive_x64-windows/bin/archive.dll');
                            foreach (`$PATH in `$LIB_PATHS){echo `$PATH; Copy-Item -Path `$PATH -Destination build/};
                            Copy-Item -Path c:/tessdata -Destination build -Recurse"

    $docker_image_id=Get-FileHash -Algorithm SHA1 build_env_msvc.Dockerfile | Select-Object -ExpandProperty Hash
    $image_exists=0
    docker manifest inspect ghcr.io/docwire/doctotext_build_env_msvc:$docker_image_id > $null; if ($?) {$image_exists=1}
    if ($image_exists -eq 1)
    {
        docker pull ghcr.io/docwire/doctotext_build_env_msvc:$docker_image_id
    }
    else
    {
        docker build -t ghcr.io/docwire/doctotext_build_env_msvc:$docker_image_id -f build_env_msvc.Dockerfile .
        if ($ghcr_login)
        {
            echo "$ghcr_password" | docker login ghcr.io -u "$ghcr_login" --password-stdin
            docker push ghcr.io/docwire/doctotext_build_env_msvc:$docker_image_id
        }
    }

    $BUILD_COMMAND = "git config --global --add safe.directory C:/t1; cd build; cmake .. -G 'Visual Studio 17 2022' -A x64 -DCMAKE_TOOLCHAIN_FILE=C://vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_MSVC_RUNTIME_LIBRARY='MultiThreaded$<$<CONFIG:Debug>:Debug>'; cmake --build . -j6 --config Release; cmake --build . --config Release --target doxygen install; cd .."
    docker run --rm -i --init -v ${pwd}:C:\t1 -w C:\t1 ghcr.io/docwire/doctotext_build_env_msvc:$docker_image_id powershell "$BUILD_COMMAND; $COPY_COMMAND"

    $TEST_COMMAND = "cd build/; ctest -V"
    docker run --rm -i --init -v ${pwd}:C:\t1 -w C:\t1 ghcr.io/docwire/doctotext_build_env_msvc:$docker_image_id powershell -c $TEST_COMMAND
}


Get-ChildItem -Path build\ -Recurse -Filter *.dll | Select-Object -Property Name,@{name="Hash";expression={(Get-FileHash $_.FullName).hash}} > build\SHA1checksums.sha1

$version = Get-Content build/VERSION

mkdir doctotext-$version-msvc

Copy-Item -Path "build\*.dll" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\*.lib" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\*.pdb" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\*.exe" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\*.h" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\*.hpp" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\SHA1checksums.sha1" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\VERSION" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "ChangeLog" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\plugins" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\tessdata" -Destination "doctotext-$version-msvc" -Recurse
Copy-Item -Path "build\doc" -Destination "doctotext-$version-msvc" -Recurse

Compress-Archive -LiteralPath doctotext-$version-msvc -DestinationPath doctotext-$version-msvc.zip
Get-FileHash -Algorithm SHA1 doctotext-$version-msvc.zip > doctotext-$version-msvc.zip.sha1

Remove-Item -Path doctotext-$version-msvc -Recurse
