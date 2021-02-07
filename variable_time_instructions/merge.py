# Takes filtered csv files and merges them into a list of
# potentially variable-time instructions
# NOTE THAT THE OUTPUT FILE WILL CONTAIN WRONG VALUES

import sys

def split_names(line):
	# line as [names, ops]
	# remove commas in names
	line[0] = line[0].replace(",", "")
	# split names
	names = line[0].split(' ')
	# return (name, op) for each name
	return [(n, line[1]) for n in names]

instr = set()

for name in sys.argv[1:]:
	infile = open(name, "r")
	lines = infile.read().splitlines()
	infile.close()

	# list of [names, ops]
	instr_names_ops = [l.split(';')[0:2] for l in lines]
	# split lines in case multiple names are specified
	instr_names_ops = [split_names(l) for l in instr_names_ops]
	# update set with the new tuples
	instr.update(*instr_names_ops)

outfile = open("merged.txt", "w")
outfile.write('\n'.join(["{};{}".format(i[0], i[1]) for i in instr]))
outfile.close()
