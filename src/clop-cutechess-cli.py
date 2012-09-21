#!/usr/bin/python
# -*- coding: utf-8 -*-

"""
Usage: clop-cutechess-cli.py CPU_ID SEED [PARAM_NAME PARAM_VALUE]...
Run cutechess-cli with CLOP_PARAM(s).

  CPU_ID	Symbolic name of the CPU or machine that should run the game
  SEED		Running number for the game to be played
  PARAM_NAME	Name of a parameter that's being optimized
  PARAM_VALUE	Integer value for parameter PARAM_NAME

CLOP is a black-box parameter tuning tool designed and written by Rémi Coulom.
More information about CLOP can be found at the CLOP website:
http://remi.coulom.free.fr/CLOP/
 
This script works between CLOP and cutechess-cli. The path to this script,
without any parameters, should be on the "Script" line of the .clop file.
'Replications' in the .clop file should be set to 2 so that this script can
alternate the engine's playing side correctly.

In this script the variables 'cutechess_cli_path', 'engine', 'engine_param_cmd',
'opponents' and 'options' must be modified to fit the test environment and
conditions. The default values are just examples.

When the game is completed the script writes the game outcome to its
standard output:
  W = win
  L = loss
  D = draw
"""

from subprocess import Popen, PIPE
import sys


# Path to the cutechess-cli executable.
# On Windows this should point to cutechess-cli.exe
cutechess_cli_path = 'path_to_cutechess-cli/cutechess-cli.sh'

# The engine whose parameters will be optimized
engine = 'conf=MyEngine'

# Format for the commands that are sent to the engine to
# set the parameter values. When the command is sent,
# {name} will be replaced with the parameter name and {value}
# with the parameter value.
engine_param_cmd = 'setvalue {name} {value}'

# A pool of opponents for the engine. The opponent will be
# chosen based on the seed sent by CLOP.
opponents = [
    'conf=OpponentEngine1',
    'conf=OpponentEngine2',
    'conf=OpponentEngine3'
]

# Additional cutechess-cli options, eg. time control and opening book
options = '-each tc=40/1+0.05 -draw 80 1 -resign 5 500'


def main(argv = None):
    if argv is None:
        argv = sys.argv[1:]
    
    if len(argv) == 0 or argv[0] == '--help':
        sys.stdout.write(__doc__)
        return 0

    argv = argv[1:]
    if len(argv) < 3 or len(argv) % 2 == 0:
        sys.stderr.write('Too few arguments\n')
        return 2
    
    clop_seed = 0
    try:
        clop_seed = int(argv[0])
    except ValueError:
        sys.stderr.write('invalid seed value: %s\n' % argv[0])
        return 2

    fcp = engine
    scp = opponents[(clop_seed >> 1) % len(opponents)]
    
    # Parse the parameters that should be optimized
    for i in range(1, len(argv), 2):
        # Make sure the parameter value is numeric
        try:
            float(argv[i + 1])
        except ValueError:
            sys.stderr.write('invalid value for parameter %s: %s\n' % (argv[i], argv[i + 1]))
            return 2
        # Pass CLOP's parameters to the engine by using
        # cutechess-cli's initialization string feature
        initstr = engine_param_cmd.format(name = argv[i], value = argv[i + 1])
        fcp += ' initstr="%s"' % initstr
    
    # Choose the engine's playing side (color) based on CLOP's seed
    if clop_seed % 2 != 0:
        fcp, scp = scp, fcp
    
    cutechess_args = '-engine %s -engine %s %s' % (fcp, scp, options)
    command = '%s %s' % (cutechess_cli_path, cutechess_args)
    
    # Run cutechess-cli and wait for it to finish
    process = Popen(command, shell = True, stdout = PIPE)
    output = process.communicate()[0]
    if process.returncode != 0:
        sys.stderr.write('failed to execute command: %s\n' % command)
        return 2
    
    # Convert Cutechess-cli's result into W/L/D
    # Note that only one game should be played
    result = -1
    for line in output.decode("utf-8").splitlines():
        if line.startswith('Finished game'):
            if line.find(": 1-0") != -1:
                result = clop_seed % 2
            elif line.find(": 0-1") != -1:
                result = (clop_seed % 2) ^ 1
            elif line.find(": 1/2-1/2") != -1:
                result = 2
            else:
                sys.stderr.write('the game did not terminate properly\n')
                return 2
            break
    
    if result == 0:
        sys.stdout.write('W\n')
    elif result == 1:
        sys.stdout.write('L\n')
    elif result == 2:
        sys.stdout.write('D\n')

if __name__ == "__main__":
    sys.exit(main())
