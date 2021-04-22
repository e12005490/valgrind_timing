# Filter potentially variable-time isntructions from a raw csv file

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

# Filter definitions

def is_float(s):
	"""returns whether the given string is a float"""
	try:
		f = float(s)
		return True
	except ValueError:
		return False

def non_constant_filter(line):
	"""accepts the line if the latency exists and is not a clear constant"""
	return line[2] and not is_float(line[2])

def parse_variation(l):
	"""expects a string of the form <float>-<float>, and returns the floats"""
	s = l.split('-')
	if len(s) != 2:
		return None
	try:
		return (float(s[0]), float(s[1]))
	except ValueError:
		return None

def clear_variation_filter(line):
	"""accepts the line if the latency clearly varies (e.g. rejects '~20')"""
	return parse_variation(line[2]) is not None

def strong_variation_filter(line):
	"""accepts the line if the latency clearly varies by more than one cycle"""
	t = parse_variation(line[2])
	if t is not None:
		return (t[1] - t[0]) > 1
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

# Filter loop

for name in sys.argv[2:]:
	lines = read_lines(name)

	# parse lines
	instr = [parse(l) for l in lines]
	# filter out instructions without given latencies or with constant
	# latencies
	instr = [i for i in instr if filter(i)]

	outfile = open("{}/../filtered/{}-{}".format(Path(__file__).parent.absolute(), sys.argv[1], os.path.basename(name)), "w")
	outfile.write('\n'.join(i[3] for i in instr))
	outfile.write('\n')
	outfile.close()
