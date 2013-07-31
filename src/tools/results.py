#!/usr/bin/env python3

if __name__ == "__main__":
	import sys
	if len( sys.argv ) < 3:
		print( "Usage: %s phase file1 [file2 [file3 [...] ] ]" % sys.argv[0] )
		sys.exit( 1 )

	from resultparser import parseFile

	for f in sys.argv[2:]:
		results = parseFile( f )
		for i in range(len(results[sys.argv[1]])):
			print( results[sys.argv[1]][i] )

