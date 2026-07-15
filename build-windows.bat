@echo off
chcp 65001 > nul
setlocal

echo ==========================================
echo        CMAKE WINDOWS BUILD SCRIPT
echo ==========================================
echo.

set BUILD_DIR=build-windows

echo [1/5] Проверка наличия CMake...
cmake --version > nul 2>&1

if %errorlevel% neq 0 (
    echo ERROR: CMake не найден!
    echo Установите CMake и добавьте его в PATH.
    pause
    exit /b 1
)

echo OK: CMake найден
echo.

echo [2/5] Проверка компилятора MinGW...
g++ --version > nul 2>&1

if %errorlevel% neq 0 (
    echo ERROR: MinGW g++ не найден!
    echo Установите MinGW-w64 и добавьте его в PATH.
    pause
    exit /b 1
)

echo OK: MinGW найден
echo.

echo [3/5] Подготовка папки сборки...
if not exist %BUILD_DIR% (
    echo Создание папки %BUILD_DIR%...
    mkdir %BUILD_DIR%
) else (
    echo Папка %BUILD_DIR% уже существует.
)

echo Переход в папку сборки...
cd %BUILD_DIR%

echo.

echo [4/5] Настройка проекта через CMake...
echo Команда:
echo cmake .. -G "MinGW Makefiles"
echo.

cmake .. -G "MinGW Makefiles"

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Ошибка конфигурации CMake!
    pause
    exit /b 1
)

echo OK: Конфигурация завершена
echo.

echo [5/5] Компиляция проекта...
echo Команда:
echo cmake --build . --config Release
echo.

cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Ошибка сборки!
    pause
    exit /b 1
)

echo.
echo ==========================================
echo        СБОРКА УСПЕШНО ЗАВЕРШЕНА
echo ==========================================
echo.
echo Файлы находятся в:
echo %CD%

pause
