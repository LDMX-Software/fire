import fire.cfg
p = fire.cfg.Process('bench')
import sys
p.input_files = sys.argv[1:]
import os
p.output_file = fire.cfg.OutputFile(f'test/module/recon_{os.path.basename(p.input_files[0])}')
p.sequence = [ fire.cfg.Producer('make','bench::Recon','Bench') ]
