#!/usr/bin/env python3

def parseFile( path ):
	import re
	
	results = {}
	currentPhase = None
	
	with open( path ) as f:   # f: file
		for l in f:           # l: line
			m = re.match( '^Phase "([^"]*)":$', l )
			if m:
				currentPhase = m.group(1)
				results[ currentPhase ] = []
			
			m = re.match( '   node +([^:]*): ([^ ]*) microsecs :: ([^ ]*) millisecs :: ([^ ]*) secs$', l )
			if m:
				assert( currentPhase )
				results[ currentPhase ].append( int( m.group(2) ) )
	
	return results

if __name__ == "__main__":
	import sys
	if len( sys.argv ) < 2:
		print( "Usage: %s file" % sys.argv[0] )
		sys.exit( 1 )
	
	results = parseFile( sys.argv[1] )
	
	for phase in results.keys():
		print( phase )
		for (rank,time) in enumerate( results[phase] ):
			print( "  rank %d: %d usecs" % (rank,time) )

