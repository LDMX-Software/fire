"""Configuration class for Processors"""

from ._process import Process

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
    fire.cfg.Process.addLibrary : how libraries are loaded
    """

    def __init__(self, name, class_name, module = None, library = None, **kwargs):
        self.__dict__ = kwargs
        self.name = name
        self.class_name = class_name

        if module is not None :
            Process.addModule(module)

        if library is not None :
            Process.addLibrary(library)

    def __repr__(self) :
        """Represent this processor with its Python class name, instance name, and C++ name"""
        return f'{self.__class__.__name__}({self.name} of class {self.class_name})'

    def __str__(self) :
        """A full str print of the processor includes the repr as well as all of its parameters"""
        msg = f'{repr(self)}'
        if len(self.__dict__) > 2 :
            msg += '\n  Parameters:'
            for k, v in self.__dict__.items() :
                if k not in ['name','class_name'] :
                    msg += f'\n   {str(k)} : {str(v)}'
        return msg
