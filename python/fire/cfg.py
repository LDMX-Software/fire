"""cfg

Basic python configuration for fire
"""

class Processor:
    """A Processor configuration object

    This object contains the parameters that are necessary for a fire::Processor to be configured.

    You should NOT use this class directly. Use one of the derived classes Producer or Analyzer for clarity.

    Parameters
    ----------
    name : str
        Name of this copy of the processor object
    class_name : str
        Name (including namespace) of the C++ class that this processor should be
    module : str
        Name of module the C++ class is in (i.e. the library that should be loaded)

    See Also
    --------
    fire.cfg.Process.addModule : how module names are interpreted as libraries to be loaded
    """

    def __init__(self, name, class_name, module, **kwargs):
        self.___dict__ = kwargs
        self.name = name
        self.class_name = class_name

        Process.addModule(module)

    def __repr__(self) :
        """Represent this processor with its Python class name, instance name, and C++ name"""
        return f'{self.__class__.__name__}({self.name} of class {self.class_name})'

    def __str__(self) :
        """A full str print of the processor includes the repr as well as all of its parameters"""
        msg = f'\n {repr(self)}'
        if len(self.__dict__) > 2 :
            msg += '\n  Parameters:'
            for k, v in self.__dict__.items() :
                if k in ['name','class_name'] :
                    msg += f'\n   {str(k)} : {str(v)}'
        return msg

class Producer(Processor):
    """A producer object.

    This object contains the parameters that are necessary for a framework::Producer to be configured.

    See Also
    --------
    fire.cfg.Processor : base class
    """

    def __init__(self, name, class_name, module, **kwargs)
        super().__init__(name, class_name, module, **kwargs)

class Analyzer(Processor):
    """A analyzer object.

    This object contains the parameters that are necessary for a framework::Analyzer to be configured.

    See Also
    --------
    fire.cfg.Processor : base class
    """

    def __init__(self, name, class_name, module, **kwargs)
        super().__init__(name, class_name, module, **kwargs)

class ConditionsObjectProvider:
    """A ConditionsObjectProvider

    This object contains the parameters that are necessary for a fire::ConditionsObjectProvider to be configured.

    In this constructor we also do two helpful processes.
    1. We append the module that this provider is in to the list of libraries to load
    2. We declare this provider so that the Process "knows" it exists and will load it into the run

    Parameters
    ----------
    objectName : str
        Name of the object this provider provides
    className : str
        Name (including namespace) of the C++ class of the provider
    moduleName : str
        Name of module that this COP is compiled into (e.g. Ecal or EventProc)

    Attributes
    ----------
    tagName : str
        Tag which identifies the generation of information

    See Also
    --------
    fire.cfg.Process.addModule : how modules are interpreted as libraries to load
    fire.cfg.Process.declareConditionsObjectProvider : how COPs are registered
    """

    def __init__(self, object_name, class_name, module):
        self.objectName=object_name
        self.className=class_name
        self.tagName=''

        # make sure process loads this library if it hasn't yet
        Process.addModule(module)
        
        #register this conditions object provider with the process
        Process.declareConditionsObjectProvider(self)

    def setTag(self,newtag) :
        """Set the tag generation of the Conditions

        Parameters
        ----------
        newtag : str
            Tag for generation of conditions
        """

        self.tagName=newtag

    def __eq__(self,other) :
        """Check if two COPs are the same

        We decide that two COPs are 'equal' if they have the same instance and class names
        
        Parameters
        ----------
        other : ConditionsObjectProvider
            other COP to compare agains
        """

        if not isinstance(other,ConditionsObjectProvider) :
            return NotImplemented

        return (self.objectName == other.objectName and self.className == other.className)

    def __str__(self) :
        """Stringify this ConditionsObjectProvider, creates a message with all the internal parameters.

        Returns
        -------
        str
            A message with all the parameters and member variables in a human readable format
        """

        msg = "\n  ConditionsObjectProvider(%s of class %s, tag='%s')"%(self.objectName,self.className,self.tagName)
        if len(self.__dict__)>0:
            msg += "\n   Parameters:"
            for k, v in self.__dict__.items():
                msg += "\n    " + str(k) + " : " + str(v)

        return msg

class RandomNumberSeedService(ConditionsObjectProvider):
    """The random number seed service

    This object registers the random number seed service with the process and
    gives some helper functions for configuration.

    Attributes
    ----------
    seedMode : str
        Name of mode of getting random seeds
    """

    def __init__(self) :
        super().__init__('RandomNumberSeedService','framework::RandomNumberSeedService','Framework')
        self.seedMode = ''
        self.seed=-1 #only used in external mode

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

