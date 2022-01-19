"""Configuration of output files of fire"""

class OutputFile :
    """Configuration for writing an output file

    Parameters
    ----------
    name : str
        Name of file to write
    rows_per_chunk : int, optional
        Number of "rows" in the output file to "chunk" together
    """

    def __init__(self, name, rows_per_chunk = 10000, compression_level = 6, shuffle = False) :
        self.name = name
        self.rows_per_chunk = rows_per_chunk
        self.compression_level = compression_level
        self.shuffle = shuffle

    def __repr__(self) :
        return f'OutputFile({self.name})'

    def __str__(self) :
        return str(self.__dict__)
