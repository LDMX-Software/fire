"""Configuration of fire Process"""

from ._output_file import OutputFile

class Process:
    """Process configuration object

    The python object that stores the necessary parameters for configuring
    a Process for fire to execute.

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
        Maximum number of attempts to make in a row before giving up on an event. 
        Only used in Production Mode (no input files)
    run : int
        Run number for this process
    input_files : list of strings
        Input files to read in event data from and process
    output_file : OutputFile
        Output file to write out event data to after processing
    storage : StorageControl
        Configuration of veto-ing decision maker
    sequence : list of Processor
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
    conditions : Conditions
        System handling providers as well as the global tag

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
        self.output_file = OutputFile('')
        self.sequence = []
        self.drop_keep_rules = []

        # import storage here to prevent circular dependencies
        from . import _storage
        self.storage = _storage.StorageControl()

        self.log_frequency = -1
        self.term_level = 2 #warnings and above
        self.file_level = 0 #print all messages
        self.log_file   = '' #won't setup log file

        # import conditions here to prevent circular dependencies
        from . import _conditions
        self.conditions = _conditions.Conditions('Default')

        Process.lastProcess=self

        # needs lastProcess defined to self-register
        #   will be the first conditions provider
        from . import _rnss
        _rnss.RandomNumberSeedService()

    def __setattr__(self, name, value) :
        """Override setting attributes to allow for legacy translation and wrapping
        of helper classes

        We do the following translations

        - maxEvents -> event_limit
        - maxTriesPerEvent -> max_tries
        - inputFiles -> input_files
        - logFrequency -> log_frequency
        - termLogLevel -> term_level
        - fileLogLevel -> file_level
        - logFileName -> log_file
        - outputFiles -> output_file

        If more than one output file is given, an AttributeError is thrown.

        For the 'keep' attribute, the provided list is parsed into drop keep rules
        assuming the legacy syntax where each rule is '<drop/keep> <regex>'.

        If value is a str and name (post translation) is output_file, then
        value is wrapped in the OutputFile constructor inheriting the default parameters.

        When legacy configuration and processors are fully removed,
        this function can be shortened to only the wrapping of internal objects.
        """

        if name == 'maxEvents' :
            name = 'event_limit'
        elif name == 'maxTriesPerEvent' :
            name = 'max_tries'
        elif name == 'inputFiles' :
            name = 'input_files'
        elif name == 'logFrequency' :
            name = 'log_frequency'
        elif name == 'termLogLevel' :
            name = 'term_level'
        elif name == 'fileLogLevel' :
            name = 'file_level'
        elif name == 'logFileName' :
            name = 'log_file'
        elif name == 'outputFiles' :
            name = 'output_file'
            if isinstance(value,list) and len(value) != 1 :
                raise AttributeError('Only one output file is allowed.')
            value = value[0]
        elif name == 'keep' :
            for rule in value :
                # assume value is space sep string
                decision, regex = rule.split()
                if decision == 'keep' :
                    self.keep(regex)
                elif decision == 'drop' :
                    self.drop(regex)
                else :
                    raise AttributeError(f'Drop keep rule {rule} does not have a decision ("drop" or "keep")')
            # leave early because we already modified 'dict' in above member functions
            return

        if name == 'output_file' and not isinstance(value,OutputFile) :
            value = OutputFile(value)

        self.__dict__[name] = value

    def skimConsider(self, namePat) :
        """Legacy function, user is encouraged to use

            p.storage.listen(processor)

        rather than this function.
        """
        from ._storage import ListeningRule
        self.storage.listening_rules.append(ListeningRule(namePat,''))

    def skimDefaultIsDrop(self) :
        """Legacy function, user is encouraged to use

            p.storage.default(False)

        rather than this function.
        """
        self.storage.default(False)

    def skimDefaultIsSave(self) :
        """Legacy function, user is encouraged to use

            p.storage.default(True)

        rather than this function.
        """
        self.storage.default(True)

    def rnss(self) :
        """Get the random number seed service condition

        We create that condition in the constructor of the process,
        so we can trust that it is the first entry in the conditions
        provider list.

        Returns
        -------
        RandomNumberSeedService for more configuration

        Examples
        --------
        You would use this function if you wanted to change
        the mode with which the random numbers are seeded.

            p.rnss().time()
        """
        return self.conditions.providers[0]

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

        Warnings
        --------
            We assume that the libraries are prefixed with 'lib' and
            have the '.so' extension. This limits the usefulness of this
            function to Linux systems.

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
        Process.addLibrary(f'lib{actual_module_name}.so')

    def keep(self,regex) :
        """Add a regex rule for keeping event objects whose name matches the regex"""
        from . import _storage
        self.drop_keep_rules.append(_storage.DropKeepRule(regex,True))

    def drop(self,regex) :
        """Add a regex rule for dropping event objects whose name matches the regex"""
        from . import _storage
        self.drop_keep_rules.append(_storage.DropKeepRule(regex,False))

    def declareConditionsProvider(cp):
        """Declare a conditions object provider to be loaded with the process

        A process object must already have been created.

        Parameters
        ----------
        cp : ConditionsProvider
            provider to load with the process

        Warnings
        --------
        - Will exit the script if a process object hasn't been defined yet.
        - Overrides an already declared COP with the passed COP if they are equal
        """

        if ( Process.lastProcess is not None ) :

            cp.setTag(Process.lastProcess.conditions.global_tag)

            # check if the input COP matches one already declared
            #   if it does match, override the already declared one with the passed one
            for index, already_defined_cp in enumerate(Process.lastProcess.conditions.providers) :
                if cp == already_defined_cp :
                    Process.lastProcess.conditions.providers[index] = cp
                    return

            Process.lastProcess.conditions.providers.append( cp )
        else :
            raise Exception( "No Process object defined yet! You need to create a Process before declaring any ConditionsProviders." )

    def setConditionsGlobalTag(self,tag) :
        """Set the global tag for all the ConditionsProviders

        Parameters
        ----------
        tag : str
            Global generation tag to pass to all COPs
        """

        self.conditions.global_tag = tag
        for cp in self.conditions.providers :
            cp.setTag(tag)

    def inputDir(self, indir, ext = 'h5') :
        """Scan the input directory and make a list of input root files to read from it

        Lists all files ending in '.root' in the input directory (not recursive).
        Extends the input_files list by these files.

        Parameters
        ----------
        indir : str
            Path to directory of event files to read in
        ext : str, optional
            Filter by the input extension string, default: 'h5'
        """

        import os
        fullPathDir = os.path.realpath(indir)
        self.input_files.extend([ os.path.join(fullPathDir,f) 
                for f in os.listdir(fullPathDir) 
                if os.path.isfile(os.path.join(fullPathDir,f)) and f.endswith(ext)
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
        msg += f'\n{str(self.conditions)}'
        msg += '\nProcessor sequence:'
        for proc in self.sequence:
            msg += '\n ' + str(proc)
        if len(self.input_files) > 0:
            msg += "\n Input files:"
            for afile in self.input_files:
                msg += '\n  ' + afile
        msg += f"\n {self.output_file}"
        msg += f"\n {self.storage}"
        if len(self.drop_keep_rules) > 0 :
            msg += f'\n DK Rules: {str(self.drop_keep_rules)}'
        return msg

