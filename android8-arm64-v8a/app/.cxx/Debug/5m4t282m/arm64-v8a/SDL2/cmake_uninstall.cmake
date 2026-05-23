if (NOT EXISTS "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/install_manifest.txt")
    message(FATAL_ERROR "Cannot find install manifest: \"E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/install_manifest.txt\"")
endif(NOT EXISTS "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/install_manifest.txt")

file(READ "E:/Development/Android/UE1/com.ast.unreal/app/.cxx/Debug/5m4t282m/arm64-v8a/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
foreach (file ${files})
    message(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
    execute_process(
        COMMAND C:/Users/andiw/AppData/Local/Android/Sdk/cmake/3.22.1/bin/cmake.exe -E remove "$ENV{DESTDIR}${file}"
        OUTPUT_VARIABLE rm_out
        RESULT_VARIABLE rm_retval
    )
    if(NOT ${rm_retval} EQUAL 0)
        message(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
    endif (NOT ${rm_retval} EQUAL 0)
endforeach(file)

