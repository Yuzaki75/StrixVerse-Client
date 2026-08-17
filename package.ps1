# Assembles a distributable StrixVerse client in dist/StrixVerse.
#
# The result is a self-contained folder that can be zipped and handed to
# someone else. It carries the Visual C++ runtime, so the target machine does
# NOT need the redistributable installed -- that is the usual reason a build
# that works here fails on a friend's laptop with a missing-DLL dialog.
param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$Root    = Split-Path -Parent $MyInvocation.MyCommand.Path
$BinDir  = Join-Path $Root "bin\$Configuration"
$DistDir = Join-Path $Root "dist\StrixVerse"

if (-not (Test-Path (Join-Path $BinDir "StrixVerseClient.exe"))) {
    Write-Error "No $Configuration build found at $BinDir. Build it first."
}

Write-Output "Packaging $Configuration from $BinDir"

if (Test-Path $DistDir) { Remove-Item -Recurse -Force $DistDir }
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

# --- executable and its dependencies ------------------------------------
Copy-Item (Join-Path $BinDir "StrixVerseClient.exe") $DistDir
Get-ChildItem -Path $BinDir -Filter *.dll | ForEach-Object {
    Copy-Item $_.FullName $DistDir
}

# --- data ----------------------------------------------------------------
# logs/ and saves/ are deliberately not copied: a fresh install should start
# with no log history and nobody else's remembered sign-in or world save.
foreach ($dir in @("assets", "shaders")) {
    $src = Join-Path $BinDir $dir
    if (Test-Path $src) {
        Copy-Item $src $DistDir -Recurse
    }
}

# Config comes from source, not from bin: the deployed copy can be stale, and
# this is the file a player edits to point at the host's machine.
New-Item -ItemType Directory -Force -Path (Join-Path $DistDir "configs") | Out-Null
Copy-Item (Join-Path $Root "configs\client.json") (Join-Path $DistDir "configs")

# --- Visual C++ runtime --------------------------------------------------
# Without these the client fails to start on any machine that has never had
# Visual Studio or the redistributable installed.
$redistRoot = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC"
$runtimeCopied = 0

if (Test-Path $redistRoot) {
    # Search for the x64 CRT folder rather than guessing the version. The
    # redist root also holds a "v145" alias directory with no x64 subfolder,
    # which sorts ahead of the real versioned ones and is not what we want.
    $crtDir = Get-ChildItem $redistRoot -Directory -Recurse -Depth 2 -ErrorAction SilentlyContinue |
              Where-Object { $_.Name -like "Microsoft.VC*.CRT" -and $_.FullName -like "*\x64\*" } |
              Sort-Object FullName -Descending |
              Select-Object -First 1 -ExpandProperty FullName

    if ($crtDir) {
        Write-Output "Runtime source: $crtDir"
        Get-ChildItem $crtDir -Filter *.dll | ForEach-Object {
            Copy-Item $_.FullName $DistDir
            $runtimeCopied++
        }
    }
}

if ($runtimeCopied -eq 0) {
    Write-Warning "No Visual C++ runtime DLLs bundled. Players will need the x64 VC++ redistributable installed."
} else {
    Write-Output "Bundled $runtimeCopied Visual C++ runtime DLL(s)"
}

# --- a note for whoever receives the folder -------------------------------
$lanIp = (Get-NetIPAddress -AddressFamily IPv4 |
          Where-Object { $_.IPAddress -notlike "127.*" -and $_.IPAddress -notlike "169.254.*" } |
          Select-Object -First 1 -ExpandProperty IPAddress)
if (-not $lanIp) { $lanIp = "<the host's IP address>" }

$readme = @"
StrixVerse - client
===================

To play:

  1. Run StrixVerseClient.exe
  2. Create an account, or sign in if you already have one

Connecting to a friend's server
-------------------------------
Open configs\client.json and set the host to the machine running the server:

    "server":
    {
        "host": "$lanIp",
        "port": 17091
    }

"127.0.0.1" means "this same computer", so it only works if you are running
the server yourself. Everyone else needs the host's address.

The host must allow inbound TCP on port 17091 through their firewall. On the
same home network the address above is enough; over the internet the host also
needs to forward that port on their router.

Display
-------
Window size, fullscreen and vsync are in the same file. The interface is laid
out on a 1920x1080 canvas and scales to any window size, so a smaller or
larger screen is fine.

Nothing here needs installing, and no Visual C++ redistributable is required.
"@

Set-Content -Path (Join-Path $DistDir "README.txt") -Value $readme -Encoding utf8

# --- report ---------------------------------------------------------------
$size = (Get-ChildItem $DistDir -Recurse | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Output ""
Write-Output "Packaged to: $DistDir"
Write-Output ("Total size : {0:N1} MB" -f $size)
Write-Output "Host LAN IP: $lanIp"
Write-Output ""
Write-Output "Contents:"
Get-ChildItem $DistDir | ForEach-Object {
    if ($_.PSIsContainer) { Write-Output ("  {0}\" -f $_.Name) }
    else { Write-Output ("  {0}" -f $_.Name) }
}
