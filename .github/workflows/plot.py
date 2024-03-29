"""bench_plot

Plot bench mark data,
this script was developed in the interactive python notebook
of the same name and then copied here for automation.
"""

import matplotlib.pyplot as plt
import pandas as pd

def bench_plot(events, trunk_time, trunk_size, dev_time, dev_size, branch_name, run_mode) :
    """
    Parameters
    ----------
    events : numpy.array
        N events values benchmarked at for both trunk and dev data
    """

    fig, ((raw_time,raw_size),(ratio_time,ratio_size)) = plt.subplots(ncols=2,nrows=2, sharex='col',
            gridspec_kw=dict(height_ratios = [3,1]))
    fig.set_size_inches(11,7)
    plt.suptitle(f'Comparison Between {branch_name} and trunk : {run_mode} Mode')
    plt.subplots_adjust(wspace=0.3, hspace=0.)

    raw_time.set_ylabel('Real Time [s]')
    raw_time.set_yscale('log')
    raw_time.plot(events, trunk_time, label='trunk')
    raw_time.plot(events, dev_time, label=branch_name)
    raw_time.legend()

    ratio_time.set_xscale('log')
    ratio_time.set_xlabel('N Events')
    ratio_time.set_ylabel(f'{branch_name} Time / trunk Time')
    ratio_time.plot(events, dev_time/trunk_time, color = 'black')

    raw_size.set_ylabel('Output File Size [MB]')
    raw_size.set_yscale('log')
    raw_size.plot(events, trunk_size/1e6)
    raw_size.plot(events, dev_size/1e6)

    ratio_size.set_xscale('log')
    ratio_size.set_xlabel('N Events')
    ratio_size.set_ylabel(f'{branch_name} Size / trunk Size')
    ratio_size.plot(events, dev_size/trunk_size, color='black')

def main() :
    import sys, os

    if len(sys.argv) < 2 :
        sys.stderr.write('ERROR: Need to provide file(s) with benchmarking data.\n')
        sys.exit(1)

    name='pr-'+os.environ['GITHUB_PR_NUMBER']

    for data_file in sys.argv[1:] :
        if not os.path.isfile(data_file) :
            sys.stderr.write(f'ERROR: {data_file} not accessible.\n')
            sys.exit(2)

        data = pd.read_csv(data_file)

        filename = os.path.basename(data_file).replace('csv','png')
        dirname = os.path.dirname(data_file)
    
        prod = data[data['mode']=='produce']
        bench_plot(
            prod[prod['branch']=='trunk']['events'].to_numpy(),
            prod[prod['branch']=='trunk']['time'].to_numpy(),
            prod[prod['branch']=='trunk']['size'].to_numpy(),
            prod[prod['branch']==name]['time'].to_numpy(),
            prod[prod['branch']==name]['size'].to_numpy(),
            name, 'Production')
        plt.savefig(f'{dirname}/production_{filename}')
        plt.clf()

        reco = data[data['mode']=='recon']
        bench_plot(
            reco[reco['branch']=='trunk']['events'].to_numpy(),
            reco[reco['branch']=='trunk']['time'].to_numpy(),
            reco[reco['branch']=='trunk']['size'].to_numpy(),
            reco[reco['branch']==name]['time'].to_numpy(),
            reco[reco['branch']==name]['size'].to_numpy(),
            name, 'Reconstruction')
        plt.savefig(f'{dirname}/recon_{filename}')
        plt.clf()

    sys.exit(0)

if __name__ == '__main__' :
    main()
