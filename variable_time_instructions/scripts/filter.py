# Filter potentially variable-time isntructions from a raw csv file
# In the dataset, constant-time instuctions have a number as latency,
# so it is easy to filter them out

from helpers import read_lines
import os
from pathlib import Path
import sys

def parse(line):
	"""given a line, returns [name, operands, latency, line]"""
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
	lines = read_lines(name)

	# parse lines
	instr = [parse(l) for l in lines]
	# filter out instructions without given latencies or with constant
	# latencies
	instr = [i for i in instr if i[2] and not is_int(i[2])]

	outfile = open("{}/../filtered/filtered-{}".format(Path(__file__).parent.absolute(), os.path.basename(name)), "w")
	outfile.write('\n'.join(i[3] for i in instr))
	outfile.close()
