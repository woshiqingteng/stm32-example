# Configure/build/flash one firmware with a layered build tree:
#   build\<BUILD_TYPE>\<BOARD>\<OS>\<APP_NAME>\
param(
    [Parameter(Mandatory = $true)][ValidateSet('debug', 'release')][string]$Config,
    [Parameter(Mandatory = $true)][string]$App,
    [ValidateSet('build', 'flash')][string]$Action = 'build'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$board = 'openedv_stm32f4'
$os = 'baremetal'
$buildDir = Join-Path $root "build/$Config/$board/$os/$App"
$target = "app_${os}_${App}"

Push-Location $root
try {
    cmake --preset $Config -B $buildDir -DAPP_TARGET=$target
    cmake --build $buildDir
    if ($Action -eq 'flash') {
        cmake --build $buildDir --target flash
    }
}
finally {
    Pop-Location
}
