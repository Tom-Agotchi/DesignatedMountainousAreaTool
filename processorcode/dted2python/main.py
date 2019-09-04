
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

#------------------------------------------------------------------------------

sys.path.append( "./" );

from eletools import ElevationGetter

#------------------------------------------------------------------------------

# Plot setup

#Make a samples and smooth kernel plot?
class ShowmeThings:

	def __init__(self):
	
		self.wait_begin( 0 );
		self.grid_x_long = -104.7146;	#Devils tower (WILL NOT be visible in dted...)
		self.grid_y_lat = 44.509;
		self.grid_bounds = [];
		self.grid_samples =  numpy.random.rand( 60, 60 );
		self.grid_samples_changed = True;
		
		self.file_stream = open( "Y:\\Incoming\\DTED2\\w104\\n39.dt2", 'rb' );
		
		self.fig = matplotlib_pyplot.figure("Terrain Square", facecolor='#cccccc', figsize=(960/80,540/80), dpi=80 )
		
		self.axis = matplotlib_pyplot.subplot(1, 1, 1,
								  #xlim=(0, 100),
								 # ylim=(0, 100),
								  facecolor="#808080")
		self.axis_changed = True;
								  
		self.imgplot = self.axis.imshow( numpy.random.rand( 100, 100 ) ,interpolation='nearest' );#, cmap = cmap,norm=norm)
					
		self.eg = ElevationGetter();
		self.eg.set_path( 'Y:\\Incoming\\DTED1' );
		#self.eg.get_dted_grid_at( self.grid_x_long, self.grid_y_lat );	#Use another process for this? ... hm. Would be BEST!
		
	def getchanges( self ):
		return [ self.imgplot ];	#self.axis ];
				
	#Huh. Wait locking.
	def wait_begin( self, seconds=0.05 ):	#50ms === 20 FPS, 100ms = 10FPS
		self.tstart = time.process_time();
		self.tamount = seconds;
		
	def wait_update( self ):
		if( (time.process_time() - self.tstart) < self.tamount ):
			time.sleep( 0.000 );
			return False;
		return True;
			
	def update( self ):
	
		self.axis_changed = False;
				
		if( self.wait_update() ):
			self.wait_begin();
			
			#Check if we have the data or not
			if self.eg.grid_ready():
				data = self.eg.grid_get();
				self.eg.grid_clear();
				
				#Process samples in some way?
				self.grid_samples = data;
				self.grid_samples_changed = True;
			
			if self.grid_samples_changed:
				self.grid_samples_changed = False;
				
				self.axis.clear();
				self.imgplot = self.axis.imshow( self.grid_samples ,interpolation='nearest' );#, cmap = cmap,norm=norm)
				#self.imgplot.set_data( self.grid_samples );
				#self.axis.relim();
				#self.axis.autoscale_view()
				
				#Grid blitting is a issue. we would like to see a FEW patches (3x3 grid maybe?)
				#	Hm. then SCROLLING the patches as we move around is a thing but. What about real scale?
				#		<- Each patch should be reduceable statistically.
				#			Max( 2x2 ), Min( 2x2 ), ExpectedValue( 2x2 ), Var( 2x2 )
				#		<- SKH would be intersting here (from CENTER of patch, what's the height distribution lol)
				#		then we tree that up by our cell size.
				#		Meaning, a 60x60 => 30x30 => 15x15 => ?7x7 => 3x3? => 1x1?
				#	A FILE of DTED data covers some amount of a DEGREE of the earth.
				#		As a result, a single FILE can AT MOST represent 60x60 nmi of data.
				#			Individual patches are at a smaller level.
				#			It may make sense to dump an ENTIRE file into a patch structure that we can use
				#				(difference encoding means each sample => tree, log2(n))
				#				DIRECT encoding takes up a LOT of memory.
				#	We would prefer a more addressible file structure for this.
				#	Simply, each file should be a 4CC system:
				#		[ (4) dpHd ][ (4) data byte size (0 padded to 4 bytes) ]	header
				#			[ (4) patch count ][ (4) xsamples ][ (4) ysamples ][ (4) typefield ]
				#			[ (4) long min ms, (4) lat min ms, (4) long max ms, (4) lat max ms ]
				#			[ (4) skip to header patch list ][ (4) skip to patches ][ (8) header fixed code ]
				#			[ (16) header fixed pad ]
				#			[ ... ]
				#		[ (4) dpHl ][ (4) data byte size (0 padded to 4 bytes) ]	header patch offset list size
				#			[ (4) offset from START OF FILE to the nth patch ] ...
				#		[ (4) dpPc ][ (4) data byte size (0 padded to 4 bytes) ]	patch of samples that should contain xsamples*ysamples*sizeof( typefield ) in y0x0, y0x1, ... y0xn, y1x0, y1x1, ... order )
				#			[ patch data ]
				#	that should
				#
				
				xsamples=60;
				ysamples=60;
				x = int( numpy.random.rand()*60 )
				y = int( numpy.random.rand()*60 )
				#fin = open( "Y:\\Incoming\\DTED2\\w081\\n32.dt2", 'rb' );
				#fin = open( "Y:\\Incoming\\DTED2\\w104\\n39.dt2", 'rb' );
				wapow = self.eg._dted_load_dt_patch( self.file_stream, x, y );
				#fin.close();
				
				#Copy patch into image data
				
				self.grid_patches = {};
		
				xsize = xsamples+1;
				ysize = ysamples+1;
				d = [];
				yrow = 0;
				while yrow < ysize:
					y = xsize*yrow;
					d.append( wapow[y:y+xsize] );
					yrow += 1;
				self.grid_samples = d;
					
				self.grid_samples_changed = True;
		
				self.axis_changed = True;
		
		return self.getchanges() # Update only what changed
		
		
globaldats = ShowmeThings();
		
def animate(idx):
	return globaldats.update();
	
def init():
	globaldats.fig.tight_layout()
	return globaldats.getchanges()
	
#------------------------------------------------------------------------------
def run():

	# Animate! (assignment to anim is needed to avoid garbage collecting it)
	anim = FuncAnimation(globaldats.fig, animate, init_func=init, interval=1, blit=True)	#blit = True is FASTER but,
	#matplotlib_pyplot.ioff()
	#matplotlib_pyplot.show() # Blocking

	matplotlib_pyplot.show();

#------------------------------------------------------------------------------
if __name__	== "__main__":
	sys.exit(run())