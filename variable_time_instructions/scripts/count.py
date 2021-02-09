# Takes filtered csv files and lists instructions with the
# number of microarchitectures executing them in variable time

from helpers import read_lines
import itertools
import sys

def split_names(line):
	"""
	given a line, splits it into the possibly multiple instructions
	it describes
	"""
	# different format
	if line[0].startswith("REP"):
		return [tuple(line)]
	# remove commas in names
	line[0] = line[0].replace(",", "")
	# split names
	names = line[0].split(' ')
	# return (name, op) for each name
	return [(n, line[1]) for n in names if n]


def split_operands(line):
	"""
	given a (name, ops), splits it into the possible operand
	combinations it describes (e.g. with 'r/m,r/m' have 'r,r', 'r,m', ...)
	"""
	name = line[0]
	# separate ops
	ops = line[1].split(',')
	# for each operand, split its possible types
	ops = [o.split('/') for o in ops]
	# list of type combinations for the instruction
	prod = itertools.product(*ops)
	# generate simple instructions
	return [(name, ','.join(o)) for o in prod]


instr = list()

for name in sys.argv[1:]:
	lines = read_lines(name)

	# list of [names, ops]
	instr_names_ops = [l.split(';')[0:2] for l in lines]
	# split lines in case multiple names are specified
	instr_names_ops = [i for l in instr_names_ops for i in split_names(l)]
	# split operands in case multiple types are possible
	instr_names_ops = [i for t in instr_names_ops for i in split_operands(t)]
	# update list of all names
	instr += instr_names_ops

# now group instructions and occurrences
occurrences = {}
for i in instr:
	if i in occurrences:
		occurrences[i] += 1
	else:
		occurrences[i] = 1

# list of ((name, operands), count)
items = list(occurrences.items())

print('\n'.join(sorted(["{:02d};{};{}".format(i[1], i[0][0], i[0][1]) for i in items])))
