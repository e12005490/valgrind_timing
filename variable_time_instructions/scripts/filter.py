# Filter potentially variable-time isntructions from a raw csv file
# In the dataset, constant-time instuctions have a number as latency,
# so it is easy to filter them out

from helpers import read_lines
import os
from pathlib import Path
import re
import sys

def parse(line):
	"""given a line, returns [name, operands, latency, line]"""
	s = line.split(';')
	s.append(line)
	return s

#
# Filter definitions
#

def is_int(s):
	try:
		i = int(s)
		return True
	except ValueError:
		return False

def non_constant_filter(line):
	"""accepts the line if the latency exists and is not a clear constant"""
	return line[2] and not is_int(line[2])

regex = re.compile("\d+-\d+")
def clear_variation_filter(line):
	"""accepts the line if the latency clearly varies (e.g. rejects '~20')"""
	return regex.fullmatch(line[2])

def strong_variation_filter(line):
	"""accepts the line if the latency clearly varies by more than one cycle"""
	if clear_variation_filter(line):
		s = line[2].split('-')
		return (int(s[1]) - int(s[0])) > 1
	return False

filter = None
if sys.argv[1] == "simple":
	filter = non_constant_filter
elif sys.argv[1] == "clear":
	filter = clear_variation_filter
elif sys.argv[1] == "strong":
	filter = strong_variation_filter
else:
	print("Unknown filter type: " + sys.argv[1])
	exit(1)

#
# Filter loop
#

for name in sys.argv[2:]:
	lines = read_lines(name)

	# parse lines
	instr = [parse(l) for l in lines]
	# filter out instructions without given latencies or with constant
	# latencies
	instr = [i for i in instr if filter(i)] #regex.match(i[2])]

	outfile = open("{}/../filtered/{}-{}".format(Path(__file__).parent.absolute(), sys.argv[1], os.path.basename(name)), "w")
	outfile.write('\n'.join(i[3] for i in instr))
	outfile.write('\n')
	outfile.close()
