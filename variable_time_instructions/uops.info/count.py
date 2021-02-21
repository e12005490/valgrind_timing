import sys

occurrences = {}
for name in sys.argv[1:]:
	input = open(name, "r")
	lines = list(map(lambda l: l.split('/')[0], input.read().splitlines()))
	input.close()

	for l in lines:
		if l in occurrences:
			occurrences[l] += 1
		else:
			occurrences[l] = 1

items = list(occurrences.items())
print('\n'.join(sorted(["{:02d};{}".format(i[1], i[0]) for i in items])))
