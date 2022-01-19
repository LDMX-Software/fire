"""RandomNumberSeedService (RNSS) configuration"""

from . import _conditions

class RandomNumberSeedService(_conditions.ConditionsProvider):
    """The random number seed service

    This object registers the random number seed service with the process and
    gives some helper functions for configuration.

    Attributes
    ----------
    seedMode : str
        Name of mode of getting random seeds
    """

    def __init__(self) :
        super().__init__('RandomNumberSeedService','fire::RandomNumberSeedService','fire::framework')
        self.seedMode = ''
        self.seed=-1 #only used in external mode
        self.overrides = {}

        # use run seed mode by default
        self.run()

    def run(self) :
        """Base random number seeds off of the run number"""
        self.seedMode = 'run'

    def external(self,seed) :
        """Input the master random number seed

        Parameters
        ----------
        seed : int
            Integer to use as master random number seed
        """
        self.seedMode = 'external'
        self.seed = seed

    def time(self) :
        """Set master random seed based off of time"""
        self.seedMode = 'time'

    def override(self, name, seed) :
        """Override the RNSS and provide your own seed for a specific name"""

        self.overrides[name] = seed
