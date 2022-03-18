"""Test legacy configuration module of fire

This is simply checking that things run and methods
aren't broken. Since the configuration module is
only responsible for copying values into C++,
"""

import pytest

def test_legacy() :
    from LDMX.Framework import ldmxcfg
    # make sure start from blank slate
    ldmxcfg.Process.lastProcess = None
    with pytest.raises(Exception) :
        ldmxcfg.Process.addModule('ShouldntWork')

    p = ldmxcfg.Process('test')
    ldmxcfg.Process.addModule('ShouldWork')
    p.outputFiles = ['test.h5']
    p.keep = ['keep .*', 'drop .*']

    assert p.conditions.providers[-1] == p.rnss()
    assert p.rnss().seedMode == 'run'
    p.rnss().time()
    assert p.rnss().seedMode == 'time'
    p.rnss().external(420)
    assert p.rnss().seedMode == 'external'
    assert p.rnss().seed == 420

    # check auto registration of conditions providers
    cp = ldmxcfg.ConditionsObjectProvider('Provides','test::Provider','CPModule')
    assert p.conditions.providers[-1] == cp
    assert p.libraries[-1] == 'libCPModule.so'

    p.setConditionsGlobalTag('NewDefault')
    for cp in p.conditions.providers :
        assert cp.tag_name == 'NewDefault'

    proc = ldmxcfg.Producer('test','TestPythonConf','TestModule')
    # check auto registration
    assert p.libraries[-1] == 'libTestModule.so'

    proc2 = ldmxcfg.Analyzer('test2','Testing',library='/full/path/to/lib.so')
    assert p.libraries[-1] == '/full/path/to/lib.so'

    # add proc to sequence to test printing later
    p.sequence = [ proc ]

    assert p.storage.default_keep
    p.skimDefaultIsDrop()
    assert not p.storage.default_keep

    p.skimConsider('test2')
    assert p.storage.listening_rules[0].processor == 'test2'

    with pytest.raises(Exception) :
        p2 = ldmxcfg.Process('CantCreateTwo')

