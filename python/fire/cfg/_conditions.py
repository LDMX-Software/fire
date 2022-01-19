"""Configuration of conditions system for fire"""

from ._process import Process

class ConditionsProvider:
    """A ConditionsProvider

    This object contains the parameters that are necessary for 
    a fire::ConditionsProvider to be configured.

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
    tag_name : str
        Tag which identifies the generation of information

    See Also
    --------
    fire.cfg.Process.addModule : how modules are interpreted as libraries to load
    fire.cfg.Process.declareConditionsProvider : how COPs are registered
    """

    def __init__(self, object_name, class_name, module):
        self.obj_name=object_name
        self.class_name=class_name
        self.tag_name=''

        # make sure process loads this library if it hasn't yet
        Process.addModule(module)
        
        #register this conditions object provider with the process
        Process.declareConditionsProvider(self)

    def setTag(self,newtag) :
        """Set the tag generation of the Conditions

        Parameters
        ----------
        newtag : str
            Tag for generation of conditions
        """

        self.tag_name=newtag

    def __eq__(self,other) :
        """Check if two COPs are the same

        We decide that two COPs are 'equal' if they have the same instance and class names
        
        Parameters
        ----------
        other : ConditionsProvider
            other COP to compare agains
        """

        if not isinstance(other,ConditionsProvider) :
            return NotImplemented

        return (self.objectName == other.objectName and self.className == other.className)

    def __repr__(self) :
        """Represent this provider with its Python class name, instance name, and C++ name"""
        return f'{self.__class__.__name__}({self.class_name} providing {self.obj_name})'

    def __str__(self) :
        """A full str print of the processor includes the repr as well as all of its parameters"""
        msg = f'{repr(self)}'
        if len(self.__dict__) > 3 :
            msg += '\n  Parameters:'
            for k, v in self.__dict__.items() :
                if k not in ['name','class_name'] :
                    msg += f'\n   {str(k)} : {str(v)}'
        return msg

class Conditions :
    """The configuration for the central conditions system"""

    def __init__(self, global_tag = 'Default') :
        self.global_tag = 'Default'
        self.providers = []

    def __repr__(self) :
        return f'Conditions(tag = {self.global_tag})'

    def __str__(self) :
        return f'{repr(self)}\n {repr(self.providers)}'
