#!/usr/bin/env python3

if __name__ == "__main__":
	import sys
	if len( sys.argv ) < 2:
		print( "Usage: %s file1 [file2 [file3 [...] ] ]" % sys.argv[0] )
		sys.exit( 1 )
	
	from resultparser import parseFile
	
	for f in sys.argv[1:]:
		results = parseFile( f )
		print( results["sort"][0] )

