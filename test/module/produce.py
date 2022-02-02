import fire.cfg
p = fire.cfg.Process('bench')
import sys
p.event_limit = int(sys.argv[1])
p.output_file = fire.cfg.OutputFile(f'test/module/output_{p.event_limit}.h5')
p.sequence = [ fire.cfg.Processor('make','bench::Produce',module='Bench') ]
