from LDMX.Framework import ldmxcfg
p = ldmxcfg.Process('rootgen')
import sys
p.maxEvents = int(sys.argv[1])
p.outputFiles = [ f'test/rootgen/output_{p.maxEvents}.root' ]
p.sequence = [ ldmxcfg.Producer('make','bench::Produce','Module') ]
