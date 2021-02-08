# Takes filtered csv files and lists instructions with the
# number of microarchitectures executing them in variable time
# NOTE THAT THE OUTPUT FILE WILL CONTAIN WRONG VALUES

from helpers import read_lines, split_names
import itertools
from pathlib import Path
import sys

instr = list()

for name in sys.argv[1:]:
	lines = read_lines(name)

	# list of [names, ops]
	instr_names_ops = [l.split(';')[0:2] for l in lines]
	# split lines in case multiple names are specified
	instr_names_ops = [split_names(l) for l in instr_names_ops]
	# update list of all names
	for l in instr_names_ops:
		instr = [*instr, *l]

# now group instructions and occurrences
occurrences = {}
for i in instr:
	if i in occurrences:
		occurrences[i] += 1
	else:
		occurrences[i] = 1

# list of ((name, operands), count)
items = list(occurrences.items())

outfile = open("{}/../count.txt".format(Path(__file__).parent.absolute()), "w")
outfile.write('\n'.join(sorted(["{:02d};{};{}".format(i[1], i[0][0], i[0][1]) for i in items])))
outfile.write('\n')
outfile.close()
