# Takes filtered csv files and merges them into a list of
# potentially variable-time instructions
# NOTE THAT THE OUTPUT FILE WILL CONTAIN WRONG VALUES

import sys

instr = set()

for name in sys.argv[1:]:
	infile = open(name, "r")
	lines = infile.read().splitlines()
	infile.close()

	instr_names = [l.split(';')[0] for l in lines]
	#some lines have multiple instructions separated by spaces
	instr.update(*[[s.replace(",", "") for s in i.split(' ')] for i in instr_names])

outfile = open("merged.txt", "w")
outfile.write('\n'.join(instr))
outfile.close()
