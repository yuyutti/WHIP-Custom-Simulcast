[CmdletBinding()]
param(
    [ValidateSet('x64')]
    [string] $Target = 'x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'

if ( $DebugPreference -eq 'Continue' ) {
    $VerbosePreference = 'Continue'
    $InformationPreference = 'Continue'
}

if ( $env:CI -eq $null ) {
    throw "Build-Windows.ps1 requires CI environment"
}

if ( ! ( [System.Environment]::Is64BitOperatingSystem ) ) {
    throw "A 64-bit system is required to build the project."
}

if ( $PSVersionTable.PSVersion -lt '7.2.0' ) {
    Write-Warning 'The obs-studio PowerShell build script requires PowerShell Core 7. Install or upgrade your PowerShell version: https://aka.ms/pscore6'
    exit 2
}

function Build {
    trap {
        Pop-Location -Stack BuildTemp -ErrorAction 'SilentlyContinue'
        Write-Error $_
        Log-Group
        exit 2
    }

    $ScriptHome = $PSScriptRoot
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
    $BuildSpec = Get-Content -LiteralPath "${ProjectRoot}/buildspec.json" -Raw | ConvertFrom-Json
    $ProductName = $BuildSpec.name

    $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse

    foreach($Utility in $UtilityFunctions) {
        Write-Debug "Loading $($Utility.FullName)"
        . $Utility.FullName
    }

    Push-Location -Stack BuildTemp
    Ensure-Location $ProjectRoot

    $CmakeArgs = @('--preset', "windows-ci-${Target}")
    $CmakeBuildArgs = @('--build')
    $CmakeTestArgs = @('--test-dir', "build_${Target}")
    $CmakeInstallArgs = @()

    if ( $DebugPreference -eq 'Continue' ) {
        $CmakeArgs += ('--debug-output')
        $CmakeBuildArgs += ('--verbose')
        $CmakeTestArgs += ('--verbose')
        $CmakeInstallArgs += ('--verbose')
    }

    $CmakeBuildArgs += @(
        '--preset', "windows-${Target}"
        '--config', $Configuration
        '--parallel'
        '--', '/consoleLoggerParameters:Summary', '/noLogo'
    )

    $CmakeInstallArgs += @(
        '--install', "build_${Target}"
        '--prefix', "${ProjectRoot}/release/${Configuration}"
        '--config', $Configuration
    )

    $CmakeTestArgs += @(
        '--build-config', $Configuration
        '--output-on-failure'
    )

    Log-Group "Configuring ${ProductName}..."
    Invoke-External cmake @CmakeArgs

    Log-Group "Building ${ProductName}..."
    Invoke-External cmake @CmakeBuildArgs

    Log-Group "Testing ${ProductName}..."
    Invoke-External ctest @CmakeTestArgs

    Log-Group "Installing ${ProductName}..."
    Invoke-External cmake @CmakeInstallArgs

    $PackageRoot = "${ProjectRoot}/release/${Configuration}/${ProductName}"
    $ExpectedPackageFiles = @(
        "${PackageRoot}/bin/64bit/${ProductName}.dll"
        "${PackageRoot}/data/locale/en-US.ini"
        "${PackageRoot}/data/locale/ja-JP.ini"
        "${PackageRoot}/LICENSE"
        "${PackageRoot}/README.md"
        "${PackageRoot}/README.en.md"
        "${PackageRoot}/CHANGELOG.md"
    )

    foreach ( $ExpectedFile in $ExpectedPackageFiles ) {
        if ( ! ( Test-Path -LiteralPath $ExpectedFile -PathType Leaf ) ) {
            throw "Required package file is missing: ${ExpectedFile}"
        }
    }

    Pop-Location -Stack BuildTemp
    Log-Group
}

Build
