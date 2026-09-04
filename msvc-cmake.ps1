param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",

    [switch]$OpenVSCode,

    [switch]$ConfigureOnly,

    [switch]$Clean,

    [switch]$NoPause
)

$ErrorActionPreference = "Stop"

function Pause-Exit {
    if (-not $NoPause) {
        Write-Host ""
        Read-Host "Press Enter to exit"
    }
}

function Fail($Message) {
    Write-Host ""
    Write-Host "[ERROR] $Message" -ForegroundColor Red
    Pause-Exit
    exit 1
}

try {
    # 当前脚本放在项目根目录
    if ([string]::IsNullOrWhiteSpace($PSScriptRoot)) {
        $RepoRoot = (Get-Location).Path
    } else {
        $RepoRoot = $PSScriptRoot
    }

    if ([string]::IsNullOrWhiteSpace($env:VCPKG_ROOT)) {
        Fail "VCPKG_ROOT is not set. Point it to your vcpkg installation directory."
    }

    $VcpkgToolchain = Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"
    if (-not (Test-Path -LiteralPath $VcpkgToolchain -PathType Leaf)) {
        Fail "Cannot find the vcpkg CMake toolchain: $VcpkgToolchain"
    }

    Write-Host ""
    Write-Host "Repo root: $RepoRoot"
    Write-Host "Config   : $Config"
    Write-Host "vcpkg    : $env:VCPKG_ROOT"
    Write-Host ""

    # 1. 查找 vswhere
    $VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

    if (-not (Test-Path $VsWhere)) {
        $VsWhereCmd = Get-Command vswhere.exe -ErrorAction SilentlyContinue
        if ($VsWhereCmd) {
            $VsWhere = $VsWhereCmd.Source
        } else {
            Fail "Cannot find vswhere.exe. Please install Visual Studio or Visual Studio Build Tools."
        }
    }

    # 2. 查找带 MSVC C++ 工具集的 Visual Studio
    $VsPath = & $VsWhere `
        -latest `
        -products "*" `
        -requires "Microsoft.VisualStudio.Component.VC.Tools.x86.x64" `
        -property installationPath

    $VsPath = $VsPath | Select-Object -First 1

    if (-not $VsPath) {
        Fail "Cannot find Visual Studio with MSVC C++ tools. Please install Desktop development with C++."
    }

    Write-Host "Using Visual Studio:"
    Write-Host "  $VsPath"
    Write-Host ""

    # 3. 初始化 MSVC 环境
    $VsDevCmd = Join-Path $VsPath "Common7\Tools\VsDevCmd.bat"

    if (-not (Test-Path $VsDevCmd)) {
        Fail "Cannot find VsDevCmd.bat: $VsDevCmd"
    }

    Write-Host "Initializing MSVC environment..."
    Write-Host ""

    $CmdLine = "`"$VsDevCmd`" -arch=x64 -host_arch=x64 && set"

    cmd.exe /d /s /c $CmdLine | ForEach-Object {
        if ($_ -match "^([^=]+)=(.*)$") {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }

    # 4. 如果 PATH 中没有 cmake / ninja，尝试加入 VS 自带路径
    $VsCMakeBin = Join-Path $VsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    $VsNinjaBin = Join-Path $VsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"

    if (Test-Path (Join-Path $VsCMakeBin "cmake.exe")) {
        $env:PATH = "$VsCMakeBin;$env:PATH"
    }

    if (Test-Path (Join-Path $VsNinjaBin "ninja.exe")) {
        $env:PATH = "$VsNinjaBin;$env:PATH"
    }

    # 5. 检查 cl / cmake / ninja
    $Cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if (-not $Cl) {
        Fail "Cannot find cl.exe. MSVC environment was not initialized correctly."
    }

    $CMake = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if (-not $CMake) {
        Fail "Cannot find cmake.exe. Please install CMake or Visual Studio CMake tools."
    }

    $Ninja = Get-Command ninja.exe -ErrorAction SilentlyContinue
    if (-not $Ninja) {
        Fail "Cannot find ninja.exe. Please install Ninja or install 'C++ CMake tools for Windows' in Visual Studio Installer."
    }

    Write-Host "Using cl:"
    Write-Host "  $($Cl.Source)"
    Write-Host ""

    Write-Host "Using CMake:"
    Write-Host "  $($CMake.Source)"
    Write-Host ""

    Write-Host "Using Ninja:"
    Write-Host "  $($Ninja.Source)"
    Write-Host ""

    # 6. 选择 preset
    if ($Config -eq "Debug") {
        $Preset = "ninja-msvc-debug"
    } else {
        $Preset = "ninja-msvc-release"
    }

    Write-Host "Using preset:"
    Write-Host "  $Preset"
    Write-Host ""

    Set-Location $RepoRoot

    # 7. 可选清理旧的失败缓存
    $BuildDir = Join-Path $RepoRoot "out\build\$Preset"

    if ($Clean) {
        if (Test-Path $BuildDir) {
            Write-Host "Removing old build directory:"
            Write-Host "  $BuildDir"
            Remove-Item $BuildDir -Recurse -Force
            Write-Host ""
        }
    }

    # 8. 可选：用当前 MSVC 环境打开 VSCode
    if ($OpenVSCode) {
        $Code = Get-Command code.cmd -ErrorAction SilentlyContinue
        if (-not $Code) {
            $Code = Get-Command code.exe -ErrorAction SilentlyContinue
        }

        if (-not $Code) {
            Fail "Cannot find VSCode command: code. Please add VSCode to PATH."
        }

        Write-Host "Opening VSCode with MSVC environment..."
        Write-Host ""

        code .

        Write-Host "If VSCode was already open, close all VSCode windows and run this script again."
        Pause-Exit
        exit 0
    }

    # 9. CMake configure
    Write-Host "Running:"
    Write-Host "  cmake --preset $Preset"
    Write-Host ""

    cmake --preset $Preset

    if ($LASTEXITCODE -ne 0) {
        Fail "CMake configure failed."
    }

    if ($ConfigureOnly) {
        Write-Host ""
        Write-Host "Configure finished."
        Pause-Exit
        exit 0
    }

    # 10. CMake build
    Write-Host ""
    Write-Host "Running:"
    Write-Host "  cmake --build --preset $Preset"
    Write-Host ""

    cmake --build --preset $Preset

    if ($LASTEXITCODE -ne 0) {
        Fail "Build failed."
    }

    Write-Host ""
    Write-Host "Build finished successfully."
    Pause-Exit
}
catch {
    Write-Host ""
    Write-Host "[ERROR] $($_.Exception.Message)" -ForegroundColor Red
    Pause-Exit
    exit 1
}
