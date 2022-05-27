import h5py
import numpy as np
import sys
with h5py.File(sys.argv[1]) as f :
    assert f['events/EventHeader/isRealData'].dtype == np.dtype('bool')
