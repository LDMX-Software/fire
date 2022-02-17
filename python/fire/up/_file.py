"""File wrapper for reading fire files"""

import h5py

class File(h5py.File) :
    """Class for reading fire file

    We are a simple specialization of the h5py.File
    that limits us to the specific "meta-format" of our
    HDF5 files.

    We don't override the h5py.File access pattern,
    so the user can still treat this file as a h5py.File
    if they wish to access lower-level information from it.

    Parameters
    ----------
    name : str
        Name of file to load for reading

    Attributes
    ----------
    events : int
        Number of events in the loaded file
    runs : int
        Number of runs in the loaded file
    """

    def __init__(self, name) :
        # performs check on if file exists
        super().__init__(name, 'r')

        # update to h5py API
        self.events = self['events/EventHeader/number'].getDimensions()[0]
        self.runs   = self['runs/number'].getDimensions()[0]

    def arrays(event_objs_spec, cut, library = 'np') :
        """Produce arrays given the input specification of event objects

        Modeled after the uproot.File.arrays method.

        Parameters
        ----------
        event_objs_spec : dict
            specification of event objects
        cut : str?
            choose which events to load based on input cut
        library : str
            which type of array should we return?
        """

        if library == 'np' :
            from ._numpy import load
        elif library == 'ak' :
            from ._awkward import load
        elif library == 'pd' :
            from ._pandas import load
        else :
            raise KeyError(f'Library name {library} not recognized.')

        return load(self, event_objs_spec)

