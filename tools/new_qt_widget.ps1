$ErrorActionPreference = "Stop"

$RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$WatchRootDir = $RepoRoot
$IgnoredPathPrefixes = @(
    (Join-Path $RepoRoot ".git"),
    (Join-Path $RepoRoot "out"),
    (Join-Path $RepoRoot "third_party")
)
$SupportedBaseClasses = @("QWidget", "QMainWindow", "QDialog")

function Get-RepoRelativePath($Path) {
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $root = $RepoRoot.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar

    if ($fullPath.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $fullPath.Substring($root.Length)
    }

    return $fullPath
}

function Write-Info($Message) {
    Write-Host "[qt-ui-watch] $Message"
}

function Write-WarningMessage($Message) {
    Write-Host "[qt-ui-watch] $Message" -ForegroundColor Yellow
}

function Test-IsIgnoredPath($Path) {
    $fullPath = [System.IO.Path]::GetFullPath($Path)

    foreach ($ignoredPath in $IgnoredPathPrefixes) {
        $ignoredPrefix = ([System.IO.Path]::GetFullPath($ignoredPath)).TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
        if ($fullPath.StartsWith($ignoredPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

function Get-UiXml($Path) {
    try {
        return [xml](Get-Content -LiteralPath $Path -Raw)
    }
    catch {
        Write-WarningMessage "Skip invalid .ui file: $(Get-RepoRelativePath $Path)"
        return $null
    }
}

function Save-UiXml($Xml, $Path) {
    $settings = New-Object System.Xml.XmlWriterSettings
    $settings.Indent = $true
    $settings.Encoding = New-Object System.Text.UTF8Encoding($false)

    $writer = [System.Xml.XmlWriter]::Create($Path, $settings)
    try {
        $Xml.Save($writer)
    }
    finally {
        $writer.Close()
    }
}

function Get-HeaderContent($ClassName, $BaseClass) {
    return @"
#pragma once

#include <$BaseClass>

QT_BEGIN_NAMESPACE
namespace Ui {
class $ClassName;
}
QT_END_NAMESPACE

namespace learnopengl::ui {

class $ClassName final : public $BaseClass {
    Q_OBJECT

public:
    explicit $ClassName(QWidget* parent = nullptr);
    ~$ClassName() override;

private:
    Ui::$ClassName* m_ui;
};

} // namespace learnopengl::ui
"@
}

function Get-GeneratedUiInclude($UiPath, $ClassName) {
    $uiDir = Split-Path -Parent ([System.IO.Path]::GetFullPath($UiPath))
    $watchRootWithSlash = ([System.IO.Path]::GetFullPath($WatchRootDir)).TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
    $relativeDir = ""

    if ($uiDir.StartsWith($watchRootWithSlash, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relativeDir = $uiDir.Substring($watchRootWithSlash.Length)
    }

    if ([string]::IsNullOrWhiteSpace($relativeDir) -or $relativeDir -eq ".") {
        return "ui_$ClassName.h"
    }

    $relativeDir = $relativeDir.Replace('\', '/')
    return "$relativeDir/ui_$ClassName.h"
}

function Get-SourceContent($ClassName, $BaseClass, $GeneratedUiInclude) {
    return @"
#include "$ClassName.h"

#include "$GeneratedUiInclude"

namespace learnopengl::ui {

$ClassName::$ClassName(QWidget* parent)
    : $BaseClass(parent)
    , m_ui(new Ui::$ClassName)
{
    m_ui->setupUi(this);
}

$ClassName::~$ClassName()
{
    delete m_ui;
}

} // namespace learnopengl::ui
"@
}

function New-QtClassFromUi($Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    if (Test-IsIgnoredPath $Path) {
        return
    }

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $dir = Split-Path -Parent $fullPath
    $className = [System.IO.Path]::GetFileNameWithoutExtension($fullPath)

    if ($className -notmatch '^[A-Z][A-Za-z0-9_]*$') {
        Write-WarningMessage "Skip $(Get-RepoRelativePath $fullPath): file name must be a PascalCase C++ class name."
        return
    }

    $xml = Get-UiXml $fullPath
    if ($null -eq $xml -or $null -eq $xml.ui -or $null -eq $xml.ui.widget) {
        Write-WarningMessage "Skip $(Get-RepoRelativePath $fullPath): missing <ui> or top-level <widget>."
        return
    }

    $baseClass = [string]$xml.ui.widget.class
    if ($SupportedBaseClasses -notcontains $baseClass) {
        Write-WarningMessage "Skip $(Get-RepoRelativePath $fullPath): unsupported base class '$baseClass'. Supported: $($SupportedBaseClasses -join ', ')."
        return
    }

    $uiChanged = $false

    if ([string]$xml.ui.class -ne $className) {
        $xml.ui.class = $className
        $uiChanged = $true
    }

    if ([string]$xml.ui.widget.name -ne $className) {
        $xml.ui.widget.name = $className
        $uiChanged = $true
    }

    if ($uiChanged) {
        Save-UiXml $xml $fullPath
        Write-Info "Updated UI class: $(Get-RepoRelativePath $fullPath)"
    }

    $headerPath = Join-Path $dir "$className.h"
    $sourcePath = Join-Path $dir "$className.cpp"

    if (-not (Test-Path -LiteralPath $headerPath)) {
        Set-Content -LiteralPath $headerPath -Value (Get-HeaderContent $className $baseClass) -Encoding UTF8
        Write-Info "Generated: $(Get-RepoRelativePath $headerPath)"
    }

    if (-not (Test-Path -LiteralPath $sourcePath)) {
        $generatedUiInclude = Get-GeneratedUiInclude $fullPath $className
        Set-Content -LiteralPath $sourcePath -Value (Get-SourceContent $className $baseClass $generatedUiInclude) -Encoding UTF8
        Write-Info "Generated: $(Get-RepoRelativePath $sourcePath)"
    }
}

function Sync-AllUiFiles {
    Get-ChildItem -LiteralPath $WatchRootDir -Filter "*.ui" -File -Recurse | Where-Object {
        -not (Test-IsIgnoredPath $_.FullName)
    } | ForEach-Object {
        New-QtClassFromUi $_.FullName
    }
}

Sync-AllUiFiles

Write-Info "Watching project root for .ui files. Press Ctrl+C to stop."

$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path = $WatchRootDir
$watcher.Filter = "*.ui"
$watcher.IncludeSubdirectories = $true
$watcher.EnableRaisingEvents = $true

$created = Register-ObjectEvent -InputObject $watcher -EventName Created -SourceIdentifier "QtUiCreated"
$changed = Register-ObjectEvent -InputObject $watcher -EventName Changed -SourceIdentifier "QtUiChanged"
$renamed = Register-ObjectEvent -InputObject $watcher -EventName Renamed -SourceIdentifier "QtUiRenamed"

try {
    while ($true) {
        $event = Wait-Event -Timeout 1
        if ($null -eq $event) {
            continue
        }

        Remove-Event -EventIdentifier $event.EventIdentifier

        Start-Sleep -Milliseconds 300
        $path = $event.SourceEventArgs.FullPath

        if ($path -and (Test-Path -LiteralPath $path)) {
            New-QtClassFromUi $path
        }
    }
}
finally {
    Unregister-Event -SourceIdentifier "QtUiCreated" -ErrorAction SilentlyContinue
    Unregister-Event -SourceIdentifier "QtUiChanged" -ErrorAction SilentlyContinue
    Unregister-Event -SourceIdentifier "QtUiRenamed" -ErrorAction SilentlyContinue
    $watcher.Dispose()
}
