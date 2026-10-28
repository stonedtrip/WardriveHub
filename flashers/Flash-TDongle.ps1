[CmdletBinding(SupportsShouldProcess=$true)]
param(
  [Parameter(Mandatory=$true)][string]$Port,
  [string]$BuildDirectory=(Join-Path $PSScriptRoot '../.validation-build/WARDRIVE_HUB_C5_T_Dongle_Node'),
  [string]$Esptool=(Join-Path $env:LOCALAPPDATA 'Arduino15/packages/esp32/tools/esptool_py/5.3.1/esptool.exe')
)
$ErrorActionPreference='Stop'
$name='WARDRIVE_HUB_C5_T_Dongle_Node'
$originalPartitionSha='8F0AEC6A70DC05036FA6DB16E385EC8EB34636B0825F35A0FA74C6E1C6B22B0D'
$csv=Join-Path $PSScriptRoot "../firmware/nodes/esp32-c5/t-dongle/WARDRIVE_HUB_C5_T_Dongle_Node/partitions.csv"
if((Get-FileHash -LiteralPath $csv -Algorithm SHA256).Hash -ne $originalPartitionSha){throw 'Partition table differs from the reviewed factory layout.'}
$manifest=Get-Content -LiteralPath (Join-Path $BuildDirectory 'safe-flash-manifest.json') -Raw | ConvertFrom-Json
if($manifest.PartitionCsvSha256 -ne $originalPartitionSha){throw 'Rebuild using build.ps1; partition manifest does not match.'}
if(!(Test-Path -LiteralPath $Esptool)){throw 'Pass -Esptool with the esptool.exe path from your ESP32 Arduino installation.'}
foreach($entry in $manifest.Files.PSObject.Properties){
  if($entry.Name -notin @("$name.ino.bin","$name.ino.bootloader.bin","$name.ino.partitions.bin")){throw 'Unexpected manifest file.'}
  if((Get-FileHash -LiteralPath (Join-Path $BuildDirectory $entry.Name) -Algorithm SHA256).Hash -ne $entry.Value){throw "Build file changed: $($entry.Name)"}
}
if(@($manifest.Files.PSObject.Properties).Count -ne 3){throw 'Incomplete build manifest.'}
$boot=Join-Path $BuildDirectory "$name.ino.bootloader.bin"
$parts=Join-Path $BuildDirectory "$name.ino.partitions.bin"
$app=Join-Path $BuildDirectory "$name.ino.bin"
if((Get-Item -LiteralPath $boot).Length -gt 0x6000){throw 'Bootloader would overlap the partition table.'}
if((Get-Item -LiteralPath $parts).Length -gt 0x1000){throw 'Partition binary would overlap NVS.'}
if((Get-Item -LiteralPath $app).Length -gt 0x300000){throw 'Application exceeds the factory slot.'}
if($PSCmdlet.ShouldProcess($Port,'Flash reviewed T-Dongle factory layout: bootloader 0x2000, partitions 0x8000, app 0x10000; preserve NVS at 0xe000')){
  & $Esptool --chip esp32c5 --port $Port --baud 460800 --before default-reset --after hard-reset write-flash -z --flash-mode keep --flash-freq keep --flash-size keep 0x2000 $boot 0x8000 $parts 0x10000 $app
  if($LASTEXITCODE -ne 0){throw 'Upload failed.'}
}
