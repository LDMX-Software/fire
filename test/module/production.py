
import fire.cfg

p = fire.cfg.Process('mypass')

p.event_limit = 10

p.output_file = fire.cfg.OutputFile('production_output.h5')

p.sequence = [ fire.cfg.Producer('make','mymodule::MyProducer','MyModule') ]

p.pause()
