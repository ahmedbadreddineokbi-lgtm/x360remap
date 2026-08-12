<#
.SYNOPSIS
    Build hiddriver.xex and push it to the console for live testing.

.NOTES
    Written from what's actually in this repo (hiddriver.sln uses the
    "Release Retail|Xbox 360" configuration, output is hiddriver.xex) and
    from the xecli (rgh) wiki command reference. NOT tested end-to-end here
    (no Windows/XDK/console access from this session) - verify the two
    marked sections against your setup before trusting it blindly.

.PARAMETER ConsoleIp
    IP address of your Xbox 360 (XBDM must be enabled - DashLaunch/Freeboot).

.PARAMETER LiveLoad
    If set, hot-loads the driver via XBDM without needing a console reboot
    (rgh modules load --system). Otherwise just copies the file to the HDD;
    you load it yourself via launch.ini or `rgh plugin enable`.
#>

param(
    [Parameter(Mandatory = $true)]
    [string]$ConsoleIp,

    [switch]$LiveLoad
)

$ErrorActionPreference = "Stop"
$RepoRoot = $PSScriptRoot

# ---------------------------------------------------------------------------
# 1. Locate MSBuild
#    VERIFY: adjust if your VS2019 install path differs, or if the XDK's
#    Xbox 360 platform toolset needs a specific "Xbox 360 Command Prompt"
#    environment instead of a plain MSBuild invocation.
# ---------------------------------------------------------------------------
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = $null

if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
    if ($vsPath) {
        $candidate = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
        if (Test-Path $candidate) { $msbuild = $candidate }
    }
}

if (-not $msbuild) {
    Write-Warning "MSBuild introuvable automatiquement via vswhere. Renseigne le chemin en dur ci-dessous si besoin."
    # $msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    throw "MSBuild.exe non trouve - complete le chemin dans le script."
}

Write-Host "MSBuild: $msbuild"

# ---------------------------------------------------------------------------
# 2. Build
# ---------------------------------------------------------------------------
$slnPath = Join-Path $RepoRoot "hiddriver.sln"
Write-Host "Building $slnPath (Release Retail|Xbox 360)..."

& $msbuild $slnPath /p:Configuration="Release Retail" /p:Platform="Xbox 360" /m
if ($LASTEXITCODE -ne 0) {
    throw "Build failed (exit code $LASTEXITCODE)."
}

# ---------------------------------------------------------------------------
# 3. Locate the built .xex
#    We search rather than hardcode the output path since it depends on
#    OutDir defaults that weren't visible from the .vcxproj alone.
# ---------------------------------------------------------------------------
$xex = Get-ChildItem -Path $RepoRoot -Recurse -Filter "hiddriver.xex" -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if (-not $xex) {
    throw "hiddriver.xex introuvable apres build. Verifie la sortie de MSBuild ci-dessus."
}

Write-Host "Built: $($xex.FullName)"

# ---------------------------------------------------------------------------
# 4. Deploy via xecli (rgh)
#    VERIFY: `rgh fs put` / `rgh modules load --system` against `rgh --help`
#    and the wiki - confirmed from the Commands Reference, but the --system
#    flag's applicability to hiddriver.xex specifically isn't confirmed here.
# ---------------------------------------------------------------------------
Write-Host "Pushing to console at $ConsoleIp..."
& rgh fs put --ip $ConsoleIp --path "Hdd:\hiddriver.xex" --in $xex.FullName
if ($LASTEXITCODE -ne 0) {
    throw "rgh fs put failed (exit code $LASTEXITCODE)."
}

if ($LiveLoad) {
    Write-Host "Hot-loading via XBDM..."
    & rgh modules load --ip $ConsoleIp --path "Hdd:\hiddriver.xex" --system
} else {
    Write-Host "Copie faite. Charge-le via ton launch.ini (rgh plugin enable) ou `rgh modules load` manuellement."
}

Write-Host "Done."
