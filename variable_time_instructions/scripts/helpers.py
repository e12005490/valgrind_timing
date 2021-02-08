def read_lines(filename):
	"""returns the list of lines of a file"""
	file = open(filename, "r")
	lines = file.read().splitlines()
	file.close()
	return lines
