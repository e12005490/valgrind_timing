def read_lines(filename):
	"""returns the list of lines of a file"""
	file = open(filename, "r")
	lines = file.read().splitlines()
	file.close()
	return lines


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
