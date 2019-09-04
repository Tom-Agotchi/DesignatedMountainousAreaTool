
import sys;
sys.path.append( "../../modules" );
import math;
import time;

import numpy;
import scipy;
import scipy.optimize;
import scipy.stats;

import matplotlib;
matplotlib.use('Qt5Agg');#WebAgg')	#CHANGE BACKEND to avoid Tk GTK GTKCairo MacOSX Qt4Agg Qt5Agg WX WXAgg GTK3Cairo GTK3Agg WebAgg cairo gdk pdf pgf ps svg template
import matplotlib.pyplot as matplotlib_pyplot;
import matplotlib.colors as matplotlib_colors;
import matplotlib.widgets as matplotlib_widgets;
from matplotlib.animation import FuncAnimation

#import ujson	#GODS has to be compiled.
import json;

#------------------------------------------------------------------------------

sys.path.append( "./" );

from eletools import ElevationGetter

def create_json_patch( eg, long, lat, fileprefix='jsonel' ):

	#convert a SINGLE PATCH into =>
	#	1/60th lat/long => [min, max, avg, stdev] elevation values ?

	#lldata = {};	#Referenced by 1/60th of a degree: ( (-104)*60 + (0..60) ) == xi	( (+39)*60 + (0..60) ) == yi
	#21600..10800 grid == 233280000 elements... (If it were completely solid! it's not...)
	
	lxyd = {};	#[ int( y*60 ) ][ int( x*60 ) ] = [ min, max, avg, stdev, count, 0, 0, 0 ]
	
	file_long = int( long );	#-104;	#w104
	file_lat = int( lat );	#39;	#n39
	file_prefix = "Y:\\Incoming\\";
	
	fpath = "";
	
	if file_long <= 0:
		fpath += "w";
		nv = str( int( abs(file_long) ) );
		while( len( nv ) < 3 ):
			nv = "0" + nv;
		fpath += nv;
	else:
		fpath += "e";
		nv = str( int( file_long ) );
		while( len( nv ) < 3 ):
			nv = "0" + nv;
		fpath += nv;
		
	fpath += "\\"
	
	if file_lat <= 0:
		fpath += "s";
		nv = str( int( abs(file_lat) ) );
		while( len( nv ) < 2 ):
			nv = "0" + nv;
		fpath += nv;
	else:
		fpath += "n";
		nv = str( int( file_lat ) );
		while( len( nv ) < 2 ):
			nv = "0" + nv;
		fpath += nv;
	
	fpath += ".dt2"
	
	#print( file_prefix + fpath );
	
	dted_level = 2;	#TRY USING 2 ALWAYS, first.	"DTED2\\"
	
	file_stream = open( file_prefix + "DTED2\\" + fpath, 'rb' );	#Denver, should be useful

	
	#Level 2 has a post spacing of approximately 30 meters.
	#zone	latitude range	level 0 (arc secs)	level 1 (arc secs)	level 2 (arc secs)
	#latitude spacing	all							30	3	1
	#
	#longitude spacing	I	0°–50° (North–South)	30	3	1
	#II	50°–70° (North–South)						60	6	2
	#III	70°–75° (North–South)					90	9	3
	#IV	75°–80° (North–South)						120	12	4
	#V	80°–90° (North–South)						180	18	6
	
	
	#WARNING: number of SAMPLES and PATCHES changes PER FILE (based on latitude, and resolution)
	xsamples = 60;
	ysamples = 60;
	xpatches = 60;
	ypatches = 60;
	#Problem: "patches" break down at DTED levels that are not 2.
	#	why? Because each file is a grid of post values.
	#	As a result, the posts do NOT always line up on a regular grid.
	#
	
	if dted_level >= 2:
		if abs( file_lat ) < 50:
			xsamples = 60;
		elif abs( file_lat ) < 70:
			xsamples = int(60/2);
		elif abs( file_lat ) < 75:
			xsamples = int(60/3);
		elif abs( file_lat ) < 80:
			xsamples = int(60/4);
		elif abs( file_lat ) < 90:
			xsamples = int(60/6);
	elif dted_level >= 1:
		"""
		xsamples = 60/3;
		ysamples = 60/3;
		if abs( file_lat ) < 50:
			xsamples = 60/3;
		elif abs( file_lat ) < 70:
			xsamples = 60/6;
		elif abs( file_lat ) < 75:
			xsamples = 60/9;
		elif abs( file_lat ) < 80:
			xsamples = 60/12;
		elif abs( file_lat ) < 90:
			xsamples = 60/18;
		"""
		pass;

	#countdata_ysize = xsamples*xpatches;
	#countdata = numpy.zeros( countdata_ysize * ysamples*ypatches, dtype=numpy.int32 );
	
	bench_starttime = time.clock();
	
	y_patch = 0;
	while y_patch < ypatches:
		x_patch = 0;
		while x_patch < xpatches:
		
			x_hc = int( file_long*60 ) + x_patch;
			y_hc = int( file_lat*60 ) + y_patch;
			if y_hc in lxyd:
				pass;
			else:
				lxyd[ y_hc ] = {};
				
			wapow = eg._dted_load_dt_patch( file_stream, x_patch, y_patch, nxsamples=xsamples, nysamples=ysamples, nxblocks=xpatches, nyblocks=ypatches );
			
			"""
			cstart = xsamples*x_patch + y_patch*( xsamples*xpatches );
			
			yi = 0;
			while yi < ysamples:
			
				xi = 0;
				while xi < xsamples:
			
					countdata[ (x_patch*xsamples + xi) + (y_patch*ysamples + yi)*countdata_ysize ] = wapow[ xi + yi*xsamples ];
					#countdata[ cstart + xi + yi*countdata_ysize ] = wapow[ xi + yi*xsamples ];
					
					xi += 1;
				yi += 1;
			"""
			
			#self.patch_buffer[ ix + iy*nxsamplesp1 ] = magnit;
				
			v_min = 0;
			v_max = 0;
			v_avg = 0;
			v_count = 0;
			#12 values for POLYNOMIAL fit to height histogram? lol... hm... SKA with 12 values??? hm.
			#	HOG type algorithm; taking a angular gradient thing
			#	well, if we have the height/count histogram per tile;
			#	Can we construct a representation with a few values??? ... since we have min/max range, well.
			#	We can probably inject quartiles? slopes? hm.
			
			ity = 0;
			while ity < ysamples:
			
				irowi = ity*(xsamples+1);
				
				for vraw in wapow[ (irowi):(irowi+xsamples) ]:	#80.4 seconds (saves some time, but only a percent)
			
				#itx = 0;
				#while itx < xsamples:	#83.9 seconds
				
					v = 1.0*vraw;	#wapow[ itx + irowi ];
				
					#magnit = (((int(bytes[bi])) & 127) << 8) | ((int(bytes[bi+1])) & 255);
					#if( (bytes[bi]&128) != 0 ):
					#	magnit = -magnit;
					if v < -16384:	#If VALID
						#Value is a HOLE, and is set to a FFFF value ( which is -32768 )
						#Values of 0 also occur (even if those make NO SENSE); are they errors?
						pass;
					else:
				
						if v_count != 0:	#If NOT FIRST VALUE
							v_min = min( v_min, v );
							v_max = max( v_max, v );
							#v_avg = ( v_avg * v_count + (v)/(v_count+1) )/(v_count+1);
							v_avg = ( v_avg * (v_count/(v_count+1)) ) + v/(v_count+1)
							#New average = old average * (n-1)/n + new value /n
							#v_avg = ( v_avg * v_count )/(v_count+1) + v/(v_count+1)
							#v_avg = ( v_avg * v_count + v )/(v_count+1)
							#v_avg = ( v_avg * v_count/(v_count+1)) + v/(v_count+1)
							#v_avg = v_avg * (v_count/(v_count+1)) + v/(v_count+1)
							#v_avg = v_avg * (v_count-1)/v_count + v/v_count
							#v_avg = ( v_avg * (v_count-1) + v )/v_count
							#(v_avg*v_count + v)/(v_count+1)
							#(v_avg*v_count + v)/(v_count+1)
							
						else:	#The FIRST VALUE THAT IS VALID
							v_min = v;
							v_max = v;
							v_avg = v;
						
						v_count += 1;
				
					#itx += 1;
				ity += 1;
					
			lxyd[ y_hc ][ x_hc ] = [ v_min, v_max, v_avg, v_count ];
			
			#print( "Patch " + str( y_hc ) + " " + str( x_hc ) + " = " + str( [ v_min, v_max, v_avg, v_count ] ) )
			
			x_patch += 1;
		y_patch += 1;
	
	bench_stoptime = time.clock();
	
	print( "took: " + str(bench_stoptime - bench_starttime) + " seconds" )
	#numpy.save( 'patch_test.bin', countdata );
	#there are 1108 files:
	#	Meaning, 1108 * time per tile
	foutpath = fileprefix + str(file_long)+'_'+str(file_lat)+'.json'
	
	fout = open( foutpath, 'wb' );
	fout.write( json.dumps( lxyd ).encode('utf-8') );
	fout.close();
	
	#Save 'lxyd' to a pickle file? hm... ujson instead? (interesting for pure data)
	
	file_stream.close();
	
	return lxyd;
	

#------------------------------------------------------------------------------
def run():

	eg = ElevationGetter();
	eg.set_path( 'Y:\\Incoming\\DTED1' );
	
	ylat_min = 50;
	ylat_max = 70;
	xlong_min = -180;
	xlong_max = 180;
	
	ylat = ylat_min;
	while ylat < ylat_max:
		
		xlong = xlong_min;
		while xlong < xlong_max:
			#create_json_patch( eg, xlong, ylat, fileprefix='jout/pavgs60_' );
			try:
				create_json_patch( eg, xlong, ylat, fileprefix='jout/pavgs60_' );
				print( "Patch: " + str( xlong) +" "+str( ylat ) );
			except:
				print( "Patch failed: " + str( xlong) +" "+str( ylat ) );
			
			xlong += 1;

		ylat += 1;
	
	#xlong = -104;
	#ylat = 39;
	#try:
	#create_json_patch( eg, xlong, ylat, fileprefix='jout/pavgs90_' );
	#except:
	#	print( "Patch failed: " + str( xlong) +" "+str( ylat ) );
	#	pass;
	

#------------------------------------------------------------------------------
if __name__	== "__main__":
	sys.exit(run())