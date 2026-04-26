@echo off
setlocal EnableExtensions EnableDelayedExpansion
set DIR=%~dp0

if exist "%DIR%gradle\wrapper\gradle-wrapper.jar" (
  java -classpath "%DIR%gradle\wrapper\gradle-wrapper.jar" org.gradle.wrapper.GradleWrapperMain %*
  exit /b %ERRORLEVEL%
)

where gradle >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  gradle %*
  exit /b %ERRORLEVEL%
)

rem Android Studio often has already downloaded the Gradle distribution into the user cache.
set "GRADLE_DISTS=%USERPROFILE%\.gradle\wrapper\dists"
if exist "%GRADLE_DISTS%" (
  for /r "%GRADLE_DISTS%" %%G in (gradle.bat) do (
    echo %%G | findstr /i "gradle-8.13" >nul
    if !ERRORLEVEL! EQU 0 (
      call "%%G" %*
      exit /b !ERRORLEVEL!
    )
  )
)

echo Could not find Gradle 8.13.
echo Open this project in Android Studio Otter 2 and run the Gradle task from the IDE,
echo or install Gradle once with: winget install Gradle.Gradle
echo After that this command will work: gradle wrapper --gradle-version 8.13 --distribution-type bin
exit /b 1
