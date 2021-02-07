# Takes filtered csv files and merges them into a list of
# potentially variable-time instructions
# NOTE THAT THE OUTPUT FILE WILL CONTAIN WRONG VALUES

from helpers import read_lines, split_names
from pathlib import Path
import sys

instr = set()

for name in sys.argv[1:]:
	lines = read_lines(name)

	# list of [names, ops]
	instr_names_ops = [l.split(';')[0:2] for l in lines]
	# split lines in case multiple names are specified
	instr_names_ops = [split_names(l) for l in instr_names_ops]
	# update set with the new tuples
	instr.update(*instr_names_ops)

outfile = open("{}/../merged.txt".format(Path(__file__).parent.absolute()), "w")
outfile.write('\n'.join(sorted([i[0] + ";" + i[1] for i in instr])))
outfile.close()
