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
    throw "Package-Windows.ps1 requires CI environment"
}

if ( ! ( [System.Environment]::Is64BitOperatingSystem ) ) {
    throw "Packaging script requires a 64-bit system to build and run."
}

if ( $PSVersionTable.PSVersion -lt '7.2.0' ) {
    Write-Warning 'The packaging script requires PowerShell Core 7. Install or upgrade your PowerShell version: https://aka.ms/pscore6'
    exit 2
}

function Package {
    trap {
        Write-Error $_
        exit 2
    }

    $ScriptHome = $PSScriptRoot
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
    $BuildSpecFile = "${ProjectRoot}/buildspec.json"

    $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse

    foreach( $Utility in $UtilityFunctions ) {
        Write-Debug "Loading $($Utility.FullName)"
        . $Utility.FullName
    }

    $BuildSpec = Get-Content -Path ${BuildSpecFile} -Raw | ConvertFrom-Json
    $ProductName = $BuildSpec.name
    $ProductVersion = $BuildSpec.version

    $OutputName = "${ProductName}-${ProductVersion}-windows-${Target}"
    $PackageRoot = "${ProjectRoot}/release/${Configuration}/${ProductName}"
    $InstallerScript = "${ProjectRoot}/installer/WHIP-Custom-Simulcast.iss"

    $ResolvedPackageRoot = ( Resolve-Path -LiteralPath $PackageRoot ).Path
    Get-ChildItem -LiteralPath $ResolvedPackageRoot -Recurse -File -Filter '*.pdb' | ForEach-Object {
        if ( ! $_.FullName.StartsWith("${ResolvedPackageRoot}\", [System.StringComparison]::OrdinalIgnoreCase) ) {
            throw "Refusing to remove a symbol file outside the package root: $($_.FullName)"
        }
        Remove-Item -LiteralPath $_.FullName -Force
    }

    $RemoveArgs = @{
        ErrorAction = 'SilentlyContinue'
        Path = @(
            "${ProjectRoot}/release/${ProductName}-*-windows-*.zip"
        )
    }

    Remove-Item @RemoveArgs

    Log-Group "Archiving ${ProductName}..."
    $CompressArgs = @{
        Path = (Get-ChildItem -Path "${ProjectRoot}/release/${Configuration}" -Exclude "${OutputName}*.*")
        CompressionLevel = 'Optimal'
        DestinationPath = "${ProjectRoot}/release/${OutputName}.zip"
        Verbose = ($Env:CI -ne $null)
    }
    Compress-Archive -Force @CompressArgs

    $ArchivePath = "${ProjectRoot}/release/${OutputName}.zip"
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $Archive = [System.IO.Compression.ZipFile]::OpenRead($ArchivePath)
    try {
        $ArchiveEntries = @( $Archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') } )
        $RequiredEntries = @(
            "${ProductName}/bin/64bit/${ProductName}.dll"
            "${ProductName}/data/locale/en-US.ini"
            "${ProductName}/data/locale/ja-JP.ini"
        )
        foreach ( $RequiredEntry in $RequiredEntries ) {
            if ( $RequiredEntry -notin $ArchiveEntries ) {
                throw "Package archive is missing ${RequiredEntry}"
            }
        }
        if ( $ArchiveEntries | Where-Object { $_ -like '*.pdb' } ) {
            throw 'Package archive must not contain PDB files.'
        }
    } finally {
        $Archive.Dispose()
    }
    Log-Group

    $InnoCompilerCandidates = @(
        "${env:LOCALAPPDATA}/Programs/Inno Setup 6/ISCC.exe"
        "${env:ProgramFiles(x86)}/Inno Setup 6/ISCC.exe"
        "${env:ProgramFiles}/Inno Setup 6/ISCC.exe"
    )
    $InnoCompiler = $InnoCompilerCandidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1

    if ( $InnoCompiler -eq $null ) {
        throw 'Inno Setup 6 compiler was not found.'
    }

    Log-Group "Creating ${ProductName} installer..."
    $InnoArgs = @(
        "/DPluginSourceDir=${PackageRoot}"
        "/DPluginVersion=${ProductVersion}"
        "/DInstallerOutputDir=${ProjectRoot}/release"
        $InstallerScript
    )
    Invoke-External $InnoCompiler @InnoArgs

    $InstallerPath = "${ProjectRoot}/release/${OutputName}-setup.exe"
    if ( ! ( Test-Path -LiteralPath $InstallerPath -PathType Leaf ) ) {
        throw "Installer was not created: ${InstallerPath}"
    }
    Log-Group
}

Package
