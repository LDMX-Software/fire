"""Legacy configuration module

We focus on providing the same front-facing interaction with users
while translating to the new mode under-the-hood.
"""

from fire.cfg import Process
from fire.cfg import Processor
from fire.cfg import ConditionsProvider as ConditionsObjectProvider

Producer = Processor
Analyzer = Processor

