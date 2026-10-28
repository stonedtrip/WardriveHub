param(
  [string]$ArduinoCli='',
  [string]$ConfigFile='',
  [string]$Esp32Libraries=(Join-Path $env:LOCALAPPDATA 'Arduino15/packages/esp32/hardware/esp32/3.3.12/libraries'),
  [string]$CoreFqbn='esp32:esp32:esp32c5:FlashSize=16M,PSRAM=enabled',
  [string]$NodeFqbn='esp32:esp32:XIAO_ESP32C5',
  [string]$MiniFqbn='esp32:esp32:XIAO_ESP32C5',
  [string]$DongleFqbn='esp32:esp32:esp32c5:FlashSize=4M,PSRAM=disabled'
)
$ErrorActionPreference='Stop'
if(!$ArduinoCli){
  $found=Get-Command arduino-cli -ErrorAction SilentlyContinue
  if($found){$ArduinoCli=$found.Source}else{$ArduinoCli=Join-Path $env:ProgramFiles 'Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe'}
}
if(!(Test-Path -LiteralPath $ArduinoCli)){throw 'Pass -ArduinoCli with the installed arduino-cli.exe path.'}
$repoRoot=Split-Path $PSScriptRoot -Parent
$paths=Get-Content (Join-Path $repoRoot 'firmware/targets.json') -Raw | ConvertFrom-Json
$configArgs=@()
if($ConfigFile){$configArgs+=@('--config-file',$ConfigFile)}
$dataPath=(& $ArduinoCli @configArgs config get directories.data | Out-String).Trim()
if($LASTEXITCODE -ne 0){throw 'Unable to read Arduino data directory.'}
$userPath=(& $ArduinoCli @configArgs config get directories.user | Out-String).Trim()
if($LASTEXITCODE -ne 0){throw 'Unable to read Arduino user directory.'}
$mapRoots=@(
  @{Path=[Environment]::GetFolderPath('UserProfile');Alias='/user'},
  @{Path=$dataPath;Alias='/arduino-data'},
  @{Path=$userPath;Alias='/arduino-user'},
  @{Path=$repoRoot;Alias='/wardrivehub'}
)
$prefixFlags=@()
foreach($rootMap in $mapRoots){
  foreach($mapPath in @($rootMap.Path,$rootMap.Path.Replace('\','/')) | Select-Object -Unique){
    $prefixFlags+='"-ffile-prefix-map='+$mapPath+'='+$rootMap.Alias+'"'
  }
}
$privacyFlags=$prefixFlags -join ' '
$buildRoot=Join-Path $repoRoot '.validation-build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$targets=@(
  @{Name='WARDRIVE_HUB_C5_CYD_Core';Fqbn=$CoreFqbn;Extra=@('--build-property','upload.maximum_size=3145728','--library',(Join-Path $Esp32Libraries 'SD'))},
  @{Name='WARDRIVE_HUB_C5_Node_Wi_Fi_and_Bluetooth';Fqbn=$NodeFqbn;Extra=@()},
  @{Name='WARDRIVE_HUB_C5_Node_Bluetooth_Only';Fqbn=$NodeFqbn;Extra=@()},
  @{Name='WARDRIVE_HUB_C5_Mini';Fqbn=$MiniFqbn;Extra=@('--library',(Join-Path $Esp32Libraries 'SD'))},
  @{Name='WARDRIVE_HUB_C5_T_Dongle_Node';Fqbn=$DongleFqbn;Extra=@('--build-property','upload.maximum_size=3145728','--library',(Join-Path $repoRoot 'third_party/LilyGo-display-library'))}
)
foreach($target in $targets){
  $dir=Join-Path $buildRoot $target.Name
  $argsList=@('compile','--fqbn',$target.Fqbn,'--jobs','2','--build-path',$dir,'--build-property',('compiler.c.extra_flags='+$privacyFlags),'--build-property',('compiler.cpp.extra_flags='+$privacyFlags))+$target.Extra
  if($ConfigFile){$argsList+=@('--config-file',$ConfigFile)}
  $argsList+=(Join-Path $repoRoot $paths.($target.Name))
  & $ArduinoCli @argsList
  if($LASTEXITCODE -ne 0){throw "Build failed: $($target.Name)"}
  if($target.Name -eq 'WARDRIVE_HUB_C5_T_Dongle_Node'){
    $resolvedRoot=(Resolve-Path -LiteralPath $buildRoot).Path
    $resolvedDir=(Resolve-Path -LiteralPath $dir).Path
    if(!$resolvedDir.StartsWith($resolvedRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Unexpected build output path.'}
    $unsafeMerged=Join-Path $resolvedDir "$($target.Name).ino.merged.bin"
    if(Test-Path -LiteralPath $unsafeMerged){Remove-Item -LiteralPath $unsafeMerged -Force}
    $flashArgs=Join-Path $resolvedDir 'flash_args'
    if(Test-Path -LiteralPath $flashArgs){
      $safeArgs=Get-Content -LiteralPath $flashArgs | Where-Object {$_ -notmatch '^\s*0x[eE]000\s'}
      $safeArgs | Set-Content -LiteralPath $flashArgs -Encoding ascii
    }
    $manifest=@{Fqbn=$target.Fqbn;PartitionCsvSha256=(Get-FileHash -LiteralPath (Join-Path $repoRoot ($paths.($target.Name)+"/partitions.csv")) -Algorithm SHA256).Hash;Files=@{}}
    foreach($suffix in @('bin','bootloader.bin','partitions.bin')){
      $filename="$($target.Name).ino.$suffix"
      $manifest.Files[$filename]=(Get-FileHash -LiteralPath (Join-Path $resolvedDir $filename) -Algorithm SHA256).Hash
    }
    $manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $resolvedDir 'safe-flash-manifest.json') -Encoding utf8
  }
}
Write-Host 'All builds passed. No device was flashed. Read the T-Dongle upload note before uploading.'
