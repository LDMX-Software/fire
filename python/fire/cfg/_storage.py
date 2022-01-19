"""Configuration of storage/veto system in fire"""

from ._processor import Processor

class ListeningRule :
    """A single rule telling storage control whether to listen to
    a set of processors

    Parameters
    ----------
    processor_regex : str
        regular expression matching processors to listen to
    purpose_regex : str
        regular expression matching purpoeses to listen to

    Note
    ----
    Empty regex strings are translated to the "match all" regex '.*'
    on the C++ side of configuration.
    """

    def __init__(self, processor_regex, purpose_regex) :
        self.processor = processor_regex
        self.purpose = purpose_regex

    def __repr__(self) :
        return f'Name {self.processor} AND Purpose {self.purpose}'

    def __str__(self) :
        return repr(self)

class StorageControl :
    """Configuration for how events are chosen to be stored.
    This configuration keeps the default storage decision and
    any rules about which processors to listen to when they are 
    making storage hints.
    
    Attributes
    ----------
    default_keep : bool
        True if the default storage is to keep the event
    listening_rules : list[ListeningRule]
        List of rules about processors/purposes to listen to
    """

    def __init__(self) :
        self.default_keep = True
        self.listening_rules = []

    def default(self, keep = True) :
        self.default_keep = keep

    def listen(self, processor) :
        """Add a listening rule that will listen to the processor with
        name exactly matching the name of the input processor object

        Parameters
        ----------
        processor : Processor
            Processor object to listen to
        """
        if not isinstance(processor, Processor) :
            raise Exception(f'{processor} is not an instance of a Processor')
        
        self.listening_rules.append(ListeningRule(f'^{processor.name}$','.*'))

    def listen_all(self) :
        self.listening_rules = [ ListeningRule('.*','.*') ]

    def __str__(self) :
        dk = 'drop'
        if self.default_keep :
            dk = 'keep'
        return f'Storage(default: {dk}, listening: {self.listening_rules})'

class DropKeepRule :
    """A single rule specifying if a specific event object should be
    saved into the output file (keep) or not (drop)

    Parameters
    ----------
    keep : bool
        True if the event object matching the regex should be kept
    regex : str
        Regular expression for this rule
    """

    def __init__(self,regex,keep) :
        self.keep = keep
        self.regex = regex

    def __repr__(self) :
        dk = 'drop'
        if self.keep :
            dk = 'keep'
        return f'{dk}({self.regex})'

    def __str__(self) :
        return repr(self)

