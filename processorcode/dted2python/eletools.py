
import sys;
sys.path.append( "./" );

import numpy;

class ElevationGetter:
	def __init__(self):
		self.path = './';
		self.grid = numpy.zeros( (100,100), dtype='float' )
		self.grid_buffer = numpy.zeros( (100,100), dtype='float' )
		self.grid_buffer_done = True;
		
		self.query_xlong = -104.7146;	#Devils tower (WILL NOT be visible in dted...)
		self.query_ylat = 44.509;
		
		#self.patch_cache = {};	#Interesting... we just dump the file into memory (or a few of them! can handle about 12+ ?)
		self.block_buffer = numpy.zeros( 120*120*2, dtype='uint8' );
		self.patch_buffer = numpy.zeros( 120*120*2, dtype='int16' );
		
	def set_path( self, p ):
		"""
			Set the filepath to the DTED or other elevation source?
		"""
		self.path = p;
		
	def get_dted_grid_at( self, xlong, ylat ):
		"""
			Return the array of elevation data at this point
		"""
		if self.grid_ready():
		
			self.query_xlong = xlong;	#Devils tower (WILL NOT be visible in dted...)
			self.query_ylat = ylat;
			
			self.grid_clear();
			
			#Perform async query to system
			
			
			return True;
			
		return False;
		
	def grid_ready( self ):
		return self.grid_buffer_done;
	
	def grid_get( self ):
		return self.grid;
	
	def grid_clear( self ):
		self.grid_buffer_done = False;
		
	#
	#DTED type files:
	#
	#Each file has a 3428 byte header:
	#	UHL[80]
	#	DSI[648]
	#	ACR[2700]
	#
	#	DTED1
	#		lat >= 50:
	#			n = ? 10x30 in spec
	#			s = ?? 7214
	#		lat < 50:
	#			n = 20x20 patches	(20x60 in spec
	#			s =	2414	(2414 bytes in spec, 1201 records in spec)
	#	DTED2
	#		lat >= 50:
	#			n = 30*30 patches? ( skip to first 5400 bytes, total record size is still 7214??? )
	#			s = 7242	??
	#		lat < 50:
	#			n = 60*60	(60*60 to spec)
	#			s = 7214	(3601 records)
	#		
	#Data in file is a set of patches:
	#	[ (8) header ]
	#	[ n*2 MSB LSB signed shorts (1's compliment!!) ]
	#	[ (2) additional value to sync end of patches together) ]
	#	[ (4) footer ]
	#
	#
	
	def _dted_patch_get_filepath( xlong, ylat ):
		"""
			Given a xlongitude and ylatitude	(-long == west) (-lat = south)
		
			(0 = W180, 360 = E180, 0 = S90, 180 = N90)
			
			w001...w180	, e000..e179	#east is positive direction
			s90..s01 , n00..n90		#north is positive direction
		
			DTED directory (or DTED0, DTED1, DTED2 directory...)
			e000/
				n00.dt0		#DTED0
				s00.dt0
				n00.dt1		#DTED1
				s00.dt1
				n00.dt2		#DTED2
				s00.dt2
			w001/	(negative xlong)
				n00.dt0		#DTED0
				s00.dt0
				n00.dt1		#DTED1
				s00.dt1
				n00.dt2		#DTED2
				s00.dt2
				
			There is an issue with patches near degree boundaries.
		"""
		return "";
		
	def _dted_load_dt_patch( self, stream, x, y, nxsamples=60, nysamples=60, nxblocks=60, nyblocks=60 ):
		"""
			1 x 1 units	60/1 = 60 units per axis
			Header is 3428 bytes
			Each BLOCK is 7214 bytes
			Each STRIP is 7202 bytes ( 3601 samples ) => 60 * 60 (1x1 second contains 60x60 + 1 samples, 1 block contains 60*60 + 1 of those strips)
			
			FILE (varies in availability)
				PATCH (60x60 at most, varies with latitude)
					SAMPLE (60x60 at most 2 byte ints, varies with latitude)
					
			x = 0..60	(1 patch for each 1/60th of a degree)	Note these numbers CHANGE depending on the file & latitude so... be warned!
			y = 0..60	(1 patch for each 1/60th of a degree)	Note these numbers CHANGE depending on the file & latitude so... be warned!
			
		"""
		#nxblocks = 60;
		#nyblocks = 60;
		#nxsamples = 60;
		#nysamples = 60;
		
		#x,y vary depending on patch...
		
		
		header_skip = 3428;	#UHM fixed header size PER PATCH
		nxsamplesp1 = nxsamples + 1;
		nysamplesp1 = nysamples + 1;
		strip_size = (nxsamples * nysamples + 1)*2;	#7202 == (60*60 + 1)*2
		strip_block_size = strip_size + 12	#Each block header has an additional 12 bytes?? well, 7214 and 2414 are sizes known... ( 60x60 and 60x20 patches )
		strip_read_size = nxsamplesp1*2	#60*2 = 120, 61*2 = 122
		strip_read_skip_size = strip_block_size - strip_read_size
		#D = numpy.zeros( nxsamplesp1, nysamplesp1 );	#Allocate array space for the selected patch
		
		seekconstant = (header_skip) + 8 + (y*nyblocks)*2 + (x*nxblocks) * strip_size;	#//Start at a specificy y + x coordinate
		
		stream.seek( seekconstant, 0 );	#Jump to start of a PATCH in file.
			
		#This methodology could be more optomized.
			
		bytes = numpy.zeros( strip_read_size, dtype='uint8' );	#Only need ONE buffer to read into for this thing.
		
		ixmax = nxsamples+1
		iymax = nysamples+1;
		#ix = 0;
		#while( ix < ixmax ):
		iy = 0;
		while( iy < iymax ):
			#seekhere = seekconstant + (ix * 7214);	#Offset to require block x
			
			stream.readinto( bytes );	#Read the current block (reading is more efficient than seeking due to caching behavior / buffered file IO... even if we only read 1/10th the information.)
			stream.seek( strip_read_skip_size, 1 );	#Skipping from CURRENT position is efficient as well
			
			#dtedin.seekg( seekhere, std::ios::beg );
			#dtedin.read( (char*)self.block_buffer, 122 );	//Hm.
			#int readbytes = dtedin.gcount();
			#uchar * sx = block;
			
			bi = 0;
			#iy = 0;
			#while( iy < iymax ):
			ix = 0;
			while( ix < ixmax ):
				
				#Remember, it's 1's compliment. Not 2's compliment. Also funky byte ordering.
				#magnit = int(bytes[0]) & 127;
				#magnit <<= 8;
				#magnit |= int(bytes[1]);
				magnit = (((int(bytes[bi])) & 127) << 8) | ((int(bytes[bi+1])) & 255);
				if( (bytes[bi]&128) != 0 ):
					magnit = -magnit;
			
				self.patch_buffer[ ix + iy*nxsamplesp1 ] = magnit;
			
				#iy += 1;
				ix += 1;
				bi += 2;
			#ix += 1;
			iy += 1;
			
		#numpy.delete( bytes );	#free buffer...
			
		return self.patch_buffer;
	
	
	
	
	"""
	//minx / miny obey unsigned convention (0 = W180, 360 = E180, 0 = S90, 180 = N90)
	int loadFilePatch( int longxmin, int latymin, dted_patch & D )	//returns data for the requested path (each minute square has a patch)
	{
		//Load required DTED file.
		ifstream dtedin;
		std::string filepath ="";

		//"e000"	"e179"	(+x)	THUS e000 covers: [e000 .. e001)
		//"w001"	"w180"	(-x)	THUS w001 covers: [w000 .. w001)
		//
		//"n00." "n59."	(+y)	+ ".dt0" or ".dt1" or ".dt2"	THUS n000 covers: [n000 .. n001)
		//"s01." "s60."	(+y)	+ ".dt0" or ".dt1" or ".dt2"	THUS s001 covers: [s000 .. s001)
		//
		//THESE DTED files are stored in blocks with vertical strips.
		//Each FILE contains BLOCKS, each BLOCK covers a 1x1 minute patch inside a 1x1 degree FILE.
		//Each BLOCK contains STRIPS, which each STRIP covers a unique longitude value.
		//Inside each STRIP, there are SAMPLES which are individual altitudes.
		//Every BLOCK contains 1 extra overlapping strip, and each STRIP contains 1 extra overlapping SAMPLE.
		//
		if( dtedpaths[2] != "" ){
			filepath = dtedpaths[2] + patchFileConvert( longxmin, latymin, 2 );
			dtedin.open( filepath.c_str(), std::ios::in | std::ios::binary );
		}else{
			dtedin.close();
		}
		if( dtedin.is_open() ){

			D.x_second = longxmin*60;	//If each PATCH is a 1x1 MINUTE block... then each patch contains 60x60 seconds.
			D.y_second = latymin*60;
			D.x_second_size = 60;
			D.y_second_size = 60;

			short minval = 0;
			short maxval = 0;

			readcache.resize( 300 );//7400 );//7214 + 360 );
			uchar * block = &readcache[0];	//new uchar[7214];

			//The following applies for values BELOW 50 latitude.
			if( (latymin >= 50*60 ) && (latymin < (50 + 90)*60 ) ){

				D.dataAlloc( 61, 61 );//, 60 + 60 + 1 );	//VARIES depending on level...

				int n_x = longxmin - ((longxmin/60))*60;
				int n_y = latymin - ((latymin/60))*60;

				bool firstval = 1;

				//1 x 1 units	60/1 = 60 units per axis
				//Header is 3428 bytes
				//Each BLOCK is 7214 bytes
				//Each STRIP is 7202 bytes ( 3601 samples ) => 60 * 60 (1x1 second contains 60x60 + 1 samples, 1 block contains 60*60 + 1 of those strips)
				//
				int seekconstant = (3428 + 8) + 2*(n_y*60) + (n_x*60) * 7214;	//Start at a specificy y + x coordinate

				for( int ix = 0; ix < 61; ix++ ){

					int seekhere = seekconstant + (ix * 7214);	//Offset to require block x
					dtedin.seekg( seekhere, std::ios::beg );
					dtedin.read( (char*)block, 122 );	//Hm.
					int readbytes = dtedin.gcount();

					uchar * sx = block;
					for( int iy = 0; iy < 61; iy++ ){

						short magnit = ((((int)(sx[0])) & 127) << 8) | (((int)(sx[1])) & 255);
						if( (sx[0]&128) != 0 ){ magnit = -magnit; }
						if( firstval ){ minval = magnit; maxval = magnit; firstval = 0; }
						if( magnit < minval ){ minval = magnit; }
						if( magnit > maxval ){ maxval = magnit; }

						D.data[ ix + iy*61 ] = magnit;	//METERS (?) raw (includes error values, which are easy to detect (less than ? meters? lol.)

						sx += 2;
					}
				}
			}else{

				//50-69 latitudes have different rules, because EACH longitude strip now covers TWO degrees of longitude, and vertical spacing remains the same?

				//Each FILE contains STRIPS; Each strip in this file is (1/30) * (60) degrees of longitude. Latitude is still 1/60, so strips are still 3600 tall.

				D.dataAlloc( 31, 61 );//, 60 + 60 + 1 );	//VARIES depending on level...

				int n_x = longxmin - ((longxmin/60))*60;
				int n_y = latymin - ((latymin/60))*60;

				bool firstval = 1;

				//1 x 1 units	60/1 = 60 units per axis
				//Header is 3428 bytes
				//Each BLOCK is 7214 bytes
				//Each STRIP is 7202 bytes ( 3601 samples ) => 60 * 60 (1x1 second contains 60x60 + 1 samples, 1 block contains 60*60 + 1 of those strips)
				//
				int seekconstant = (3428 + 8) + 2*(n_y*60) + (n_x*30) * 7214;	//Start at a specificy y + x coordinate

				for( int ix = 0; ix < 31; ix++ ){

					int seekhere = seekconstant + (ix * 7214);	//Offset to require block x
					dtedin.seekg( seekhere, std::ios::beg );
					dtedin.read( (char*)block, 122 );	//Hm.
					int readbytes = dtedin.gcount();

					uchar * sx = block;
					for( int iy = 0; iy < 61; iy++ ){

						short magnit = ((((int)(sx[0])) & 127) << 8) | (((int)(sx[1])) & 255);
						if( (sx[0]&128) != 0 ){ magnit = -magnit; }
						if( firstval ){ minval = magnit; maxval = magnit; firstval = 0; }
						if( magnit < minval ){ minval = magnit; }
						if( magnit > maxval ){ maxval = magnit; }

						D.data[ ix + iy*31 ] = magnit;	//METERS (?) raw (includes error values, which are easy to detect (less than ? meters? lol.)

						sx += 2;
					}
				}
			}

			//Data is now in a PATCH of (x longitude increasing E to W) * (y latitude increasing S to N)
			D.min_m = minval;
			D.max_m = maxval;

			dtedin.close();
			return 1;

		}else{
			dtedin.close();

			if( dtedpaths[1] != "" ){
				filepath = dtedpaths[1]+ patchFileConvert( longxmin, latymin, 1 );
				dtedin.open( filepath.c_str(), std::ios::in | std::ios::binary );
			}
			if( dtedin.is_open() ){

				//DTED 1 table:
				//	yxx
				//	3x3
				//	3x6
				//

				//Load in a DTED1 file patch (file data changes based on latitude)

				D.x_second = longxmin*60;	//If each PATCH is a 1x1 MINUTE block... then each patch contains 60x60 seconds.
				D.y_second = latymin*60;
				D.x_second_size = 60;
				D.y_second_size = 60;

				short minval = 0;
				short maxval = 0;

				readcache.resize( 300 );//7400 );//7214 + 360 );
				uchar * block = &readcache[0];	//new uchar[7214];


				if( (latymin >= 50*60 ) && (latymin < (50 + 90)*60 ) ){
					D.dataAlloc( 21, 21 );//, 60 + 60 + 1 );	//VARIES depending on level...

					int n_x = (longxmin - ((longxmin/60))*60);	//0..60	(strip index, multiply by 20)
					int n_y = (latymin - ((latymin/60))*60);	//0..60 (sample index, multiply by 20)

					bool firstval = 1;

					//3 x 3 units	60/3 = 20 units per axis
					//Header is 3428 bytes
					//Each BLOCK is 2414 bytes
					//Each STRIP is 2402 bytes ( 1201 samples ) => 60 * 20 (1x1 second contains 20x20 + 1 samples, 1 block contains 20*60 + 1 of those strips)

					int seekconstant = (3428 + 8) + 2*(n_y*20) + (n_x*20) * 2414;	//Start at a specificy y + x coordinate

					for( int ix = 0; ix < 21; ix++ ){

						int seekhere = seekconstant + (ix * 2414);	//Offset to require block x
						dtedin.seekg( seekhere, std::ios::beg );
						dtedin.read( (char*)block, 42 );	//Hm.
						int readbytes = dtedin.gcount();

						uchar * sx = block;
						for( int iy = 0; iy < 21; iy++ ){

							short magnit = ((((int)(sx[0])) & 127) << 8) | (((int)(sx[1])) & 255);
							if( (sx[0]&128) != 0 ){ magnit = -magnit; }
							if( firstval ){ minval = magnit; maxval = magnit; firstval = 0; }
							if( magnit < minval ){ minval = magnit; }
							if( magnit > maxval ){ maxval = magnit; }

							D.data[ ix + iy*21 ] = magnit;	//METERS (?) raw (includes error values, which are easy to detect (less than ? meters? lol.)

							sx += 2;
						}
					}
				}else{

					//50-69 degrees lat

					D.dataAlloc( 11, 21 );//, 60 + 60 + 1 );	//VARIES depending on level...

					int n_x = (longxmin - ((longxmin/60))*60);	//0..60	(strip index, multiply by 20)
					int n_y = (latymin - ((latymin/60))*60);	//0..60 (sample index, multiply by 20)

					bool firstval = 1;

					//3 x 6 units	60/3 = 20 units per axis
					//Header is 3428 bytes
					//Each BLOCK is 2414 bytes
					//Each STRIP is 2402 bytes ( 1201 samples ) => 60 * 20 (1x1 second contains 20x20 + 1 samples, 1 block contains 20*60 + 1 of those strips)

					int seekconstant = (3428 + 8) + 2*(n_y*20) + (n_x*10) * 2414;	//Start at a specificy y + x coordinate

					for( int ix = 0; ix < 11; ix++ ){

						int seekhere = seekconstant + (ix * 2414);	//Offset to require block x
						dtedin.seekg( seekhere, std::ios::beg );
						dtedin.read( (char*)block, 42 );	//Hm.
						int readbytes = dtedin.gcount();

						uchar * sx = block;
						for( int iy = 0; iy < 21; iy++ ){

							short magnit = ((((int)(sx[0])) & 127) << 8) | (((int)(sx[1])) & 255);
							if( (sx[0]&128) != 0 ){ magnit = -magnit; }
							if( firstval ){ minval = magnit; maxval = magnit; firstval = 0; }
							if( magnit < minval ){ minval = magnit; }
							if( magnit > maxval ){ maxval = magnit; }

							D.data[ ix + iy*11 ] = magnit;	//METERS (?) raw (includes error values, which are easy to detect (less than ? meters? lol.)

							sx += 2;
						}
					}
				}

				D.min_m = minval;
				D.max_m = maxval;

				dtedin.close();
				return 1;

			}else{
				dtedin.close();

				if( dtedpaths[0] != "" ){
					filepath = dtedpaths[0] + patchFileConvert( longxmin, latymin, 0 );
					dtedin.open( filepath.c_str(), std::ios::in | std::ios::binary );
				}
				if( dtedin.is_open() ){

					//Load in a DTED0 file patch (file data changes based on latitude)

					//The real magic happens here. (loading binary data)

					dtedin.close();
					return -1;
				}else{
					dtedin.close();

				}
			}
		}

		return -1;
	}
	"""
		
def run():
	pass;

if __name__	== "__main__":
	sys.exit( run() )