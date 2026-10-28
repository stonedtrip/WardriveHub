"""Compile one preserved target; never flash or publish."""
import argparse,hashlib,json,subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
TARGETS=json.loads((ROOT/'firmware/targets.json').read_text())
PROFILES={
 'WARDRIVE_HUB_C5_CYD_Core':('esp32:esp32:esp32c5:FlashSize=16M,PSRAM=enabled',3145728),
 'WARDRIVE_HUB_C5_T_Dongle_Node':('esp32:esp32:esp32c5:FlashSize=4M,PSRAM=disabled',3145728),
}
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--target',required=True,choices=TARGETS)
 p.add_argument('--cli',default='arduino-cli');p.add_argument('--config')
 a=p.parse_args();common=[a.cli]+(['--config-file',a.config] if a.config else [])
 profile,limit=PROFILES.get(a.target,('esp32:esp32:XIAO_ESP32C5',3342336))
 cfg={'directories':{key:subprocess.check_output(common+['config','get','directories.'+key],text=True,encoding='utf-8').strip() for key in ['data','user']}}
 maps=[]
 for path,alias in [(Path.home(),'/user'),(Path(cfg['directories']['data']),'/arduino-data'),(Path(cfg['directories']['user']),'/arduino-user'),(ROOT,'/wardrivehub')]:
  for value in dict.fromkeys([str(path),path.as_posix()]):
   maps.append('"-ffile-prefix-map='+value+'='+alias+'"')
 flags=' '.join(maps)
 out=ROOT/'.validation-build'/a.target;out.mkdir(parents=True,exist_ok=True)
 cmd=common+['compile','--fqbn',profile,'--jobs','2','--build-path',str(out),'--build-property',f'upload.maximum_size={limit}','--build-property','compiler.c.extra_flags='+flags,'--build-property','compiler.cpp.extra_flags='+flags]
 if 'T_Dongle' in a.target:cmd+=['--library',str(ROOT/'third_party/LilyGo-display-library')]
 if any(x in a.target for x in ['CYD_Core','Mini']):
  sd=Path(cfg['directories']['data'])/'packages/esp32/hardware/esp32/3.3.12/libraries/SD'
  if not sd.is_dir():raise SystemExit('Install esp32 3.3.12; its SD library was not found.')
  cmd+=['--library',str(sd)]
 cmd+=[str(ROOT/TARGETS[a.target])]
 result=subprocess.run(cmd)
 # Generic merged T-Dongle output is unsafe with the factory NVS layout,
 # including when a failed rebuild leaves older output behind.
 if 'T_Dongle' in a.target:
  (out/(a.target+'.ino.merged.bin')).unlink(missing_ok=True)
  args=out/'flash_args'
  if args.exists():
   import re
   args.write_text('\n'.join(s for s in args.read_text().splitlines() if not re.match(r'^\s*0x[eE]000\s',s))+'\n')
  (out/'safe-flash-manifest.json').unlink(missing_ok=True)
 if result.returncode:raise SystemExit(result.returncode)
 app=out/(a.target+'.ino.bin')
 if not app.exists() or app.stat().st_size>limit:raise SystemExit('Application missing or exceeds reviewed slot.')
 if 'T_Dongle' in a.target:
  digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
  csv=ROOT/TARGETS[a.target]/'partitions.csv'
  if digest(csv).lower()!='8f0aec6a70dc05036fa6db16e385ec8eb34636b0825f35a0fa74c6e1c6b22b0d':raise SystemExit('Unexpected T-Dongle partition layout.')
  m={'Fqbn':profile,'PartitionCsvSha256':digest(csv),'Files':{}}
  for suffix in ['bin','bootloader.bin','partitions.bin']:
   f=out/(a.target+'.ino.'+suffix);m['Files'][f.name]=digest(f)
  (out/'safe-flash-manifest.json').write_text(json.dumps(m,indent=2))
 print('Compile passed. Hardware not tested. Output:',out)
if __name__=='__main__':main()
