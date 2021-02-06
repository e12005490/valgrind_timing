# Filter potentially variable-time isntructions from a raw csv file
# In the dataset, constant-time instuctions have a number as latency,
# so it is easy to filter them out

import sys

def parse(line):
	s = line.split(';')
	s.append(line)
	return s

def is_int(s):
	try:
		i = int(s)
		return True
	except ValueError:
		return False

for name in sys.argv[1:]:
	infile = open(name, "r")
	lines = infile.read().splitlines()
	infile.close()

	instr = [parse(l) for l in lines]
	instr = [i for i in instr if i[2] and not is_int(i[2])]

	outfile = open("filtered/filtered-{}".format(name), "w")
	outfile.write('\n'.join(i[3] for i in instr))
	outfile.close()
