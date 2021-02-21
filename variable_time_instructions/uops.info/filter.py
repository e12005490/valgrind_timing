import sys
import re

regex = re.compile("\\d+")

for name in sys.argv[1:]:
	input = open(name, "r")
	lines = input.read().splitlines()
	input.close()

	output = open("filtered-" + name, "w")
	for l in lines:
		if not regex.fullmatch(l.split('/')[1]):
			output.write(l + "\n")
	output.close()
