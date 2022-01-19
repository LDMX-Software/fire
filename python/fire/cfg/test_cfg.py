"""Test configuration module of fire

This is simply checking that things run and methods
aren't broken. Since the configuration module is
only responsible for copying values into C++,
"""

import pytest

def test_cfg() :
    import fire.cfg
    with pytest.raises(Exception) :
        fire.cfg.Process.addModule('ShouldntWork')

    p = fire.cfg.Process('test')
    fire.cfg.Process.addModule('ShouldWork')
    p.output_file = fire.cfg.OutputFile('test.h5')
    p.keep('.*')
    p.drop('.*')

    assert p.conditions.providers[-1] == p.rnss
    assert p.rnss.seedMode == 'run'
    p.rnss.time()
    assert p.rnss.seedMode == 'time'
    p.rnss.external(420)
    assert p.rnss.seedMode == 'external'
    assert p.rnss.seed == 420

    # check auto registration of conditions providers
    cp = fire.cfg.ConditionsProvider('Provides','test::Provider','CPModule')
    assert p.conditions.providers[-1] == cp
    assert p.libraries[-1] == 'libCPModule.so'

    p.setConditionsGlobalTag('NewDefault')
    for cp in p.conditions.providers :
        assert cp.tag_name == 'NewDefault'

    proc = fire.cfg.Processor('test','TestPythonConf','TestModule')
    # check auto registration
    assert p.libraries[-1] == 'libTestModule.so'
    # add proc to sequence to test printing later
    p.sequence = [ proc ]

    p.storage.default(False)
    with pytest.raises(Exception) :
        p.storage.listen('not_a_processor_instance')
    p.storage.listen(proc)

    import re
    assert re.match(p.storage.listening_rules[-1].processor,proc.name)

    p.storage.listen_all()
    assert re.match(p.storage.listening_rules[-1].processor,'literally_anything')
    assert re.match(p.storage.listening_rules[-1].purpose  ,'literally_anything')

    # set up some dummy files to test directory globbing
    import os
    test_input_d = 'test_input_crawl'
    os.makedirs(test_input_d,exist_ok=True)
    file_names = ['one.h5','two.h5','three.h5']
    exclude = ['noop.root','listing.txt']
    for fn in file_names+exclude :
        open(f'{test_input_d}/{fn}','a').close()

    p.inputDir(test_input_d)

    # need to sort because file globbing doesn't order the same as our list necessarily
    assert p.input_files.sort() == [
            os.path.realpath(os.path.join(test_input_d,fn)) 
            for fn in file_names].sort()

    dummy_var = str(p)
    dummy_var = p.parameterDump()
    with pytest.raises(Exception) :
        p2 = fire.cfg.Process('CantCreateTwo')

