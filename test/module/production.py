
import fire.cfg

p = fire.cfg.Process('mypass')

p.event_limit = 10

p.output_file = fire.cfg.OutputFile('production_output.h5')

p.sequence = [ fire.cfg.Producer('make','mymodule::MyProducer','MyModule') ]

p.term_level = 0
p.log_frequency = 1

print(p)
