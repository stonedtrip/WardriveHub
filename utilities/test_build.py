"""Computer-only checks of T-Dongle packaging guards; no compiler or device."""
import importlib.util,json,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('builder',Path(__file__).with_name('build.py'))
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
NAME='WARDRIVE_HUB_C5_T_Dongle_Node'
CSV=b.ROOT/b.TARGETS[NAME]/'partitions.csv'
class Guards(unittest.TestCase):
 def exercise(self,rc=0,size=10,tamper=False):
  parent=(Path.cwd()/'.build-tests').resolve()
  assert parent.is_relative_to(Path.cwd().resolve())
  parent.mkdir(exist_ok=True)
  self.temp=tempfile.TemporaryDirectory(dir=parent)
  root=Path(self.temp.name).resolve();assert root.is_relative_to(parent)
  self.addCleanup(self.temp.cleanup)
  sketch=root/'sketch';sketch.mkdir(parents=True)
  (sketch/'partitions.csv').write_bytes(CSV.read_bytes()+(b'\n' if tamper else b''))
  out=root/'.validation-build'/NAME;out.mkdir(parents=True)
  (out/'safe-flash-manifest.json').write_text('stale')
  def run(cmd):
   for suffix in ['bootloader.bin','partitions.bin','merged.bin']:(out/(NAME+'.ino.'+suffix)).write_bytes(b'123')
   (out/(NAME+'.ino.bin')).write_bytes(b'X'*size)
   (out/'flash_args').write_text('0x2000 bootloader.bin\n0xe000 boot_app0.bin\n0x10000 app.bin\n')
   return type('Result',(),{'returncode':rc})()
  def cfg(args,**kwargs):return str(root/('data' if args[-1].endswith('.data') else 'user'))
  with patch.object(b,'ROOT',root),patch.object(b,'TARGETS',{NAME:'sketch'}),patch.object(sys,'argv',['build.py','--target',NAME]),patch.object(b.subprocess,'run',run),patch.object(b.subprocess,'check_output',side_effect=cfg):
   try:b.main();error=None
   except SystemExit as e:error=e
  self.assertFalse((out/(NAME+'.ino.merged.bin')).exists())
  self.assertNotIn('0xe000',(out/'flash_args').read_text())
  return out,error
 def test_success_manifest(self):
  out,e=self.exercise();self.assertIsNone(e)
  m=json.loads((out/'safe-flash-manifest.json').read_text());self.assertEqual(len(m['Files']),3)
 def test_failure_removes_stale_manifest(self):
  out,e=self.exercise(rc=1);self.assertIsNotNone(e);self.assertFalse((out/'safe-flash-manifest.json').exists())
 def test_oversize_rejected(self):
  out,e=self.exercise(size=3145729);self.assertIsNotNone(e);self.assertFalse((out/'safe-flash-manifest.json').exists())
 def test_partition_change_rejected(self):
  out,e=self.exercise(tamper=True);self.assertIsNotNone(e);self.assertFalse((out/'safe-flash-manifest.json').exists())
if __name__=='__main__':unittest.main()