class OutputFile :
    """Configuration for writing an output file

    Parameters
    ----------
    name : str
        Name of file to write
    rows_per_chunk : int, optional
        Number of "rows" in the output file to "chunk" together
    """

    def __init__(self, name, rows_per_chunk = 10000) :
        self.name = name
        self.rows_per_chunk = rows_per_chunk

    def __repr__(self) :
        return f'OutputFile({self.name})'

    def __str__(self) :
        return str(self.__dict__)

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

    def default(keep = True) :
        self.default_keep = keep

    def listen(processor) :
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

    def listen_all() :
        self.listening_rules = [ ListeningRule('.*','.*') ]

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

class Process:
    """Process configuration object

    The python object that stores the necessary parameters for configuring
    a Process for ldmx-app to execute.

    Upon construction, the class-wide reference lastProcess is set
    and the rest of the attributes are set to sensible defaults.

    Parameters
    ----------
    pass_name : str
        Short reference name for this run of the process

    Attributes
    ----------
    lastProcess : Process
        Class-wide reference to the last Process object to be constructed
    event_limit : int
        Maximum number events to process
    max_tries : int
        Maximum number of attempts to make in a row before giving up on an event
        Only used in Production Mode (no input files)
    run : int
        Run number for this process
    input_files : list of strings
        Input files to read in event data from and process
    output_file : OutputFile
        Output file to write out event data to after processing
    storage : StorageControl
        Configuration of veto-ing decision maker
    sequence : list of Producers and Analyzers
        List of event processors to pass the event bus objects to
    drop_keep_rules : list of DropKeepRule
        List of rules to keep or drop objects from the event bus
    libraries : list of strings
        List of libraries to load before attempting to build any processors
    log_frequency : int
        Print the event number whenever its modulus with this frequency is zero
    term_level : int
        Minimum severity of log messages to print to terminal: 0 (debug) - 4 (fatal)
    file_level : int
        Minimum severity of log messages to print to file: 0 (debug) - 4 (fatal)
    log_file : str
        File to print log messages to, won't setup file logging if this parameter is not set

    conditionsGlobalTag : str
        Global tag for the current generation of conditions
    conditionsObjectProviders : list of ConditionsObjectProviders
        List of the sources of calibration and conditions information
    randomNumberSeedService : RandomNumberSeedService
        conditions object that provides random number seeds in a deterministic way

    See Also
    --------
    fire.cfg.Producer : one type of event processor
    fire.cfg.Analyzer : the other type of event processor
    """

    lastProcess=None
    
    def __init__(self, pass_name):

        if ( Process.lastProcess is not None ) :
            raise Exception( "Process object is already created! You can only create one Process object in a script." )

        self.libraries = []
        self.pass_name = pass_name
        self.event_limit = -1
        self.max_tries = 1
        self.run = -1
        self.input_files = []
        self.output_file = None
        self.sequence = []
        self.drop_keep_rules = []
        self.storage = StorageControl()

        self.log_frequency = -1
        self.term_level = 2 #warnings and above
        self.file_level = 0 #print all messages
        self.log_file   = '' #won't setup log file

        self.conditionsGlobalTag = 'Default'
        self.conditionsObjectProviders = []

        Process.lastProcess=self

        # needs lastProcess defined to self-register
        self.randomNumberSeedService=RandomNumberSeedService()

    def addLibrary(lib) :
        """Add a library to the list of dynamically loaded libraries

        A process object must already have been created.

        Parameters
        ----------
        lib : str
            name of library to load 

        Warnings
        --------
        - Will exit the script if a process object hasn't been defined yet.

        Examples
        --------
            fire.cfg.Process.addLibrary( 'libMyModule.so' )
        """

        if ( Process.lastProcess is not None ) :
            Process.lastProcess.libraries.append( lib )
        else :
            raise Exception( "No Process object defined yet! You need to create a Process before creating any Processors." )
    
    def addModule(module) :
        """Add a module to the list of dynamically loaded libraries

        A process object must already have been created.

        Parameters
        ----------
        module : str
            Name of module to load as a library

        See Also
        --------
        Process.addLibrary

        Examples
        --------
        You can use this function to load a general module
            addModule('MyModule')

        With the string substitutions that are made, you can
        refer to submodules with cmake, C++, or the library
        syntax. The following calls are all equivalent.
            addModule('MyModule/Event')
            addModule('MyModule::Event')
            addModule('MyModule_Event')
        """

        actual_module_name = module.replace('/','_').replace('::','_')
        Process.addLibrary('lib%s.so'%(actual_module_name))

    def keep(regex) :
        """Add a regex rule for keeping event objects whose name matches the regex"""
        self.drop_keep_rules.append(DropKeepRule(regex,True))

    def drop(regex) :
        """Add a regex rule for dropping event objects whose name matches the regex"""
        self.drop_keep_rules.append(DropKeepRule(regex,False))

    def declareConditionsObjectProvider(cop):
        """Declare a conditions object provider to be loaded with the process

        A process object must already have been created.

        Parameters
        ----------
        cop : ConditionsObjectProvider
            provider to load with the process

        Warnings
        --------
        - Will exit the script if a process object hasn't been defined yet.
        - Overrides an already declared COP with the passed COP if they are equal
        """

        if ( Process.lastProcess is not None ) :

            cop.setTag(Process.lastProcess.conditionsGlobalTag)

            # check if the input COP matches one already declared
            #   if it does match, override the already declared one with the passed one
            for index, already_defined_cop in enumerate(Process.lastProcess.conditionsObjectProviders) :
                if cop == already_defined_cop :
                    Process.lastProcess.conditionsObjectProviders[index] = cop
                    return

            Process.lastProcess.conditionsObjectProviders.append( cop )
        else :
            raise Exception( "No Process object defined yet! You need to create a Process before declaring any ConditionsObjectProviders." )

    def setConditionsGlobalTag(self,tag) :
        """Set the global tag for all the ConditionsObjectProviders

        Parameters
        ----------
        tag : str
            Global generation tag to pass to all COPs
        """

        self.conditionsGlobalTag=tag
        for cop in self.conditionsObjectProviders :
            cop.setTag(tag)

    def inputDir(self, indir) :
        """Scan the input directory and make a list of input root files to read from it

        Lists all files ending in '.root' in the input directory (not recursive).
        Extends the input_files list by these files.

        Parameters
        ----------
        indir : str
            Path to directory of event files to read in
        """

        import os
        fullPathDir = os.path.realpath(indir)
        self.input_files.extend([ os.path.join(fullPathDir,f) 
                for f in os.listdir(fullPathDir) 
                if os.path.isfile(os.path.join(fullPathDir,f)) and f.endswith('.root') 
                ])

    def parameterDump(self) :
        """Recursively extract all configuration parameters for this process

        Only includes objects somehow attached to the process.
        """
        keys_to_skip = [ 'libraries' ]

        def extract(obj):
            """Extract the parameter from the input object"""

            if isinstance(obj,list) :
                return [ extract(o) for o in obj ]
            elif hasattr(obj,'__dict__') :
                params = dict()
                for k in obj.__dict__ :
                    if k not in keys_to_skip :
                        params[k] = extract(obj.__dict__[k])
                return params
            else :
                return obj

        return extract(self)


    def pause(self) :
        """Print this Process and wait for user confirmation to continue

        Prints the process through the print function, and then
        waits for the user to press Enter to continue.
        """

        print(self)
        input("Press Enter to continue...")

    def __str__(self):
        """Stringify this object into a human readable, helpful form.

        This function creates a very large, multi-line string that reports (almost) all of the important
        details of this configured process.

        Returns
        -------
        str
            A human-readable, multi-line description of this process object
        """

        msg = "Process with pass name '%s'"%(self.pass_name)
        if (self.run>0): msg += "\n using run number %d"%(self.run)
        if (self.event_limit>0): msg += "\n Maximum events to process: %d"%(self.event_limit)
        else: msg += "\n No limit on maximum events to process"
        if (len(self.conditionsObjectProviders)>0):
            msg += "\n conditionsObjectProviders:\n";
            for cop in self.conditionsObjectProviders:
                msg+=str(cop)
        msg += "\n Processor sequence:"
        for proc in self.sequence:
            msg += str(proc)
        if len(self.input_files) > 0:
            msg += "\n Input files:"
            for afile in self.input_files:
                msg += '\n  ' + afile
        msg += "\n Output file: " + self.output_file[0]
        msg += "\n Skim rules:"
        if self.skimDefaultIsKeep: msg += "\n  Default: keep the event"
        else: msg += "\n  Default: drop the event"
        for i in range(0,len(self.skimRules)-1,2):
            if self.skimRules[i+1]=="": 
                msg += "\n  Listen to hints from processors with names matching '%s'"%(self.skimRules[i])
            else:
                msg += "\n  Listen to hints with labels matching '%s' from processors with names matching '%s'"%(self.skimRules[i+1],self.skimRules[i])
        if len(self.keep) > 0:
            msg += "\n Rules for keeping previous products:"
            for arule in self.keep:
                msg += '\n  ' + arule
        if len(self.libraries) > 0:
            msg += "\n Shared libraries to load:"
            for afile in set(self.libraries):
                msg += '\n  ' + afile

        return msg

    
