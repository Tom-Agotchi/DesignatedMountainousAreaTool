
import sys;
sys.path.append( "../../modules" );
import math;
import time;
import os;
import struct;
import re;
import json;

import numpy;

import zipfile;

"""
import scipy;
import scipy.optimize;
import scipy.stats;

import matplotlib;
matplotlib.use('Qt5Agg');#WebAgg')	#CHANGE BACKEND to avoid Tk GTK GTKCairo MacOSX Qt4Agg Qt5Agg WX WXAgg GTK3Cairo GTK3Agg WebAgg cairo gdk pdf pgf ps svg template
import matplotlib.pyplot as matplotlib_pyplot;
import matplotlib.colors as matplotlib_colors;
import matplotlib.widgets as matplotlib_widgets;
from matplotlib.animation import FuncAnimation
"""

#------------------------------------------------------------------------------

sys.path.append( "./" );

#------------------------------------------------------------------------------

def remove_file_extension( path ):
	try:
		lastdot = path.rindex('.')
		return path[:lastdot];
	except:
		pass;
	return lastdot;

def run():

	nedzprefix = 'float'
	nedzpostfix = '_1.flt'
	nedpath = "Y:/NED/NGTOC_NED_Ops/Output/distribution/1/GridFloat/"
	nedfiles = os.listdir( nedpath );
	
	nedlinesize = 3600;			#latitude segements / #rows
	nedlinebeginpadding = 6;
	nedlineendpadding = 6;
	
	nedrowsize = 3600;		#values / lon segments / #columns
	nedrowbeginpadding = 6;	
	nedrowendpadding = 6;	
	nedrowfullsize = nedrowbeginpadding + nedrowsize + nedrowendpadding;
	nedelementbytesize = 4;
	
	nedblocksizelat = 60;
	nedblocksizelon = 60;
	
	#Test case:
	nedfiles = [ "n42w080.zip" ];
	
	#we only read the data one LINE at a time (since it's stored this way)
	linebuffer = bytearray( nedrowfullsize * nedelementbytesize );
	
	reggy_letter = re.compile("[a-zA-Z]");
	reggy_number = re.compile("[0-9]");
	for filename in nedfiles:
		filenameremext = remove_file_extension( filename );
		findfilename = nedzprefix + filenameremext + nedzpostfix;
		
		#
		#n = +	(if +, subtract 1... because there is no n00 apparently, just n01?
		#w = -
	
		lattile = 0;#42 - 1;	#NED is stored wrong for positive latitude tiles... so watch out (maximum as index)
		lontile = 0;#-80;	#It's stored right for longitude (minimum first as index)
		
		splits = reggy_letter.split( filenameremext )
		splitsnum = reggy_number.split( filenameremext )
		
		#print( splits );
		#print( splitsnum );
		
		if( len( splits ) > 2 and len( splitsnum ) > 2 ):
		
			lattile = int( splits[1] );#NED is stored wrong for positive latitude tiles... so watch out (maximum as index)
			lontile = int( splits[2] );#-80;	#It's stored right for longitude (minimum first as index)
			
			lathemi = splitsnum[0].lower();
			lonhemi = splitsnum[2].lower();
			
			if lathemi == 'n':	#n01 exists but is from n00..n01, s01 is from s01..s00/n00
				lattile -= 1;
			else:
				lattile = -lattile;
			if lonhemi == 'e':	#e01 exists but is from e00..e01, w01 is from w01..w00/e00
				lontile -= 1;
			else:
				lontile = -lontile;
			
		else:
			print( [ "Invalid filename "+filenameremext+" syntax: ", splits ] )
			continue;
			
		with zipfile.ZipFile(nedpath + filename, 'r') as arch:
			
			#Find the file we want to read:
			filesinside = arch.namelist();
			usefile = None;
			for finname in filesinside:
				if finname == findfilename:
					usefile = finname;
					
			#must exist
			if usefile:
			
				savedata = {};	#JSON data to save;
							
				nblocks = math.floor( nedrowsize/nedblocksizelon )
				
				n = nedrowfullsize;
				spackstring = 'f'*n;
				
				#We are using the undocumented 'readinto' to read blocks of bytes
				with arch.open( usefile ) as subfile:
				
					#print( dir( subfile ) )	#subfile is a 'ZipExtFile' object but has SOME stream methods
					
					latintegerindex = lattile*nedlinesize - nedlinebeginpadding;
					
					prevtilelat60 = -9999;	#latintegerindex / nedblocksizelat;
					
					lastlatkey = "";
					
					rowindex = -nedlinebeginpadding
					maxlines = nedlinesize + nedlinebeginpadding;
					bytesread = 1;
					while maxlines > 0:
						
						#read zipped file ONE ROW at a time (into a preallocated buffer!)
						bytesread = subfile.readinto( linebuffer );
						
						if rowindex > 0:
						
							#Update latitude tile queue if needed
							currtilelat60 = math.floor(latintegerindex/nedblocksizelat)
							if currtilelat60 != prevtilelat60:
								prevtilelat60 = currtilelat60;	#latintegerindex / nedblocksizelat;
								
								#Create new tile list
								latkey = int( currtilelat60 );
								
								lastlatkey = latkey;
								
								savedata[ latkey ] = {};
								
								lastlonkey = math.floor( lontile*nedblocksizelon)#nedrowsize)/nedblocksizelon );
								
								nblock = 0;
								while nblock < nblocks:
								
									savedata[ latkey ][ lastlonkey ] = [0,0,0,0];
								
									nblock += 1;
									lastlonkey += 1;
								
							#process ned line
							floatrow = struct.unpack( spackstring, linebuffer );
							
							#lonintegerindex = lontile*nedrowsize - nedrowbeginpadding;
							#latintegerindex = lat*3600
							#lonintegerindex = lon*3600
							
							lonkeyindex = lontile*nedblocksizelon;
							lonblockstart = nedrowbeginpadding;
							nblock = 0;
							while nblock < nblocks:
							
								currstats = savedata[ latkey ][ lonkeyindex ];
								
								x = 0;
								while x < nedblocksizelon:
								
									v = floatrow[ lonblockstart ];
									
									if v > -1000 and v < 10000:
									
										if currstats[3] <= 0:
											currstats[0] = v;
											currstats[1] = v;
											currstats[2] = v;
											currstats[3] = 1;
										else:
											currstats[0] = min( currstats[0], v );
											currstats[1] = max( currstats[0], v );
											currstats[2] = ( currstats[2] * (currstats[3]/(currstats[3]+1)) ) + v/(currstats[3]+1);
											currstats[3] += 1;
											
									else:
										pass	#invalid
										
									x += 1;
									lonblockstart += 1;
									
								savedata[ latkey ][ lonkeyindex ] = currstats;
									
								#lonblockstart += nedblocksizelon;
								lonkeyindex += 1;
								nblock += 1;
							
						rowindex += 1;
						latintegerindex += 1;
						maxlines -= 1;
						
				outname = 'out/nedpfx60_' +str(int(lontile)) + '_'+str(int(lattile))+'.json';
				jsonout = json.dumps( savedata );
				fout = open( outname, 'wb' );
				fout.write( jsonout.encode('ascii') );
				fout.close();
				
			else:
				print( "#ERROR file "+filename+" does NOT contain float data " )
				
		#"get row" of floats:
		#n = nedrowfullsize
		#floatrow = struct.unpack('f'*n, file.read(4*n))
		
	
	#"n42w080.zip"	=> -80..-79 lon, 41..42 lat ??? jesus
	#Bounding_Coordinates:
	#West_Bounding_Coordinate: -80.00166666667 = -80 - 6*(1/3600)
	#East_Bounding_Coordinate: -78.99833333334 = -79 + 6*(1/3600)
	#North_Bounding_Coordinate: 42.00166666666 = 42 + 6*(1/3600)
	#South_Bounding_Coordinate: 40.99833333333 = 41 - 6*(1/3600)
	#File inside zip file:
	#floatn42w080_1.flt = 'float' + remove_file_extension( 'n42w080.zip' ) + '_1.flt'
	#Geographic:
	#Latitude_Resolution: 0.00001
	#Longitude_Resolution: 0.00001
	#Geographic_Coordinate_Units: Decimal degrees
	#Geodetic_Model:
	#Horizontal_Datum_Name: North American Datum of 1983
	#Ellipsoid_Name: Geodetic Reference System 80
	#Semi-major_Axis: 6378137.000000
	#Denominator_of_Flattening_Ratio: 298.2572221
	#Vertical_Coordinate_System_Definition:
	#Altitude_System_Definition:
	#Altitude_Datum_Name: North American Vertical Datum of 1988
	#Altitude_Resolution: 0.001
	#Altitude_Distance_Units: meters
	#Altitude_Encoding_Method: Implicit coordinate
	#Raster_Object_Type: Pixel
	#Row_Count: 3612
	#Column_Count: 3612
	#Vertical_Count: 1

	#print( nedfiles );

#------------------------------------------------------------------------------
if __name__	== "__main__":
	sys.exit(run())
	
