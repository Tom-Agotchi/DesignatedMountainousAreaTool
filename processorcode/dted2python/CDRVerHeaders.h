
#ifndef CDRVERHEADER_H
#define CDRVERHEADER_H

//#undef CDR_HARDDEBUG
//#define CDR_HARDDEBUG

#include "IndexableArray.h"
#include "./cdrsources/stringMapping.h"

#include "GHOST_Wrapper.h"

#include "uMap.h"
#include "arrayV.h"
#include "arrayO.h"

#include <deque>

#include "mapCustom.h"	//nice
#include "uniqueMapT.h"	//nice
#include "lvColumnData.h"	//nice

#include <sys/stat.h>
#include <errno.h>
extern int errno;

#include "../core/vec3.h"
#include "../core/matrix4x4.h"
#include "../mingl/mingl.h"

#include "../bmp_fonts/GLtexture_font.h"

#include "./cdrsources/stopwatch.h"
#include "./cdrsources/cdr_core.h"
#include "./cdrsources/cdr_structures.h"
#include "./cdrsources/cdr_airport_hash.h"
#include "./cdrsources/cdr_track_hash.h"

//#include "./cdrsources/dted_getter.h"

#include "../core/gaplessrange.h"

#include "../core/epoch_day.h"
#include "../core/gisloc.h"

#include "../core/timedate.h"

//We *have* to support zip files for traf5 formats, and it'll make working with NOP a lot easier if it's zipped.
//Though, this is a MAJOR revision, changes a LOT.
#include "./zip/unzip/unzip.h"

//ORACLE Oracle interface
//#include "oci.h"
#include "../../Utility/Oracle/ocilib/include/ocilib.h"

#include "pxlib/pxLib.h"

#include "./cdrsources/cdrlua.h"

#include "./sanitize.h"

//CURL interface:
//Requires (win32):
//	libvc6libcurl.a
//	libwsock32.a
//	libwldap32.a
#include <curl/curl.h>

//Draw some damn models!
#include "./cdrsources/gModel.h"

//Time to get srs with IGL!
#include "iglGeo.h"

#include "stringhashmap.h"

#include "toString.h"

//Additional game modes here:
#include "./cdrsources/gamemode.h"
#include "./cdrsources/graphmode.h"
#include "./cdrsources/easymode.h"

//NEW entire input abstraction
#include "GWRAP_Inputs.h"
#include "GWRAP_InputsData.h"

#include "./cdrsources/SFMT.h"

#include "sguiI.h"

#include "sphereCube.h"

#include "statSet.h"

#include "CDRVer_vsfInternalProcess.h"

#include "KarneyGeod/geodesic.h"

//AUDIO system!
#include "..\allib\alLib.h"
#include "..\espeak\speak_lib.h"


//When you commit a memory crime, use this:
#ifdef USE_MEMORY_TRACKER
	//Required to trigger memory tracking!!! but difficult to use really; unless you heapify all your weird classess
	#define MM_DEBUG
	//#pragma include( );
	#include "../../Utility/IGTL/igtl_MMHeap.h"
#endif

class CDRVer;

std::vector< uint > string_calc_splits( const uchar * buffer, const uint b_length, const std::string delimstr );

//////////////////////////////////////////////////////////////////////////////////
//
//	Hardcore value conversion tools:
//		Interesting you can't tell the compiler WHAT built in type conversions are OK...
//		Even more there aren't some conveinient converting builtin!
//		Not to mention the compiler dies on a const const value... how stupid.
//
template< typename _T, typename _R >
bool univAssign( const _T & A, _R & B )
{
	B = A;
	return true;
}

//Obviously, well defininig data <--> string conversions is very important here if you SCREW UP AND DO THIS... strings are for LOADING ONLY!
template< typename _T, typename _R >
bool univAssign( const _T & A, std::string & B )
{
	std::stringstream ss;
	ss<<A;
	B = ss.str();
	return true;
}

/*

//template< typename _T >
//bool toTypeCode( const _T * p, int &ttypesize ) { ttypesize = sizeof(_T); ttypesize=0; return false; }

//template<> bool univAssign( const void * & A, float & B ) { return false; }
//template<> bool univAssign( const float & A, void * & B ) { return false; }

//* -> STRING, as a DXML <hexdata> element. Note this IGNORES ALL STRUCTURE...
template< typename _T > bool univAssign( _T & A, std::string & B )
{
	int nbytes = sizeof( _T );
	uchar * P = static_cast< uchar * >( static_cast< void * > ( (&A) ) );
	char Htable[17]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','0'};
	B = "<hexdata>";
	for( int i = 0; i < nbytes; i++ ){
		B += Htable[ (P[i] & 15) ];
		B += Htable[ ((P[i]>>4) & 15) ];
	}
	B += "</hexdata>";
	return true;
}
*/
//
//////////////////////////////////////////////////////////////////////////////////

//really, really need to take this string kit and make it a component.
//
//	PLENTY of things to do.
//
bool stringEqualsCaseless( const std::string & A, const std::string & B, uint maxlen = 0 );
int stringIsNumeric( const uchar * instr, uint len );
double stringToDouble( const uchar * instr, uint len );
std::string stringToUppercase( const std::string & A );

extern std::map< std::string, uint > string_mapaxisnamecaseless;
extern std::map< std::string, double > string_maplinearunitscaseless;	//Linear units => nautical miles

void init( CDRVer * CDR );

struct filesource
{
	std::string filename;
	std::string aux1;
	std::string aux2;
	int type;
};


struct airsplit{

	uint portdex;
	uint rwaydex;
	uint rwaycount;
	std::vector< float > rwayparams;
};

struct tracksplitkey{

	//int dyear = S.key.initial_date / (32*12);
	//int dmonth = (S.key.initial_date / (32))%12;
	//int dday = S.key.initial_date % 32;
	//int startms = S.key.initial_time;
	//int endms = S.key.final_time_delta;

	//epoch_day DD;
	//DD.to_epoch_day( dyear, dmonth, dday );

	int64_t xstime;//(8) = (( (long)DD.get() )*1000*60*60*24) + (long)startms;

	uint initial_date;	//(4) Initial day (sets 32 days per month, 12 months per year)
	uint initial_time;	//(4) Initial milliseconds
	uint final_time_delta;	//(4) Final delta time in milliseconds
	uchar track_color[4];	//(4) Assignable track color

	uint sourceid;			//(4) Input source id (mapped data)

	//29 characters, how about 30 with definitions:
	unsigned char dkey[36];	//(32)	--> extended to 36 for alignment! what are the other 4 characters?

	//char airport[3];
	//char fltnum[4];
	//char fltnum2[4];
	//char beacon[4];
	//char type[1];
	//char craft[4];
	//char calc_airport[6];
	//char calc_runway[3];

	tracksplitkey(){

		initial_date = 0;	//(4) Initial day (sets 32 days per month, 12 months per year)
		initial_time = 0;	//(4) Initial milliseconds
		final_time_delta = 0;	//(4) Final delta time in milliseconds
		xstime = 0;
		sourceid = 0;
	}

	tracksplitkey( const tracksplitkey & old ){

		if( this != &old ){
			initial_date = old.initial_date;	//(4) Initial day (sets 32 days per month, 12 months per year)
			initial_time = old.initial_time;	//(4) Initial milliseconds
			final_time_delta = old.final_time_delta;	//(4) Final delta time in milliseconds
			memcpy( track_color, old.track_color, 4 );
			memcpy( dkey, old.dkey, 36 );
			xstime = old.xstime;
			sourceid = 0;
		}
	}
	tracksplitkey & operator =( const tracksplitkey & old ){

		if( this != &old ){
			initial_date = old.initial_date;	//(4) Initial day (sets 32 days per month, 12 months per year)
			initial_time = old.initial_time;	//(4) Initial milliseconds
			final_time_delta = old.final_time_delta;	//(4) Final delta time in milliseconds
			memcpy( track_color, old.track_color, 4 );
			memcpy( dkey, old.dkey, 36 );
			xstime = old.xstime;
			sourceid = old.sourceid;
		}
		return *this;
	}

	inline const unsigned char * get_fltnum() const { return &dkey[0]; }//4
	inline const unsigned char * get_fltnum2() const { return &dkey[4]; }//4
	inline const unsigned char * get_beacon() const { return &dkey[8]; }//4
	inline const unsigned char * get_craft() const { return &dkey[12]; }//4
	inline const unsigned char * get_type() const { return &dkey[16]; }//1
	inline const unsigned char * get_airport() const { return &dkey[17]; }//3
	inline const unsigned char * get_calc_runway() const { return &dkey[20]; }//3
	inline const unsigned char * get_calc_airport() const { return &dkey[23]; }//6
	inline const unsigned char * get_group() const { return &dkey[29]; }//1	//Group key (8 bits)
	inline const unsigned char * get_enabled() const { return &dkey[30]; }//1	//

	inline unsigned char * get_fltnum() { return &dkey[0]; }//4
	inline unsigned char * get_fltnum2() { return &dkey[4]; }//4
	inline unsigned char * get_beacon() { return &dkey[8]; }//4
	inline unsigned char * get_craft() { return &dkey[12]; }//4
	inline unsigned char * get_type() { return &dkey[16]; }//1
	inline unsigned char * get_airport() { return &dkey[17]; }//3
	inline unsigned char * get_calc_runway() { return &dkey[20]; }//3
	inline unsigned char * get_calc_airport() { return &dkey[23]; }//6
	inline unsigned char * get_group() { return &((uchar*)dkey)[29]; }//1	//Group key (8 bits)
	inline unsigned char * get_enabled() { return &dkey[30]; }//1	//

	inline const unsigned char * get_callsign() const { return &dkey[0]; }//7
	inline unsigned char * get_callsign() { return &dkey[0]; }//7

	//inline uchar * get_sensor_index() { return &dkey[31]; }
	inline int get_sensor_index() const { return dkey[31]; }

	inline int64_t get_xstime() const { return xstime; }

	inline uint get_sourceid() const { return sourceid; }
	inline void set_sourceid( uint v ) { sourceid = v; }

	inline int get_initial_year() const { return initial_date/(32*12); }
	inline int get_initial_month() const { return ((initial_date/(32))%12) + 1; }
	inline int get_initial_day() const { return (initial_date%32) + 1; }
	inline void set_initial_date( int uyear, int umonth, int uday ) { initial_date = 32*( (umonth-1) + 12*(uyear) ) + (uday-1); }

	inline int get_initial_time() const { return initial_time; }	//in day ms
	inline int get_initial_hour() const { return (initial_time/(1000*60*60)) %24; }	//in hours
	inline int get_initial_minute() const { return (initial_time/(1000*60)) %60; }	//in minutes
	inline int get_initial_second() const { return (initial_time/(1000)) %60; }	//in seconds
	inline int get_initial_millisecond() const { return initial_time%1000; }	//in day ms

	inline void calc_xstime() {

		int dyear = get_initial_year();//initial_date / (32*12);
		//int dmonth = 1 + get_initial_month();// 1 + (initial_date / (32))%12;
		//int dday = 1 + get_initial_day();//1 + initial_date % 32;
		int dmonth = get_initial_month();// 1 + (initial_date / (32))%12;
		int dday = get_initial_day();//1 + initial_date % 32;
		int startms = initial_time;

		if( dday < 1 ){ dday = 1; }
		if( dmonth < 1 ){ dmonth = 1; }
		if( dyear <= epoch_day::_epoch_year ){ dyear = epoch_day::_epoch_year + 1; }
		if( dyear > epoch_day::_epoch_end_year ){ dyear = epoch_day::_epoch_end_year; }
		dmonth %= 13;
		dday %= 32;

		epoch_day DD;
		DD.to_epoch_day( dyear, dmonth, dday );

		xstime = (( (int64_t)DD.get() )*1000*60*60*24) + (int64_t)startms;
	}


	const unsigned char * get_offset_by_sid( const std::string & sid )	//String id
	{
		//Name to data conversion; Slow, but needed for generalization?

		//map string -> byte offset to data, byte size of data, type of data? (hmrgh; types need to be aligned, so, minimum 4 bytes per type, unless char string with size??)
		return 0;
	}

	const unsigned char * get_by_dkey( const uint offset, const uint size, const uint type )
	{
		//Range check, then pointer cast to offset?

		//Return pointer from (uchar*)this + offset; Has no type though... Hm.
		return 0;
	}

	//get_data( offset ? );

	//"sid" = String Identifyer, a human readable string to represent a logical data field
	//"dkey" = Data Key, used to specify a local offset, a byte size, and a internal type for accessing data generically.
	//
	//the sid/dkey interface allows complete generic data access; But it's not fast enough yet.
	//
	//string -> dkey_index -> dkey (offset, size, type)
	//user get's dkey_index (might be invalid index if nonexistant);
	//user can pass dkey_index to get data... but, how?
	//
	//float(4), uint(4), int(4), ushort(2), short(2), char(1), uchar(1), long(8), ulong(8), cstr(n, 0 0 terminated (two consecutive zero bytes (for unicode support)))
	//
	//		bool load_float( float * p );	//Either works or not (loads or not loads, if type matches)
	//		bool load_uint( uint * p );	//Either works or not (loads or not loads, if type matches)
	//		...
	//		bool load_string( uchar * s, uint n );	//Either works or not (loads or not loads, if type matches)
	//
	//		float get_float();	//Works for all numeric types?
	//		float get_int();	//Works for some intergers types, downconvers float via flooring
	//		...
	//
	//"generic" - a type to represent an array of generic data types; The problem is, how do you get it?
	//	the type knows what it is, but how do you access it, without knowing what it is in YOUR program?
	//	basically, if you want to access a generic list of elements, you can query it's type, but, you can;t call the same function to "get" the data!
	//
	//	carray< generic * > pointers_to_type_objects;	//Java-like
	//
	//
	//Only for loading files I guess.
	//
	//"string key" for defining the structure type,
	//	"data type" for defining the role of the data inside this structure, along with machine type and byte size (never changes)
	//
	//Data conversion is handled automaticaly to the best of possible abilities; IE "data type" gets converted into whatever we store internally.
	//Exactly like Blender's RNA/DNA system;
	//	Which is implemented as a property accessor;
	//	Properties are accessed by name, then you get access to the internal generic type;
	//	Strided access should be possible as well however...
	//

	inline bool enabled() const { return (get_enabled()[0] & 128) != 0; }
	inline bool set_enabled() { get_enabled()[0] |= 128; return true; }
	inline bool set_disabled() { get_enabled()[0] &= ~128; return false; }

	//Hm.... NEW stuff.
	//inline int get_tracktype() const { return (get_enabled()[0] & 15); }
	//inline void set_tracktype( uint v ) { get_enabled()[0] |= (v&15); }

	inline void setColor( float r, float g, float b, float a )
	{
		track_color[0] = r*255;
		track_color[1] = g*255;
		track_color[2] = b*255;
		track_color[3] = a*255;
	}
	inline float getColorR() const { return (track_color[0])/255.0f; }
	inline float getColorG() const { return (track_color[1])/255.0f; }
	inline float getColorB() const { return (track_color[2])/255.0f; }
	inline float getColorA() const { return (track_color[3])/255.0f; }
};

struct disttimepoint{

	disttimepoint();

	disttimepoint( const disttimepoint & B );

	//VA data:
	float x,y,z;	//(12)
	uchar rgba[4];	//(4)
	//Indexing data for analysis stuff
	int trackid;	//(4)
	int pointid;	//(4)
	float pointdt;	//(4)
	int pointtime;	//(4)
	//
	float dx,dy,dz;	//(12)
	short roll;		//(2)
	short pitch;	//(2)
};

struct nearinfo{

	nearinfo( uint ua, uint ub, uint uc ) : othertrack(ua), mid(ub), pid(uc) {}

	uint othertrack;
	uint mid;
	uint pid;
};

struct tracksplit{

	//Track key information (if exists)
	tracksplitkey key;		//(64?)

	//cdr::track_hash::iterator itr;	//-> itr.time_ms, itr.time_ms,
	//uint splitdex;	//(4)
	uint trackid;	//(4)	splitdex should never be used again. self referring trackid?
	uint count;		//(4)
	uint gl_vbo;	//(4)	hm! cache as a vbo? weird.

	uint flags;		//(4)

	float gl_linewidth;	//(4)
	uint gl_drawtype;	//(4)

	//std::vector< nearinfo > pair_ids;

	//std::vector< float > gl_trackdata;		//Stored SAME as regular GL data, { [r,g,b,a],x,y,z } which we now know is not a good format...
	//std::vector< uint > gl_trackdatatime;	//local times (split HAS the times and such)

	enum {
		INVISIBLE = 1,
		DISABLED = 2,
		RESERVED2 = 4,
		RESERVED3 = 8,
		RESERVED4 = 16,
		RESERVED5 = 32,
		UNTRACKED_TRACK = 64,	//Hm! Important...
		NON_TRACK = 128,

		FLAGS_MASK = 65535,

		CRAFT_TYPE_SHIFT = 24,
		CRAFT_TYPE_MASK = 127,	//Max of 254 loaded models!
		CRAFT_TYPE_NONE = (CRAFT_TYPE_MASK)<<CRAFT_TYPE_SHIFT
	};

	bool isTracked() const { return ((flags & UNTRACKED_TRACK) == 0); }
	int getTrackedType() const {
		int r = 0;
		if( isTracked() ){
			//
		}else{
			r += 1;
		}
		if( false ){
			r += 2;	//Hm... "dashed" lines are uh. ... require an extra parameter. Shader based coloring?
		}
		if( false ){
			r += 4;	//Hm
		}
		return r;
	}

	inline float glLineSize() const {
		return gl_linewidth;
	};
	inline uint glPrimType() const {
		return gl_drawtype;
	};

	tracksplit(){
		//splitdex = 0;
		trackid = 0;
		count = 0;
		gl_vbo = mingl::INVALID_INDEX;
		flags = CRAFT_TYPE_NONE;
		//pair_ids;
		gl_linewidth = 1;
		gl_drawtype = GL_LINE_STRIP;
	}

	tracksplit( const tracksplit & old ){
		//if( this != &old ){
			//itr = old.itr;	//-> itr.time_ms, itr.time_ms,
			//splitdex = old.splitdex;
			trackid = old.trackid;
			count = old.count;
			gl_vbo = old.gl_vbo;
			key = old.key;
			flags = old.flags;
			//pair_ids.assign( old.pair_ids.begin(), old.pair_ids.end() );
			gl_linewidth = old.gl_linewidth;
			gl_drawtype = old.gl_drawtype;
		//}
	}

	tracksplit & operator=( const tracksplit & old ){
		//if( this != &old ){
			//itr = old.itr;	//-> itr.time_ms, itr.time_ms,
			//splitdex = old.splitdex;
			trackid = old.trackid;
			count = old.count;
			gl_vbo = old.gl_vbo;
			key = old.key;
			flags = old.flags;
			//pair_ids.assign( old.pair_ids.begin(), old.pair_ids.end() );
			gl_linewidth = old.gl_linewidth;
			gl_drawtype = old.gl_drawtype;
		//}
		return *this;
	}
};

struct trackdatablock
{

	trackdatablock() : gl_trackdata( 0 ), gl_trackdatatime( 0 ), gl_size(0), pair_ids(0) {}

	~trackdatablock()
	{
		clear();
		if( pair_ids != 0 )
		{
			delete pair_ids;
			pair_ids = 0;
		}
	}

	trackdatablock( const trackdatablock & old ) : gl_trackdata( old.gl_trackdata ), gl_trackdatatime( old.gl_trackdatatime ), gl_size( old.gl_size ), pair_ids( old.pair_ids )
	{
		const_cast< trackdatablock & >( old ).gl_trackdata = 0;
		const_cast< trackdatablock & >( old ).gl_trackdatatime = 0;
		const_cast< trackdatablock & >( old ).gl_size = 0;
		const_cast< trackdatablock & >( old ).pair_ids = 0;
	}

	trackdatablock & operator=( const trackdatablock & old )
	{
		//NO! for FD*FAS*D! AARRGGHHH
		//
		//clear();

		int itsbroken = 10;
		itsbroken -= 10;
		return *this;
	}

	void resize( uint n )
	{
		/*
		if( gl_trackdata != 0 ){
			gl_trackdata->resize( (n*4) + 4 );
			gl_trackdatatime->resize( n + 1 );
		}else{
			gl_trackdata = new std::vector< float >;
			if( gl_trackdatatime == 0 )
			{
				gl_trackdatatime = new std::vector< uint >;
			}
			gl_trackdata->resize( (n*4) + 4 );
			gl_trackdatatime->resize( n + 1 );
		}
		*/
		if( n > gl_size ){

			float * newgl_trackdata = new float[ n*4 + 4 ];		//Stored SAME as regular GL data, { [r,g,b,a],x,y,z } which we now know is not a good format...
			uint * newgl_trackdatatime = new uint[ n + 1 ];	//local times (split HAS the times and such)
			if( (gl_trackdata != 0) && (gl_trackdatatime != 0)  ){
				for( uint i = 0; i < gl_size; i++ ){
					int ij = 4*i;
					newgl_trackdata[ij] = gl_trackdata[ij]; ij++;
					newgl_trackdata[ij] = gl_trackdata[ij]; ij++;
					newgl_trackdata[ij] = gl_trackdata[ij]; ij++;
					newgl_trackdata[ij] = gl_trackdata[ij];
					newgl_trackdatatime[i] = gl_trackdatatime[i];
				}
				delete [] gl_trackdata;
				delete [] gl_trackdatatime;
			}
			gl_trackdata = newgl_trackdata;
			gl_trackdatatime = newgl_trackdatatime;
			gl_size = n;
		}else if( n < gl_size ){

			gl_size = n;
			float * newgl_trackdata = new float[ gl_size*4 + 4 ];		//Stored SAME as regular GL data, { [r,g,b,a],x,y,z } which we now know is not a good format...
			uint * newgl_trackdatatime = new uint[ gl_size + 1 ];	//local times (split HAS the times and such)

			if( gl_trackdata != 0 ){
				memcpy( newgl_trackdata, gl_trackdata, sizeof(float)*4*(gl_size+1) );
				float * oldp = gl_trackdata;
				gl_trackdata = newgl_trackdata;
				delete [] oldp;
			}
			if ( gl_trackdatatime != 0 ){
				memcpy( newgl_trackdatatime, gl_trackdatatime, sizeof(uint)*(gl_size+1) );
				uint * oldp = gl_trackdatatime;
				gl_trackdatatime = newgl_trackdatatime;
				delete [] oldp;
			}
			gl_trackdata = newgl_trackdata;
			gl_trackdatatime = newgl_trackdatatime;
		}
	}

	inline uint size()
	{
		return gl_size;
	}

	inline void clear()
	{
		if( gl_trackdata != 0 )
		{
			delete [] gl_trackdata;
			gl_trackdata = 0;
		}
		if( gl_trackdatatime != 0 )
		{
			delete [] gl_trackdatatime;
			gl_trackdatatime = 0;
		}
		gl_size = 0;
	}

	inline uint & getTimeUint( uint n )
	{
		return gl_trackdatatime[ n ];
	}

	inline const uint & getTimeUint( uint n ) const
	{
		return gl_trackdatatime[ n ];
	}

	inline float & getDataFloat( uint n )	//Careful! doesn't do the 4 thing...
	{
		return gl_trackdata[ n ];
	}

	inline const float & getDataFloat( uint n ) const
	{
		return gl_trackdata[ n ];
	}

	//std::vector< float > * gl_trackdata;		//Stored SAME as regular GL data, { [r,g,b,a],x,y,z } which we now know is not a good format...
	//std::vector< uint > * gl_trackdatatime;	//local times (split HAS the times and such)
	//std::vector< nearinfo > * pair_ids;	//in case they exist per track! Hm...


	float * gl_trackdata;		//Stored SAME as regular GL data, { [r,g,b,a],x,y,z } which we now know is not a good format...
	uint * gl_trackdatatime;	//local times (split HAS the times and such)
	uint gl_size;
	std::vector< nearinfo > * pair_ids;	//in case they exist per track! Hm...

	/*
	void resize( uint n )
	{
		gl_trackdata.resize( (n*4) + 4 );
		gl_trackdatatime.resize( n + 1 );
	}

	getDataPtr( uint n ){ }
	*/
};

struct altspike{	//64 bytes per spike

	float x,y,z;	//(12)	Point on earth (hm)
	float nx,ny,nz;	//(12)	Normal on earth
	float dx,dy,dz;			//(12)	Plane NORMAL
	float minalt;			//(4)	Minimum used altitude
	float maxalt;			//(4)	Maximum used altitude
	float width;			//(4)	Width of this spike
	int flags;				//(4)	Flags about this spike
	int xlongms, ylatms; 	//(8)	Truth position (ms)(12)
	uchar color[4];			//(4)	Colorize it?
};

struct haystack_point{

	enum{
		ALL = 0,
		DEPARTURES,
		ARRIVALS,
		UNKNOWNS,
		BADS,
	};

	int lat_y;
	int long_x;
	float radius;
	std::string name;
	int flags;
	float pdegrees;
	float pradius;
};

struct d_manuplane{

	enum{
		USE_INSIDE = 1,
		USE_BEFORE_START = 2,
		USE_AFTER_END = 4,
		USE_ALL = USE_INSIDE | USE_BEFORE_START | USE_AFTER_END,
		USE_NOT_IN = USE_BEFORE_START | USE_AFTER_END,

		//Actual change flags

		SELECTED = 4,
		CHANGED = 1,

		DISTANCEGRID = 64,
		RADIAL = 128,	//In this case, the first point is the CENTER, second point defines RADIUS. Alt start/end are the same. (need arc segments? Hm. Hplanes allows for semis or greater._

		COUNT_0 = 1<<8,
		COUNT_1 = 1<<9,
		COUNT_2 = 1<<10,
		COUNT_3 = 1<<11,
	};

	int first_lat_y;		//
	int first_long_x;
	int last_lat_y;
	int last_long_x;		//(16)
	float alt_start_ft;
	float alt_end_ft;
	int flags;
	uint timedecayms;		//(16) Milliseconds this tile will time decay for in time subselect mode

	float rgba[4];	//(16) Plane color

	float coords[28];	//xyz for all 4 points on this plane, and the center and center + up point, and normal
	uint coordflags;
	std::string name;

	//float gmm_mean[2];	//Current mean and sigma for this plane...
	//float gmm_sigma[2];
	//float gmm_up[2];	//local up axis (cause it's a rotated/scaled distribution...)
	//float gmm_right[2];

	uint m_hitindex;	//Point to other hit data via cache index
	float angle_filter;	//useful!

	//struct hit{
	//	uint id;
	//	float x,y;
	//};
	//std::vector< hit > hits;	//?
};

struct d_manuplane_hit
{
	int m_planeid;	//Plane that we hit
	int m_trackid;	//Track that hit this plane
	int m_pointid;	//Point index just before or exactly at hit
	int m_timelocalms;	//Delta between this point and next point for hit time linear interpolation

	int m_flags;	//Flags about this point
    float m_xnmi;		//x hit coordinate (relative to pierce plane)
    float m_ynmi;		//y hit coordinate (relative to pierce plane)
    uchar m_color[4];  //Color RGBA value (8 bit)

    //float m_coord[4];	//x, y, z, radius (hm)
};

class d_manuplane_hits
{
	public:
		//int m_planeid;	//Plane that we hit
		std::vector< d_manuplane_hit > m_hits;

		int m_histoflag;

		//X and Y histogram data
		statSet<double> m_PSx;
		statSet<double> m_PSy;
		std::vector<float> m_xpoints;
		std::vector<float> m_ypoints;
		float m_xpoint_max;
		float m_ypoint_max;
		double m_rxbinstart;// = 0;
		double m_rxbinsize;// = 0;
		double m_rybinstart;// = 0;
		double m_rybinsize;// = 0;
		int m_xbins;// = 0;
		int m_ybins;// = 0;
		float m_xgramsize;
		float m_ygramsize;


		float m_halfwidthplane;// = mplanesize/2;
		float m_altstart;// = 0;
		float m_altend;// = CONVERT_FT_TO_NMI * (double)(MP.alt_end_ft - MP.alt_start_ft);

		float m_histobinsizexnmi;// = 100 * CONVERT_FT_TO_NMI;
		float m_histobinsizeynmi;// = 100 * CONVERT_FT_TO_NMI;
		float m_histobinwidthxnmi;// = 200 * CONVERT_FT_TO_NMI;
		float m_histobinwidthynmi;// = 200 * CONVERT_FT_TO_NMI;
		float m_xhistocolor[4];// = m_manuplanes_show_selecthits_histx_color;///{1,0.5,0.5,1};
		float m_yhistocolor[4];// = m_manuplanes_show_selecthits_histy_color;///{0.5,1,0.5,1};

	d_manuplane_hits()
	{
		m_histoflag = 0;
		m_xpoint_max = 0;
		m_ypoint_max = 0;
		m_xbins = 0;
		m_ybins = 0;
	}
};

struct gfx_navfix{	//48 bytes per waypoint

	float x,y,z;		//(12) x,y,z in nmi? ass. needs to be in cm!
	uchar r,g,b;		//(3)  color of this waypoint
	uchar type;			//(1) type of waypoint (hm)
	int sortindex;		//(4) (internal sorting index)
	//uchar name[31];		//(31) NAME of this waypoint (can truncate? observed = 28)
	//Replace with NAME IDEX, not NAME!
	//Then add in mode (vis, type, ect)
	uchar name[28];		//(28) NAME of this waypoint (can truncate? observed = 28)
};


struct display_joy{
	int active;
	int num_buttons;
	uint buttons;
	int num_analog;
	float analog[32];
};


struct joy_input_set{

	int type;
	int index;
	float scale;
	float deadrange;
};



struct missed_approach_thread_data
{
	int active;
	int currprogress_max;
	int currprogress;

	//??

	std::vector< uint > copied_view_list;	//HM. Better.
	//std::vector< uint > * view_list;
	const cdr::airport_hash * cdr_airports;
	const std::vector< uint > * track_time_data;
	std::vector< float > * track_data;
	const std::vector< tracksplit > * track_splits;

	std::vector< uint > ret_missed;	//Return of missed data.
	CDRVer * CDRP;
};


struct select_thread_data
{
	float x1,y1,z1;	//Initial direction
	float x2,y2,z2;	//Final direction
	float cx,cy,cz;	//center (start)
	float fx,fy,fz;	//final (final)
	int active;
	int currprogress_max;
	int currprogress;
	int display;
	std::vector< float > * track_data;
	std::vector< tracksplit > * track_splits;
	std::vector< uint > * view_list;
	CDRVer * CDRP;
};


struct write_thread_data
{
	int active;
	char curr_airport[6];
	char curr_runway[3];
	//cdr::airport_hash * cdr_airports;
	//cdr::track_hash * cdr_tracks;
	std::vector< tracksplit > * track_splits;
	std::map< int, std::string > * sensor_id_map;
	std::vector< uint > viewlist;
	std::vector< filesource > * sourceidlist;
	CDRVer * CDRP;
};

struct point_select_thread_data
{
	int active;
	int doit;
	float m_ray_point[3];
	float m_ray_dir[3];

	int current_track;	//CHECK ONLY one track at a time... hm...
	int status;
	//std::vector< uint > * viewlist_pointer;	//Oh my. this POINTER doesnt go out of scope, but... hm. it can change in the main thread!
};

//!!! Major feature ability change:
//-less ram (huh)
//-color change ability
//
//Make it so a thread can recolor tracks to indicate data
//	Hm!!!
//
//Change color of current SELECTION based on:
//	-> Recolor based on altitute
//	-> Based on proximity to some shape (?)
//	-> Type
//	-> Facility ID
//	-> Time + type
//	-> random Color per track
//	-> Time proximity?
//	-> Distance (lol)
//This is kinda what shaders are for though.
//
//Color by FILTER category (aircraft and such)
//	->craft, airport, sensor, ect...
//
//RECOLOR COLOR

struct recolor_color
{
	float value;	//Value to apply this color
	float blend;	//Blending factor to use, if applicable
	float r,g,b,a;	//RGBA values

	inline bool operator <( const recolor_color & old ) const { return value < old.value; }
	inline bool operator <( const float & cvalue ) const { return value < cvalue; }
};

class recolor_palette
{
	public:

		inline float to_normalized( float value )//Convert a value to normalized range 0.0...1.0
		{
			return (value - m_curr_min) / (m_curr_max - m_curr_min);
		}
		inline float from_normalized( float value )//Convert a normalized range to a value
		{
			return value*(m_curr_max - m_curr_min) + m_curr_min;
		}
		const recolor_color & get( int index ) const//Get the color at index (must exist)
		{
			return m_col_colors[index];
		}
		int size() const//Get the number of colors
		{
			return m_col_colors.size();
		}

		void recalc()//Make sure all values are available, resorts + fixes current color array
		{
			if( m_col_colors.size() > 0 ){

				//Make sure this is all ready
				//m_normalized.resize( m_col_colors.size() );

				//Resort colors
				std::sort( m_col_colors.begin(), m_col_colors.end() );

				m_curr_min = m_col_colors[0].value;
				m_curr_max = m_col_colors[0].value;

				std::vector< recolor_color >::iterator itr = m_col_colors.begin();
				std::vector< recolor_color >::iterator itr_end = m_col_colors.end();
				while( itr != itr_end ){

					if( itr->value < m_curr_min ){
						m_curr_min = itr->value;
					}
					if( itr->value > m_curr_max ){
						m_curr_max = itr->value;
					}
					itr++;
				}

				if( m_curr_min < m_curr_max ){

				}else{

					//Avoid problems by cheating
					m_curr_max = m_curr_min + 1.0;
				}

				/*
				if( m_curr_min < m_curr_max ){

					//Calculate normalized range
					int idex = 0;
					itr = m_col_colors.begin();
					while( itr != itr_end ){

						m_normalized[ idex ] = to_normalized( itr->value );

						itr++;
					}
				}else{

					int idex = 0;
					itr = m_col_colors.begin();
					while( itr != itr_end ){
						m_normalized[ idex ] = 0;
						itr++;
					}

					//Avoid problems by cheating
					m_curr_max = m_curr_min + 1.0;
				}
				*/

				m_dirty = 0;
			}
		}

		int remove( int index )//Remove the color at index
		{
			if( (index >=0) && (index < m_col_colors.size()) ){
				m_dirty = 1;
				m_col_colors.erase( m_col_colors.begin() + index );
				return 1;
			}
			return 0;
		}

		int nearest( float value ) const//Get the index of the color nearest to this value
		{
			//Returns the index of the 'nearest' color (half ranges)
			//recolor_color dummy; dummy.value = value;
			std::vector< recolor_color >::const_iterator  itr = std::lower_bound( m_col_colors.begin(), m_col_colors.end(), value );

			//Nearest uses half-range to next value:
			if( itr == m_col_colors.begin() ){
				return 0;
			}else if( itr == m_col_colors.end() ){
				return m_col_colors.size() - 1;
			}else{

				float xmax = itr->value;
				float xmin = (itr - 1)->value;
				float xdel = xmax - xmin;
				if( xdel != 0 ){
					if( ((value - xmin)/xdel) > 0.5 ){
						return itr - m_col_colors.begin();
					}
				}
				return (itr-1) - m_col_colors.begin();
			}
		}

		int lower_bound( float value ) const
		{
			std::vector< recolor_color >::const_iterator  itr = std::lower_bound( m_col_colors.begin(), m_col_colors.end(), value );
			if( itr == m_col_colors.end() ){
				return (itr - m_col_colors.begin()) - 1;
			}
			return itr - m_col_colors.begin();
		}

		int move( int index, float newvalue )//Move an existing color (by value), Requires recalc after you call this
		{
			if( (index >=0) && (index < m_col_colors.size()) ){

				m_col_colors[ index ].value = newvalue;

				m_dirty = 1;

				return 1;
			}
			return 0;
		}

		int insert( float value )//Insert a new color at value, return it's index. Will not create one if value is the same.
		{
			int before = lower_bound( value );
			if( before < m_col_colors.size() ){
				m_col_colors.insert( m_col_colors.begin() + before, recolor_color() );
				m_col_colors[ before ].value = value;
				m_col_colors.back().blend = 0.0;	//Linear
				return before;
			}else{
				m_col_colors.push_back( recolor_color() );
				m_col_colors.back().value = value;
				m_col_colors.back().blend = 0.0;	//Linear
				return m_col_colors.size()-1;
			}
		}
		int get( float value, float & R, float & G , float & B, float & A ) const//Get the RGBA at "value"
		{
			if( !m_col_colors.empty() ){

				if( value < m_curr_min ){
					//Clamp at begin?
					//Wrap? Hm. Disabled for now.
					const recolor_color & C0 = m_col_colors.front();
					R = C0.r;
					G = C0.g;
					B = C0.b;
					A = C0.a;
				}else if( value > m_curr_max ){
					//Clamp at end?
					//Wrap? Hm. Disabled for now.
					const recolor_color & C0 = m_col_colors.back();
					R = C0.r;
					G = C0.g;
					B = C0.b;
					A = C0.a;

				}else{

					int before = lower_bound( value );
					if( before > 0 ){
						float lmax = m_col_colors[ before ].value;
						float lmin = m_col_colors[ before - 1 ].value;
						float lblend = m_col_colors[ before - 1 ].blend;	//Use minimum (hm)
						float ldel = lmax - lmin;
						if( ldel != 0 ){

							float relQ = (value - lmin)/ldel;

							//Linear/clamp/exp blend equation?
							float blendQ = relQ;
							if( (lblend > 0.01) && (relQ > 0) ){

								float divy = exp( lblend ) - 1.0;
								if( (divy > -0.0001) && (divy < 0.0001) ){
									blendQ - (exp( lblend*relQ ) - 1.0) / divy;
								}
							}

							//blend = -inf..inf = ((exp( blend * X ) - 1)) / (exp( blend ) - 1)
							//if( blend < 0.01 ) -> linear (?)
							//Output range is always 0..1, but adjusting blend changes the shape of the blending eponent.
							//blend cannot be '1' however, which is a problem you'll have to fix.

							const recolor_color & C0 = m_col_colors[ before - 1 ];
							const recolor_color & C1 = m_col_colors[ before ];

							R = blendQ*(C1.r - C0.r) + C0.r;
							G = blendQ*(C1.g - C0.g) + C0.g;
							B = blendQ*(C1.b - C0.b) + C0.b;
							A = blendQ*(C1.a - C0.a) + C0.a;

						}else{

							const recolor_color & C0 = m_col_colors[ before - 1 ];
							R = C0.r;
							G = C0.g;
							B = C0.b;
							A = C0.a;
						}

					}else{

						const recolor_color & C0 = m_col_colors[ before ];
						R = C0.r;
						G = C0.g;
						B = C0.b;
						A = C0.a;
					}
				}
			}
			return 0;
		}

		inline void setColor( int idex, float R, float G, float B )
		{
			recolor_color & C = m_col_colors[idex];
			C.r = R;
			C.g = G;
			C.b = B;
		}
		inline void setColor( int idex, float R, float G, float B, float A )
		{
			recolor_color & C = m_col_colors[idex];
			C.r = R;
			C.g = G;
			C.b = B;
			C.a = A;
		}

		//inline void setBlend();

		bool is_dirty() const { return m_dirty; }

		void clear()
		{
			m_col_colors.clear();
			//m_normalized.clear();
		}

		std::string to_dxml( int tabdepth ) const
		{
			std::stringstream sendlx;
			sendlx<<std::endl;
			std::string sendl;
			sendl = sendlx.str();
			if( tabdepth > 250 ){ tabdepth = 250; }
			std::string tabby = "";
			for( int i = 0; i < tabdepth; i++ )
			{
				tabby += "\t";
			}
			std::string outme;
			outme += tabby + "<palette>" + sendl;
			std::string tabbyin = tabby;
			tabbyin += "\t";
			outme += tabbyin + "<name>" + m_name + "</name>" + sendl;
			outme += tabbyin + "<units>" + m_units + "</units>" + sendl;
			for( int i = 0; i < m_col_colors.size(); i++ )
			{
				outme += tabbyin + "<entry>";

				const recolor_color & C = m_col_colors[i];
				std::stringstream cox;
				cox.precision( 6 );
				cox<<C.value<<" "<<C.r<<" "<<C.g<<" "<<C.b<<" "<<C.a;
				outme += cox.str();
				outme += "</entry>" + sendl;
			}
			outme += tabby + "</palette>" + sendl;
			return outme;
		}

		int from_dxml( const uchar * data, uint datalength )
		{
			//Very unstrict parsing. Good enough.

			m_col_colors.clear();
			m_name = "";

			int found_palette = 0;

			std::string tagdata = "";

			const uchar * p = data;
			const uchar * p_final = data + datalength;
			while( p != p_final ){

				if( *p == '<' ){

					std::string tagtype;
					int endtag = 0;

					p++;
					while( p != p_final ){

						if( *p == '>' ){

							//What kind of tag is it?
							p++;
							break;
						}else{
							tagtype += *p;
							p++;
						}
					}

					if( p != p_final ){

						if( tagtype.size() > 0 ){

							if( tagtype[0] == '/' ){
								endtag = 1;
								tagtype.erase( tagtype.begin() );
							}

							if( (tagtype.size() == 7) && (tagtype == "palette") ){

								if( !endtag ){

									if( found_palette == 0 ){

										//Excellent, found the first palette tag.
										found_palette = 1;

									}else{

										return -13;	//Invalid formatting!
									}
								}else{

									if( found_palette == 1 ){

										recalc();

										return p - data;	//Perfect! Return bytes read. Done.
									}else{

										return -5;	//palette end found before begin
									}
								}

							}else if( (tagtype.size() == 5) && (tagtype == "entry") ){

								if( found_palette == 1 ){

									if( endtag ){

										//Parse tag data.
										if( tagdata.size() > 0 ){

											float A=0,B=0,C=0,D=0,E=0,F=0;
											//lols? Hm.
											const uchar * strstart = (uchar *)&tagdata[0];
											std::vector< uint > spits = string_calc_splits( (const uchar*)tagdata.c_str(), tagdata.size(), " " );
											if( spits.size() > 0 ){
												//strstart;
												if( spits.size() > 1 ){
													//strstart
													if( spits.size() > 2 ){
														//strstart
														if( spits.size() > 3 ){
															//strstart
															if( spits.size() > 4 ){
																//strstart
															}
														}
													}
												}
											}

											sscanf( tagdata.c_str(), "%f %f %f %f %f %f", &A,&B,&C,&D,&E,&F );

											int reti = insert( A );
											recolor_color & CEX = m_col_colors[ reti ];
											CEX.r = B;
											CEX.g = C;
											CEX.b = D;
											CEX.a = E;
											//CEX.value = A;
											CEX.blend = 0.0;
										}else{

											return -8;	//Invalid data for entry tag!
										}
									}

								}else{

									return -12;	//Invalid formatting!
								}

							}else if( (tagtype.size() == 4) && (tagtype == "name") ){

								if( found_palette == 1 ){

									if( endtag ){

										//Parse tag data.
										m_name = tagdata;
									}

								}else{

									return -12;	//Invalid formatting!
								}
							}else if( (tagtype.size() == 5) && (tagtype == "units") ){

								if( found_palette == 1 ){

									if( endtag ){

										//Parse tag data.
										m_units = tagdata;
									}

								}else{

									return -18;	//Invalid formatting!
								}
							}else{

								bool ignore = 0;
								if( tagtype.size() > 4 )
								{
									if( (tagtype[0] =='!') && (tagtype[1] =='-') && (tagtype[2] =='-') )
									{
										if( (*(tagtype.end() - 1) == '-') && (*(tagtype.end() - 2) == '-') )
										{
											ignore = 1;//Just a comment. Ignore it.
										}
									}
								}

								if( !ignore ){

									return -14;	//Unknown tag!
								}
							}
						}else{

							return -3;//Invalid tag name, too short!
						}
					}

					//Clear out data.
					tagdata = "";

				}else{

					tagdata += *p;

					p++;
				}
			}
			return -1;
		}

		float getMin() const { return m_curr_min; }
		float getMax() const { return m_curr_max; }

	private:

		float m_curr_min;
		float m_curr_max;
		int m_dirty;

		std::vector< recolor_color > m_col_colors;
		//std::vector< float > m_normalized;

	public:

		std::string m_name;
		std::string m_units;
};

struct recolor_thread_data
{
	enum{
		CLAMP_BEFORE = 1<<0,	//Color before this one is identical to this one
		CLAMP_AFTER = 1<<1,		//Color after this one is identical to this one
	};

	int active;
	int currprogress_max;
	int currprogress;

	int method;				//Coloring method
	//int col_palette;		//Select a palette
	//float col_smoothing;	//Select smoothing method
	float range_scale;	//Ranging scale (adjust conversion to palette)
	float range_shift;	//Ranging shift, adjust offset conversion to palette.
	float feedback_range_at_zero;
	float feedback_range_at_one;
	int feedback_units_id;	//! dangerous. since it's NOT atomic, read/writes can be split thread!
	float col_alpha;		//Recolor with ALPHA. But, this defaults to 1.0, and the palette contains alpha anyways. If this is NOT 1.0, use blanking alpha.
	int col_flags;

	recolor_palette col_palette;	//Ther REAL palette data.

	//??
	std::vector< uint > copied_view_list;	//Must create a copy as soon as possible.
	const cdr::airport_hash * cdr_airports;
	const std::vector< uint > * track_time_data;
	std::vector< float > * track_data;
	const std::vector< tracksplit > * track_splits;
	const std::vector< tracksplit > * non_track_splits;
	void * selfptr;
	CDRVer * CDRP;
};

struct intrail_computation_thread_data
{
	int active;				//Required
	int currprogress_max;	//Required
	int currprogress;		//Required

	std::vector< uint > copied_view_list;	//HM. Better.
	const cdr::airport_hash * cdr_airports;		//Airports to check against
	const std::vector< uint > * track_time_data;	//Time data!
	const std::vector< float > * track_data;				//Track data!
	std::vector< tracksplit > * track_splits;	//The splits of them!
	const std::map< int, std::string > * sensor_id_map;	//Required for sensor names

	std::string output_filename;
	CDRVer * CDRP;
};

//tracksplit
struct tracksubsect
{
	uint begin;	//Start point (INSIDE of time range)
	uint end;	//End point		(INSIDE of time range)
	uint tbegin;//Start time LOCAL to track
	uint tend;	//End time LOCAL to track
	uint trackid;	//Points to tracksplit -> tracksplitkey
	uint flags;	//Flags about this subsection
};

//Subselection group (because there are more than 1 possible)
struct tracksubselectgroup
{
	uint planeid;
	std::string name;
	std::vector< tracksubsect > selected;
	std::vector< vec3 > hitpoints;	//Shows the intersection points.
};

struct timechain_thread_data
{
	int active;
	int currprogress_max;
	int currprogress;

	const std::vector< float > * track_data;	//Track data!
	const std::vector< uint > * track_time_data;	//Time data!
	const std::vector< tracksplit > * track_splits;	//The splits of them!
	//const IndexableArray< d_manuplane > * planedata;	//Planes to subselect against (note each plane stores a selected flag)
	const std::vector< d_manuplane > * planedata;	//Planes to subselect against (note each plane stores a selected flag)
	std::vector< uint > copied_view_list;
	std::vector< tracksubselectgroup > subselections;	//New selection groups
	//const std::map< int, std::string > * sensor_id_map;	//Required for sensor names
	//const cdr::airport_hash * cdr_airports;		//Airports to check against
	CDRVer * CDRP;
};

struct dted_request{
	int minlongx, minlaty;	//Requests for PATCHES (minute patches)
	dted_request() {}
	dted_request( const dted_request & old ) : minlongx( old.minlongx ), minlaty( old.minlaty ) {}
	bool operator==( const dted_request & old ) const { return (minlongx == old.minlongx) && (minlaty == old.minlaty); }
};

////////////////////////////////////////////////
//

//okay... this is crap.

//Need to redo it, so that:
//
//1. All units are in ms
//2. A patch can only cover a max of 60x60 seconds (1 minutex1minute)
//3. ALL UNITS are defined:
//	W000 = 0 E000 = 180 E180 = 360	//IE instead of NEGATIVE west values, use 0 as westmost, +360 as eastmost
//	S000 = 0 N000 = 90 N090 = 180	//IE instead of NEGATIVE south values, use 0 as south pole, 180 as north pole.
//	This will keep "patch" bounds consistent.
//4. Use cache of patches
//

class dted_patch{	//Is a struct, because it only stores atomic types. But behaves like a class...

	public:

	//A (1/60)x(1/60) degree (minute) patch (max of 60x60 entries => 3600 points)
	union{
		short * data;		//(8) 64/32 bit compatible pointer
		uchar data_ptr[8];
	};
	uint x_second;	//(4) 360*60*60 => total seconds on earth (21600), x_second/3600 = long = W180, 360 = E180
	uint y_second;	//(4) 360*60*60 => total seconds on earth (21600), y_second/3600 = lat = S90, 180 = N90
	ushort x_second_size;	//(4) Up to 1 minute max (60 extra seconds) 0 by default is a 1x1 second square, 60 = 60 seconds patch (full DTED2)
	ushort y_second_size;	//(4) Up to 1 minute max (60 extra seconds) 0 by default is a 1x1 second square, 60 = 60 seconds patch (full DTED2)
	ushort x_size;	//(2) Number of entries in x axis
	ushort y_size;	//(2) Number of entries in y axis
	short min_m;	//(2) DTED units are METERS. Fun note, tallest structure on earth isn't more than 28,000 ft, so +-32000ft fits perfectly; DTED spec says nothing more than 9000m exists.
	short max_m;	//(2)

	public:

	//Works like a class.
	dted_patch() : data(0), x_second(0), y_second(0), x_second_size(0), y_second_size(0), x_size(0), y_size(0) {}
	~dted_patch() { dataClear(); }
	dted_patch( const dted_patch & old ){
		if( this != &old ){
			x_second = old.x_second;	//(4) 360*60*60 => total seconds on earth (21600), x_second/3600 = long (- = W, + = E)
			y_second = old.y_second;	//(4) 360*60*60 => total seconds on earth (21600), y_second/3600 = lat (- = S, + = N)
			x_second_size = old.x_second_size;	//(1) Up to 1 minute max (60 extra seconds) 0 by default is a 1x1 second square
			y_second_size = old.y_second_size;	//(1) Up to 1 minute max (60 extra seconds) 0 by default is a 1x1 second square
			x_size = old.x_size;	//(1) Number of entries in x axis (max +128? IE 1/2 second max resolution.)
			y_size = old.y_size;	//(1) Number of entries in y axis (max +128? IE 1/2 second max resolution.)
			min_m = old.min_m;	//(2) DTED units are ft. Fun note, tallest structure on earth isn't more than 28,000 ft, so +-32000 fits perfectly.
			max_m = old.max_m;	//(2)
			data = old.data;	//TAKE data.
			const_cast< dted_patch* >( &old )->data = 0;
		}
	}
	dted_patch & operator=( const dted_patch & old )
	{
		x_second = old.x_second;	//(4) 360*60*60 => total seconds on earth (21600), x_second/3600 = long (- = W, + = E)
		y_second = old.y_second;	//(4) 360*60*60 => total seconds on earth (21600), y_second/3600 = lat (- = S, + = N)
		x_second_size = old.x_second_size;	//(1) Up to 1 minute max (60 extra seconds) 0 by default is a 1x1 second square
		y_second_size = old.y_second_size;	//(1) Up to 1 minute max (60 extra seconds) 0 by default is a 1x1 second square
		x_size = old.x_size;	//(1) Number of entries in x axis (max +128? IE 1/2 second max resolution.)
		y_size = old.y_size;	//(1) Number of entries in y axis (max +128? IE 1/2 second max resolution.)
		min_m = old.min_m;	//(2) DTED units are ft. Fun note, tallest structure on earth isn't more than 28,000 ft, so +-32000 fits perfectly.
		max_m = old.max_m;	//(2)
		dataAlloc( x_size, y_size );	//COPY data
		memcpy( data, old.data, x_size*y_size*2 );
		return *this;
	}

	void dataClear(){
		if( data !=0 ){ delete [] data; } x_size = 0; y_size = 0;
	}
	void dataAlloc( ushort x, ushort y, ushort extras=0 )
	{
		if( (x == x_size) && (y == y_size) ){
			if( data != 0 ){
				//Done.
			}else{
				dataClear();
				x_size = x;
				y_size = y;
				data = new short[x_size*y_size + extras];	//Wipe out?
			}
		}else{
			dataClear();
			x_size = x;	//(1) Number of entries in x axis (max +128? IE 1/2 second max resolution.)
			y_size = y;	//(1) Number of entries in y axis (max +128? IE 1/2 second max resolution.)
			data = new short[x_size*y_size + extras];
		}
	}

	bool toLocal( int mslongx, int mslaty, float & idx, float & idy )//Returns true if valid coordinte (ms in lat/long), output is local coordinates to tile
	{
		//Local index MAY NOT be as you think it should...
		int cmsx = mslongx / 1000;	//ms -> second
		int cmsy = mslaty / 1000;
		if( ( cmsx >= x_second ) && ( cmsx < (x_second + x_second_size) ) ){
		//if( ( cmsx < x_second ) && ( cmsx >= (x_second - x_second_size) ) ){
			if( ( cmsy >= y_second ) && ( cmsy < (y_second + y_second_size) ) ){
				idx = ((double)((mslongx - (x_second*1000)) ))/((double)(x_second_size*1000));
				//idx = ((float)((mslongx - ((x_second-x_second_size)*1000)) ))/((float)(x_second_size*1000));
				idy = ((double)((mslaty - (y_second*1000)) ))/((double)(y_second_size*1000));
				idx *= x_size;
				idy *= y_size;
				return true;
			}
		}
		return false;
	}

	inline bool valid()
	{
		return data != 0 && ( x_size > 0 ) && ( y_size > 0 );
	}

	inline int getHeightm( short idx, short idy )	//NO ERROR CHECKING, Get a (nearest) interpolated point at a specific local index in x_size/y_size
	{
		return data[ idx + ((int)idy)*x_size ];	//DATA index only.
	}
	inline int getHeightLinearm( float idx, float idy )	//NO ERROR CHECKING, in LOCAL coordinates (inside of x_size/y_size bounds, 0 <= idx < x_size, 0 <= idy < y_size); NO overrun checking
	{
		//Hm. idx/idy are in LOCAL index coordinates.
		int x0 = (int)idx;	//IF they are 1.0, not allowed. [0.0 .. 1.0)
		int y0 = (int)idy;
		int x1 = x0 + 1;
		int y1 = y0 + 1;
		int id0 = x0 + y0*x_size;
		float A = data[ id0 ];
		float B = data[ id0 + 1 ];
		id0 += x_size;
		float C = data[ id0 ];
		float D = data[ id0 + 1 ];
		float x = (idx - x0);
		float y = (idy - y0);

		return y*( x*( D - B - C + A ) + B - A ) + ( x*(C - A) + A );
		//Linear blend 2D:
		//
		//Add together Using X-Bias = Add together Using Y-Bias (V = Xf*(Yint1 - Yint0) + Yint0)
		//
		//Xint0 = x*(C - A) + A;	//Use X-biased equations (Y ones are same result)
		//Xint1 = x*(D - B) + B;
		//
		//V = y*(Xint1 - Xint0) + Xint0;	//add together!
		//
	}
};

struct dted_thread_data
{
	int active;				//set to 0 to quit
	int currprogress_max;	//?
	int currprogress;		//?
	int isdisabled;

	int maxpatches;
	int locked;	//Mutex lock (atomic)
	bool whichrqcache;	//if 0, WRITE to 0. if 1, WRITE to 1.	//Current cache to write to
	std::string dtedpaths[3];
	std::deque< dted_request > requestcache0;
	std::deque< dted_request > requestcache1;
	std::deque< dted_patch * > patches;

	//Asychronous cache
	int request_curr_longxmin;
	int request_curr_latymin;
	int request_curr_status;
	int request_x_size;
	int request_y_size;
	int request_read_longxmin;
	int request_read_latymin;
	std::vector< int > request_data;	//ECEF xyz coordinates (for accuracy)
	std::vector< uchar > readcache;
	CDRVer * CDRP;

	//std::deque< dted_vertex > vertpatch;	//Can request a building of a DTED vertex patch (longxms, latyms, altft) so that you can be somewhat guaranteed this will occur safely.

	//Returns patch pointer if it exists; otherwise 0, AND requests it. mslongx / mslaty obey unsigned convention (0 = W180, 360 = E180, 0 = S90, 180 = N90)
	dted_patch * _hasPatch( int mslongx, int mslaty, float & idx, float &idy )	//MUST ALREADY BE LOCKED
	{
		dted_patch * validpatch = 0;
		//if( locked == 1 ){	//MUST ALREADY BE LOCKED
		//if( locked == 0 ){
		//	locked = 1;

			//Find a specific patch (hm)
			std::deque< dted_patch * >::iterator  ip_begin = patches.begin();
			std::deque< dted_patch * >::iterator  ip_end = patches.end();
			std::deque< dted_patch * >::iterator  ip = ip_begin;
			while( ip != ip_end ){
				if( *ip != 0 ){
					if( (*ip)->toLocal( mslongx, mslaty, idx, idy ) ){
						dted_patch * vp = *ip;
						if( ip != ip_begin ){
							patches.erase( ip );
							patches.push_front( vp );	//Make it a cache (most used things to the FRONT of cache.
						}
						validpatch = vp;
						break;
					}
				}
				ip++;
			}

			if( validpatch == 0 ){

				//REQUEST a new patch. (hm)
				dted_request RQ;
				RQ.minlongx = mslongx / (1000*60);	//ms -> minute
				RQ.minlaty = mslaty / (1000*60);	//ms -> minute
				if( whichrqcache ){
					int hasit = 0;
					std::deque< dted_request >::iterator  iq = requestcache1.begin();
					std::deque< dted_request >::iterator  iq_end = requestcache1.end();
					while( iq != iq_end ){
						if( RQ == *iq ){
							hasit = 1;
							break;
						}
						iq++;
					}
					if( !hasit ){
						requestcache1.push_front( RQ );
					}
				}else{
					int hasit = 0;
					std::deque< dted_request >::iterator  iq = requestcache0.begin();
					std::deque< dted_request >::iterator  iq_end = requestcache0.end();
					while( iq != iq_end ){
						if( RQ == *iq ){
							hasit = 1;
							break;
						}
						iq++;
					}
					if( !hasit ){
						requestcache0.push_front( RQ );
					}
				}
			}
		//	locked = 0;
		//}
		//}
		return validpatch;
	}

	/*
	//Sidenote; to generate patch meshes, use SECONDS. DTED 2 is only accurate to the arc second, so querying a ring of those arc seconds would be best.
	bool getHeightft( int mslongx, int mslaty, int & heit )	//MUST ALREADY BE LOCKED //Returns true if this height exists (automatically requests patches as needed)
	{
		//Does this patch exist?
		//if( locked == 1 ){	//MUST ALREADY BE LOCKED
			float idx, idy;
			dted_patch * hasp = _hasPatch( mslongx, mslaty, idx, idy );
			if( hasp != 0 ){
				heit = hasp->getHeightft( idx, idy );
				return true;
			}
		//}
		return false;
	}
	bool getHeightLinearft( int mslongx, int mslaty, int & heit ) //MUST ALREADY BE LOCKED //Returns true if we have the height (automatically requests patches as needed)
	{
		//if( locked == 1 ){	//MUST ALREADY BE LOCKED
			float idx, idy;
			dted_patch * hasp = _hasPatch( mslongx, mslaty, idx, idy );
			if( hasp != 0 ){
				heit = hasp->getHeightLinearft( idx, idy );
				return true;
			}
		//}
		return false;
	}

	inline dted_patch * getPatch( int mslongx, int mslaty, float & idx, float & idy ){
		//if( locked == 1 ){	//MUST ALREADY BE LOCKED
			return _hasPatch( mslongx, mslaty, idx, idy );
		//}
		//return 0;
	}

	template< typename _T >
	inline dted_patch * getPatch( const _T & x, const _T & y, const _T & z, int & longx, int & laty, float & idx, float & idy ){
		//if( locked == 1 ){	//MUST ALREADY BE LOCKED

			double cx = x;
			double cy = y;
			double cz = z;
			double cm = sqrt(cx*cx + cy*cy +cz*cz);
			cx /= cm;
			cy /= cm;
			cz /= cm;

			//cdr::XYZToSphere<double>( cx, cy, cz, theta, phi ); -> made more efficient:
			laty = (int)(( (acos( cz ) - M_PI_2) )* 3600000.0 * M_RTD);
			longx = -(int)(( (atan2( cy, cx )) )* 3600000.0* M_RTD);
			return _hasPatch( longx, laty, idx, idy );
		//}
		//return 0;
	}

	//Given input XYZ nmi, request the height (AGL) under me (hm) expensive, only works in this version of CDRViewer. Also converts xyz into lat long.
	template< typename _T >
	bool getHeightLinearft( const _T & x, const _T & y, const _T & z, int & heit, int & longx, int & laty )
	{
		//if( locked == 1 ){	//MUST ALREADY BE LOCKED
			double cx = x;
			double cy = y;
			double cz = z;
			double cm = sqrt(cx*cx + cy*cy +cz*cz);
			cx /= cm;
			cy /= cm;
			cz /= cm;

			//cdr::XYZToSphere<double>( cx, cy, cz, theta, phi ); -> made more efficient:
			laty = (int)(( (acos( cz ) - M_PI_2) )* 3600000.0 * M_RTD);
			longx = -(int)(( (atan2( cy, cx )) )* 3600000.0* M_RTD);

			float idx, idy;
			dted_patch * hasp = _hasPatch( longx, laty, idx, idy );
			if( hasp != 0 ){
				heit = hasp->getHeightLinearft( idx, idy );
				return true;
			}
		//}
		return false;
	}

	template< typename _T >
	bool getHeightft( const _T & x, const _T & y, const _T & z, int & heit, int & longx, int & laty )
	{
		//if( locked == 1 ){	//MUST ALREADY BE LOCKED
			double cx = x;
			double cy = y;
			double cz = z;
			double cm = sqrt(cx*cx + cy*cy +cz*cz);
			cx /= cm;
			cy /= cm;
			cz /= cm;

			//cdr::XYZToSphere<double>( cx, cy, cz, theta, phi ); -> made more efficient:
			laty = (int)(( (acos( cz ) - M_PI_2) )* 3600000.0 * M_RTD);
			longx = -(int)(( (atan2( cy, cx )) )* 3600000.0* M_RTD);

			float idx, idy;
			dted_patch * hasp = _hasPatch( longx, laty, idx, idy );
			if( hasp != 0 ){
				heit = hasp->getHeightft( idx, idy );
				return true;
			}
		//}
		return false;
	}
	*/

	//This method allows you to draw with patches more efficiently. (IE you can make a linear ++ loop against minutes; and draw patches that way.)
	inline dted_patch * getPatchDirect( int minlongx, int minlaty ){
		float idx, idy;

		//Upconvert range? Hm. (we expect correct input values.)
		//minlongx += 10800;	//60*180
		//minlaty += 5400;	//60*90

		return _hasPatch( minlongx*1000*60, minlaty*1000*60, idx, idy );
	}

	//minx / miny obey unsigned convention (0 = W180, 360 = E180, 0 = S90, 180 = N90)
	std::string patchFileConvert( int minx, int miny, int dtedl ) const
	{
		char bax[32];

		//There is an issue with patches near degree boundaries.

		int absminx = minx;
		int iswest = 0;
		if( minx >= 180*60 ){
			//"east"
			absminx = (minx - 180*60)/60;
		}else{
			//"west" (invert)
			absminx = (180*60 - minx);
			if( absminx % 60 != 0 ){
				absminx /= 60;
				absminx += 1;
			}else{
				absminx /= 60;
			}
			iswest = 1;
		}

		//if( absminx < 0 ){ absminx = -absminx; }
		//if( minx < 0 ){ absminx += 60; }	//Because w001 exists, not w000 [0..-1) => w001 [-1..-2) => w002

		//absminx /= 60;
		int baxn = sprintf( bax, "%d", absminx );

		std::string baxs;
		if( baxn > 0 ){
			baxs.assign( bax, bax + baxn );
		}
		while( baxs.size() < 3 ){
			baxs = "0" + baxs;
		}
		if( iswest ){
			baxs = "w" + baxs;
		}else{
			baxs = "e" + baxs;
		}

		char bay[32];

		int absminy = miny;
		int issouth = 0;
		if( miny >= 90*60 ){
			//"north"
			absminy = (miny - 90*60)/60;
		}else{
			//"south" (invert)
			absminy = (90*60 - miny);
			if( absminy % 60 != 0 ){
				absminy /= 60;
				absminy += 1;
			}else{
				absminy /= 60;
			}
			issouth = 1;
		}

		//if( absminy < 0 ){ absminy = -absminy; }
		//if( miny < 0 ){ absminy += 60; }
		//absminy /= 60;
		int bayn = sprintf( bay, "%d", absminy );

		std::string bays;
		if( bayn > 0 ){
			bays.assign( bay, bay + bayn );
		}
		while( bays.size() < 2 ){
			bays = "0" + bays;
		}
		if( issouth ){//miny < 0 ){
			bays = "s" + bays;
		}else{
			bays = "n" + bays;
		}

		baxs += "/" + bays + ".dt";
		baxs += (dtedl + '0');
		return baxs;
	}

	/*
	std::string patchFilePath( int minx, int miny, int dtedl ) const
	{
		char bax[8];

		int absminx = minx;
		if( absminx < 0 ){ absminx = -absminx; }
		if( minx < 0 ){ absminx += 1; }	//Because w001 exists, not w000 [0..-1) => w001 [-1..-2) => w002
		int baxn = sprintf( bax, "%d", absminx );

		std::string baxs;
		baxs.assign( bax, bax + baxn );
		while( baxs.size() < 3 ){
			baxs = "0" + baxs;
		}
		if( minx < 0 ){
			baxs = "w" + baxs;
		}else{
			baxs = "e" + baxs;
		}

		char bay[8];
		int absminy = miny;
		if( absminy < 0 ){ absminy = -absminy; }
		if( miny < 0 ){ absminy += 1; }
		int bayn = sprintf( bay, "%d", absminy );
		std::string bays;
		bays.assign( bay, bay + bayn );
		while( bays.size() < 2 ){
			bays = "0" + bays;
		}
		if( miny < 0 ){
			bays = "s" + bays;
		}else{
			bays = "n" + bays;
		}

		baxs += "/" + bays + ".dt";
		baxs += (dtedl + '0');
		return baxs;
	}
	*/

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

	//Expect request to be in... SIGNED coordinates.
	int requestPatch( int longxmin, int latymin )
	{
		//Convert
		if( longxmin < 0 ){
			longxmin += 180*60;//10800;
			//longxmin += 1;	//WHY? cause dted tiles reverse?
			longxmin -= 1;
		}else{
			longxmin += 180*60;//10800;
		}
		if( latymin < 0 ){
			latymin += 90*60;//5400;
			//latymin += 1;
		}else{
			latymin += 90*60;//5400;
		}

		if( (request_curr_longxmin == longxmin) && (request_curr_latymin == latymin) ){
			if( request_curr_status > 0 ){
				return request_curr_status;	//Thread will no longer touch the data.
			}else{
				return request_curr_status;	//Error or failed occured. (negative is error IE does not exist.)
			}
		}else{
			if( request_curr_status != 0 ){
				request_curr_longxmin = longxmin;	//lols?
				request_curr_latymin = latymin;		//
			}
		}
		return 0;
	}
};


//A single hit of weatherstation data. Includes the TIME this data applies, in yearmonths. (1 year == 12 months)
struct weatherstationdata
{
	uint m_time;	//yearmonth
	uint m_id;
	uint m_wmonum;
	int m_y_lat_ms;		//latitude in ms
	int m_x_long_ms;	//longitude in ms
	int m_groundheight_ft;	//reported height of ground
	int m_stationheight_ft;	//reported station height (height above ground?)
	int m_baroheight_ft;	//reported current barometric height (pressure altimeter)
	int m_timezone;		//Timezone this station is in

	std::string callsign;	//callsign for this weather station
	//?	Climate Division Code (int)|Climate Division State Code (int)|Climate Division Station Code (int)
	std::string name;		//logical name
	std::string location;	//location (sometimes last two chars are state code)
	std::string address;	//address (sometimes the mailing address

	//YearMonth|WBAN Number|WMO Number|Call Sign|Climate Division Code|Climate Division State Code|Climate Division Station Code|Name|Location|Address|Latitude|Longitude|Ground Height|Station Height|Barometer Height|Time Zone
	//1996-07|24237|72781|SMP|05|45|8009|STAMPEDE PASS, WA|TOP OF CASCADE MTNS.|WSCMO STAMPEDE PASS*P.O. BOX 128*EASTON, WA. 98925*|47.17|-121.2|3964|3964|3964|
	//2003-03|24237|72781|SMP|05|45|8009|STAMPEDE PASS, WA|STAMPASS PASS FLTWO|WSCMO STAMPEDE PASS*P.O. BOX 128*EASTON, WA. 98925*|47.17|-121.2|3961|3967|3964|+8
	//2007-05|24237|72781|SMP|05|45|8009|STAMPEDE PASS|WA|STAMPASS PASS FLTWO|47.293|-121.337|3973|3967|3964|-8
	//2012-05|24237||SMP|05|45|8009|STAMPEDE PASS|WA|STAMPASS PASS FLTWO|47.2767|-121.3372|3959|3967|3964|-8
	//

};

//A single hit of weather. Includes a lot, and the month minute this hir occured at.
//Store an array of these when you load in a weather hits file. (But use the cached version.)
struct weatherhit	//Each hit is: 32 bytes? => 2 x 16 byte vectors
{
	//(4)
	ushort m_time;	//Minute from start of month! (easy to calculate day this way. 1 day == 1440 minutes, 32 days = 46080 minutes. which is less than 65535 minutes.
	short m_precipitation;	//HourlyPrecip HourlyPrecipFlag in 1/100th inches

	//(8)
	uint m_skycond_mapped;		//SkyCondition	SkyConditionFlag	-> map index to string (not that many unique strings?)
	uint m_weathertype_mapped;	//WeatherType	WeatherTypeFlag		-> map index to string (not that many unique strings?)

	//(4)
	uchar m_visibility_thenthsnmi;	//Visibility VisibilityFlag in 1/10th nmi statuate	(up to 25.5 max!)
	uchar m_windspeed_knots;	//knots	WindSpeed	WindSpeedFlag	(-128 = flag, 0..127 knots)
	uchar m_winddir_deg;	//WindDirection	WindDirectionFlag wind direction, in heading/2 degrees (each value is 2 degrees, so 90 == 45.)
	char m_windchar;		//ValueForWindCharacterValueForWindCharacterFlag wind character value, -128 for none

	//DryBulbFarenheit	DryBulbFarenheitFlag	DryBulbCelsius	DryBulbCelsiusFlag	WetBulbFarenheit	WetBulbFarenheitFlag	WetBulbCelsius	WetBulbCelsiusFlag	DewPointFarenheit	DewPointFarenheitFlag	DewPointCelsius	DewPointCelsiusFlag	RelativeHumidity	RelativeHumidityFlag
	//(4)
	char m_dry_bulb_fah;	//Dry Bulb temp (-127..127 fahrenheit;	-128 == error code? )
	char m_wet_bulb_fah;	//Wet Bulb temp (-127..127 fahrenheit;	-128 == error code? )
	char m_dew_point_fah;	//Dew point temp (-127..127 fahrenheit;	-128 == error code? )
	char m_rel_humidity;	//Relative huminity (-100..100;	-128 == error code? )

	//(8)
	short m_pressure_station;	//StationPressure	StationPressureFlag Pressure at station in 100* baro (?)
	short m_pressure_sea;		//SeaLevelPressure SeaLevelPressureFlag Pressure at sea level? in 100* baro (?)
	short m_pressure_altimiter;	//Altimeter AltimeterFlag Pressure of altimeter in 100* baro (?)
	char m_pressure_tendency;	//PressureTendency PressureTendencyFlag Pressure tendency units? hm. 0 by default.
	char m_pressure_change;		//PressureChange PressureChangeFlag Pressure change in ? hm. 0 by default.

	//(4)
	short m_recordtype;		//RecordType RecordTypeFlag	-> map index to string (not that many unique strings?)
	ushort m_stationindex;	//Which station is this? (stations have id's, this is the index for that id?)
};




struct weather_thread_data
{
	int active;				//set to 0 to quit
	int currprogress_max;	//?
	int currprogress;		//?
	int locked;

	bool whichrqcache;	//if 0, WRITE to 0. if 1, WRITE to 1.	//Current cache to write to
	std::string weatherpath;
	std::deque< dted_request > requestcache0;
	std::deque< dted_request > requestcache1;
	CDRVer * CDRP;

	//Weather zip stuff.

	//time in MINUTES; Weather accuracy is useless beyond that granularity. This will be an accurate date for a long time (JDN 40000+), except dates themselves are inaccurate without a table.
	//This is a contiguous time structure! Meaning there is NO GAP in the minutes, they are contiguous.
	struct wminute{

		int minutes;	//No epoch. See equations for conversion.

		inline int getMinute() const { return (minutes)%60; }
		inline int getHour() const { return (minutes/60)%24; }
		inline int getDayNumber() const { return (minutes/(60*24)); }
		inline bool getDate( int & Y, int & M, int & D ) const { return JToYMD( minutes/(60*24), Y, M, D ); }

		inline void setMinute( int newminute ) { minutes += newminute - (minutes % 60); }	//Replace minutes
		inline void setHour( int newhour ) { minutes += 60*(newhour - (minutes/60) % 12); }	//Replace hours
		inline void setDate( int year, int month, int day ) { minutes = minutes%(60*24) + YMDToJ( year, month, day )*60*24; }
		inline int setDayNumber( int dayn ) { minutes = minutes%(60*24) + dayn*60*24; return 0; }

		//Stupid calendrics: This is likely incorrect as all calendars are. But it's more accurate than anything YOU have.
		/*inline int YMDToN( const int & Y, const int & M, const int & D ) const	//M, D are expected in the 1..12/31 range.
		{
			int m = (M + 9)%12;
			int y = Y - M/10;
			return 365*y + y/4 - y/100 + y/400 + (m*306 + 5)/10 + (D-1);
		}

		inline bool NToYMD( const int & N, int & Y, int & M, int & D ) const	//M, D are returned in the 1..12/31 range.
		{
			Y = (10000*N + 14780)/3652425;
			int ddd = N - (365*Y + Y/4 - Y/100 + Y/400);
			if( ddd < 0 ){
				Y = Y - 1;
				ddd = N - (365*Y + Y/4 - Y/100 + Y/400);
			}
			int mi = (100*ddd + 52)/3060;
			M = (mi + 2)%12 + 1;
			Y = Y + (mi + 2)/12;
			D = ddd - (mi*306 + 5)/10 + 1;
			return 1;
		}*/

		//Given a GREGORIAN calender date (Y, M = 1..12, D = 1..31) return the julian day number.
		inline int YMDToJ( const int & Y, const int & M, const int & D ) const	//M, D are expected in the 1..12/31 range, J is returned in the 0.. +6300 range (~3200 AD)
		{
			int a = (14 - M) / 12;
			int y = Y + 4800 - a;
			int m = M + 12*a - 3;
			return D + (153*m + 2) /5 + 365*y + y/4 - y/100 + y/400 - 32045;	//if YMD are in Gregorian
			//return D + (153*m + 2) /5 + 365*y + y/4 - 32083;	//if Y M D are in Julian
			//Cool sidenote, julian day numner % 7 == day of the week, starting with monday. This can help you sanity check results.
		}

		//Given a Julian day number, set the Y, M, D to the gregorian day.
		inline bool JToYMD( const int & J, int & Y, int & M, int & D ) const	//M, D are returned in the 1..12/31 range.
		{
			int j = (J + 0.5) + 32044;	//Shift back a half day + back to year -4800
			int g = j / 146097;
			int dg = j % 146097;
			int c = (((dg / 36542 + 1)) * 3) / 4;
			int dc = dg - c * 36524;
			int b = dc / 1461;
			int db = dc % 1461;
			int a = ((db / 365 + 1) * 3) / 4;
			int da = db - a*365;
			int y = g*400 + c*100 + b*4 + a;	//integer # of full years elapsed since march 1 4801 BC 00:00 UTC)
			int m = (da*5 + 308) / 153 - 2;	//integer number of months since March 1
			int d = da - ((m+4)*153) / 5 + 122;	//integer number of days elapsed since day 1, including fractions of day
			Y = y - 4800 + (m+2) / 12;
			M = (m + 2) % 12 + 1;
			D = d + 1;
			return true;
		}

		inline void nextMinute()
		{
			int newminute = getMinute() + 1;
			if( newminute >= 60 ){
				int newhour = getHour() + 1;
				if( newhour >= 24 ){
					int newday = getDayNumber() + 1;
					setDayNumber( newday );
				}
			}
		}
		inline void prevMinute()
		{
			if( minutes > 0 ){
				int newminute = getMinute() - 1;
				if( newminute < 0 ){
					int newhour = getHour() - 1;
					if( newhour < 0 ){
						int newday = getDayNumber() - 1;
						setDayNumber( newday );
					}
				}
			}else{
				minutes = 0;
			}
		}

		inline void nextHour()
		{
			int newhour = getHour() + 1;
			if( newhour >= 24 ){
				int newday = getDayNumber() + 1;
				setDayNumber( newday );
			}
		}
		inline void prevHour()
		{
			if( minutes > 60 ){
				int newhour = getHour() - 1;
				if( newhour < 0 ){
					int newday = getDayNumber() - 1;
					setDayNumber( newday );
				}
			}else{
				minutes = 0;
			}
		}

		inline void nextDay()
		{
			minutes += 60*24;	//1 day added
			//int newday = getDayNumber() + 1;
			//setDayNumber( newday );
		}

		inline void prevDay()
		{
			if( minutes > 60*24 ){
				minutes -= 60*24;	//1 day subtracted
			}else{
				minutes = 0;
			}
			//int newday = getDayNumber() - 1;
			//setDayNumber( newday );
		}

		inline bool operator<( const wminute & B ) const { return minutes < B.minutes; }
		inline bool operator==( const wminute & B ) const { return minutes == B.minutes; }
	};

	void request( )
	{

	}

	bool get( int stationid, int year, int month, int day, int hour, int param )
	{


		return false;
	}

};

int getGEOrdindates( uint gx, uint gy, uint depth, double & xlong, double & ylat );
int getGETileCoords( double glongx, double glaty, uint depth, int & gx, int & gy );

struct googletterr_tile	//16
{
	//This is the AABB of the tile, but, as you know, mercator projections cause fanning, so it's not a box, but some other shape that always fits IN a box
	//short id;		//the external index of this tile (mapping)
	//short cacheid;	//The cache index of this texture
	//short gfxoffset;//Offset into coordinate array for this tile
	//short flags;	//Flags about this tile

	int googlex; 		//x Index in the 2d grid for this tile
	int googley;		//y Index in the 2d grid for this tile
	int googledepth;	//depth
	int flags;			//User flags (alignit)

	inline int set( double xlong, double ylat, uint ndepth )
	{
		googledepth = ndepth;
		getGETileCoords( xlong, ylat, googledepth, googlex, googley );
		return 0;
	}

	inline int getDepth(){ return googledepth; }

	inline void getMin( double & xlong, double & xlat )
	{
		//mercator -> long lat?
		getGEOrdindates( googlex, googley, googledepth, xlong, xlat );
	}
	inline void getMax( double & xlong, double & xlat )
	{
		getGEOrdindates( googlex + 1, googley + 1, googledepth, xlong, xlat );
	}

	inline void addOffset( int tileoffsetx, int tileoffsety )
	{
		googlex += tileoffsetx;
		googley += tileoffsety;
	}

	inline void depthUp()	//Zoom in
	{
		googlex *= 2;
		googley *= 2;
		googledepth++;
	}
	inline void depthDown()	//Zoom out
	{
		googlex /= 2;
		googley /= 2;
		googledepth--;
	}

	//int long_x;			//Actual longitude of this tile
	//int lat_y;			//Actual latitude of this tile
	//uint googledepth;	//Depth, aka 1 << depth = number of cells

	bool operator==( const googletterr_tile & old  ) const
	{
		return (googlex == old.googlex) && (googley == old.googley) && (googledepth==old.googledepth);
	}
	bool operator<( const googletterr_tile & old  ) const
	{
		if( googlex == old.googlex ){
			if( googley == old.googley ){
				if( googledepth < old.googledepth ){
					return true;
				}
			}else if( googley < old.googley ){
				return true;
			}
		}else if( googlex < old.googlex ){
			return true;
		}
		return false;
	}
};

struct googletterr_cachetile
{
	googletterr_tile ord;
	std::vector< uchar > texdata;
	uint flags;

	enum stateenum{
		STATE_IDLE = 0,
		STATE_LOAD = 1,
		STATE_DOWNLOADING = 2,
		STATE_READYTODRAW = 3,
		STATE_DRAWING = 4,
		STATE_UNLOAD = 5,	//??
		STATE_FAILED = 6,
	};

	void set( stateenum v ){ flags = v; }
	uint get(){ return flags; }
	bool is( stateenum v ){ return flags == v; }

	bool operator<( const googletterr_cachetile & old  ) const
	{
		return ord < old.ord;
	}
};

struct googletterr_thread_data
{
	int active;
	int currprogress_max;
	int currprogress;

	googletterr_cachetile loadme;	//only 1 at a time

	matrix4x4 center;	//Point to pull from (compute lat/long from this)
	CDRVer * m_cdrptr;
	//uchar depth;		//Depth requested at center, because mesh is consistent
	//uchar tiletype;		//Load a specific tile type
	//uchar waitforload;	//Flag only, no need for 4 bytes


	//std::map< googletterr_tile, int > tiles;	//Hm. x, y, d stores a status flag. 0 = ready, +1 = load, -1 = unload.
};


//Static obstacle data == 48 bytes (location, radius, heights, index, name)
struct dbObstacle{

	int x_long_ms;	//(4) Uses CORRECT sign convention ('E' = +, 'W' = -)
	int y_lat_ms;	//(4) Uses CORRECT sign convention ('N' = +, 'S' = -)

	uint mapbits;		//(4) Many bits here
	uint descrindex;	//(4) description index
	//16
	int alt_agl_ft;	//(4) altitude of object from ground height
	int alt_msl_ft;	//(4) altitude of object from msl (hm)
	short err_loc_ft;	//(2) error in location (radius)
	short err_height_ft;//(2) error in height (additional height)
	//28
	uchar name[12];		//(12)obstacle name (non-zero terminated)
	//36
	//uint index;		//(4) some sort of index (reverse-find from pointer?)
	//uint user;		//(4) user value
	//48
	float x,y,z;	//(12)	converted position (saves compy)
	float nx,ny,nz;	//(12)	converted normal (saves compy)

	//Use THESE only for access
	inline int getAltitudeMSLft() const { return alt_msl_ft; };
	inline int getAltitudeAGLft() const { return alt_agl_ft; };
	inline int getLongitudems() const { return x_long_ms; };
	inline int getLatitudems() const { return y_lat_ms; };
	inline int getErrLocft() const { return err_loc_ft; };
	inline int getErrHeightft() const { return err_height_ft; };

	inline uint getVerifiedBit() const { return (mapbits >>23) & 1; };
	inline uint getValidatedBit() const { return (mapbits >>31) & 1; };

	inline uint getStateIndex() const { return (mapbits) & 255; };
	inline uint getCountryIndex() const { return (mapbits >> 8) & 255; };
	inline uint getTypeIndex() const { return (mapbits >> 16) & 127; };
	inline uint getDatumIndex() const { return (mapbits >> 24) & 127; };
	inline uint getDescriptionIndex() const { return descrindex; }// 0) & 0; };

	////#ERROR map this encoding!
	//O.mapbits =
	//	(dex_datum & 127)<<24 |
	//	(dex_type & 127)<<16 |
	//	(dex_country & 255)<<8 |
	//	(dex_state & 255);
	//if( bitvalid ){ O.mapbits |= 1<<31; }
	//if( bitverif ){ O.mapbits |= 1<<23; }

	inline const std::string & getState() const { return m_map_state_codes[ getStateIndex() ]; }
	inline const std::string & getCountry() const { return m_map_country_codes[ getCountryIndex() ]; }
	inline const std::string & getType() const { return m_map_type_codes[ getTypeIndex() ]; }
	inline const std::string & getDatum() const { return m_map_datum_codes[ getDatumIndex() ]; }
	inline const std::string & getDescription() const { return m_map_description_tags[ getDescriptionIndex() ]; }

	static std::vector< std::string > m_map_state_codes;		//255 -> bits
	static std::vector< std::string > m_map_country_codes;		//255 -> bits
	static std::vector< std::string > m_map_type_codes;			//127 -> bits
	static std::vector< std::string > m_map_datum_codes;		//127 -> bits
	static std::vector< std::string > m_map_description_tags;	//? -> bits
};

//One time load, from a thread (which is really nice)
struct obstloadthreaddata
{
	int active;
	int currprogress_max;
	int currprogress;

	std::string database_obstacle_filename_zipped;
	std::string database_obstacle_filename;

	std::vector< dbObstacle > * m_db_obstacles_ptr;

	CDRVer * cdrp;

	int * obstuseflag;	//m_use_obstacles_data
};


//Draaaaaawing!
struct dbDrawShape{

};



struct csvinfertype{

	uint mode;
	uint skiplines;
	uint header_len;
	std::string searchfor;
	std::string headerrow;
	std::string inferstring;
	std::string searchlinestart;
	std::vector< int > commasplits;
	std::vector< std::string > columnheaders;
};

struct csvinfersplitlinetype{
	int mode;
	std::string findkey;
    std::string replaceheader;
	std::vector< uint > commasplits;
	std::vector< std::string > columnheaders;
};

struct trackablecsvcolumn
{
	std::vector< int > column_types;
	std::vector< std::string > column_type_formats;

	std::vector< int > data_types;
	std::vector< int > data_types_excoltype;
	std::vector< int > data_types_excolformat;

	//CDRVer::exdata data_exsinglecol;	//always write ot element 0 per column used.
	//int hadexdata;

	std::vector< double > data_multipliers;
	std::vector< uint > columntypesrequired;
	std::vector< std::string > columnheaders;
};

struct filterkey{

	bool operator ==( const filterkey & B ) const {
		if( (memcmp( data, B.data, 8 ) == 0 ) ){
			return true;
		}
		return false;
	}
	bool operator <( const filterkey & B ) const {
		if( (memcmp( data, B.data, 8 ) < 0 ) ){
			return true;
		}
		return false;
	}

	inline void from_int( const int V ){
		allspaces();
		char buff[28];
		int writ = sprintf( buff, "%d", V );
		if( writ > 7 ){ writ = 8; }
		memcpy( data, buff, writ );
	}

	inline void from_string( const std::string & S ){
		allspaces();
		int ssize = S.size();
		if( ssize > 7 ){ ssize = 8; }
		memcpy( data, S.c_str(), ssize );
	}

	inline void allspaces(){ data[0] = ' '; data[1] = ' '; data[2] = ' '; data[3] = ' '; data[4] = ' '; data[5] = ' '; data[6] = ' '; data[7] = ' '; }

	uchar data[8];
};

struct vsfState{

	//Something...
};

//Cool
namespace afs{

template< typename _T >
std::string toString( const _T & V, uint options = 0 )
{
	std::stringstream sizzle;
	//On ni
	sizzle<<V;
	return sizzle.str();
}

class routeData
{
	public:

		struct fltpoint{

			float x,y,z;	//Embedded altitude (CDRECEF XYZ nmi)
			uint timems;

			float vx,vy,vz;	//Ground Speed vector (nmi/hr) and BEARING (direction we are moving)
			float heading;	//Heading vector (direction airframe is pointed!)

			//Control values (roll
			float roll;		//AIRFRAME relative roll
			float pitch;	//AIRFRAME relative pitch
			float reserved1;
			float reserved2;

			float getHeadingdeg() { return heading; }
			uint getTimems() { return timems; }
			float getSpeednmihr() { return sqrt(vx*vx + vy*vy + vz*vz); }
			float getAltft() { double P[3] = {x,y,z}; return ( cdr::XYZToAltitude( P ) - EARTH_RADIUS_WGS84_NMI ); }

			//Given TWO points:
			//Initial, Final
			//Compute interpolating points between...
			//
			//	(x,y,z) + desired heading
			//
			//Hm...
			//	Then the MAXIMUM energy curve between them is a HERMITE spline because:
			//	3D position + direction => P0 V0 ... P1 V1
			//	Hermites require specially formed velocity curves. To do this correctly....
			//	This is NOT a hermite... or better, how do we form a hermite?
			//
			//
			inline void herp( float factor, const fltpoint & p0, const fltpoint & p1, fltpoint & result )
			{
				//Given input x,y,z (which includes altitude) and HEADING only
				//Compute result point with a factor, fills in available fields.
				//Once route is computed, we have to go back and compute appropriate times, speeds and such.
				//

				//finite difference interpolation:

				//mk = (p[k+1] - p[k])/(2*(t[k+1] - t[k])) + (p[k] - p[k-1])/(2*(t[k] - t[k-1]))
				//p0 .. p0 + M0/3, p1 - M1/3, p1
				//

			}

			inline void setAltitudeFt( float v )
			{
				double P[3] = { CONVERT_CM_TO_NMI*(double)x, CONVERT_CM_TO_NMI*(double)y, CONVERT_CM_TO_NMI*(double)z };
				double R[3];
				cdr::XYZChangeAltitude( P, CONVERT_FT_TO_NMI * (double)v, R );
				x = R[0] * CONVERT_NMI_TO_CM;
				y = R[1] * CONVERT_NMI_TO_CM;
				z = R[2] * CONVERT_NMI_TO_CM;
			}

            inline void setVelocityFromHeading( float headdegrees, float magnitude = 1.0 )
            {
            	//cdr::
            	//cdr::GetXYZSpherical(0,0,heading, 1.0, x, y, z, vx, vy, vz );
				double P[3] = { CONVERT_CM_TO_NMI*(double)x, CONVERT_CM_TO_NMI*(double)y, CONVERT_CM_TO_NMI*(double)z };
				double V[3];
            	cdr::XYZToBearing( P[0], P[1], P[2], (double)headdegrees, V[0], V[1], V[2] );
				vx = magnitude*V[0];
				vy = magnitude*V[1];
				vz = magnitude*V[2];
            }

		};

	public:

		void clear();
		bool addLeg( const std::string & legtype );
		bool setTopLegWaypoint( uint fixid );
		bool setTopLegPositioncm( int x, int y, int z );
		bool setTopHeadingDegrees( float useheadingdegrees );
		bool setTopAltitudeFt( float usealtfeet );
		bool setTopSpeedNmiperhr( float speednmihr );

		void loadFromCommand( std::vector< std::string > words, CDRVer * CDR );	//cool?
		void saveToCommand( std::string & output, CDRVer * CDR ) const;	//cool? hm...

		int getNumLegs() const;

		//Logical access (data view)
		inline void setSpeedNmiperhr( uint id, float v ) { m_points[id].setSpeedNmiperhr(v); }
		inline void setAltitudeFt( uint id, float v ) { m_points[id].setAltitudeFt(v); }
		inline void setHeadingDegrees( uint id, float v ) { m_points[id].setHeadingDegrees(v); }
		inline void setFixID( uint id, int fid ) { m_points[id].setFixID(fid); }
		inline void setPositioncm( uint id, int x, int y, int z ) { m_points[id].setPositioncm(x,y,z); }
		inline void setLegType( uint id, uchar legt ) { m_points[id].setLegType(legt); }
		inline void setFlags( uint id, uchar whichflags ) { m_points[id].setFlags(whichflags); }
		inline void clearFlags( uint id, uchar whichflags ) { m_points[id].clearFlags(whichflags); }

		inline bool validLeg( uint id ) const { return m_points[id].validLeg(); }
		inline bool validSpeed( uint id ) const { return m_points[id].validSpeed(); }
		inline bool validPosition( uint id ) const { return m_points[id].validPosition(); }
		inline bool validAlt( uint id ) const { return m_points[id].validAlt(); }
		inline bool validHeading( uint id ) const { return m_points[id].validHeading(); }
		inline bool validFixID( uint id ) const { return m_points[id].validFixID(); }

		inline float getSpeedNmiperhr( uint id ) const { return m_points[id].getSpeedNmiperhr(); }
		inline float getAltitudeFt( uint id ) const { return m_points[id].getAltitudeFt(); }
		inline float getHeadingDegrees( uint id ) const { return m_points[id].getHeadingDegrees(); }
		inline int getFixID( uint id ) const { return m_points[id].getFixID(); }
		inline uchar getLegType( uint id ) const { return m_points[id].getLegType(); }
		inline uint getFlags( uint id ) const { return m_points[id].getFlags(); }
		inline void getPositioncm( uint id, int &ux, int &uy, int &uz ) const { ux = m_points[id].x_cm; uy = m_points[id].y_cm; uz = m_points[id].z_cm; }

		static void init( CDRVer * CDR );

		//int getLegPoint( uint index );

		int fly( uint deltimems, std::vector< fltpoint > & track, uint options = 0 );	//GENERATE data for a buffer which includes the following: x, y, z, |t rel| ?

		//int placePlanesOnLeg( uint index, uint nplanes, ? );//float startwidth, float endwidth = 0 );

	private:

		struct interpoint{	//32 bytes per point

			//Internal storage (data)
			int x_cm,y_cm,z_cm;		//12 ECEF position & altitude (CDRAlt)
			float param;		//4 parameter for this leg type

			int fixid;			//4 fix id that has been SET. hopefully there are no more than 65000 fix id's... well there are at least 60000...
			uint flags;			//4

			short speedset_10nmihr;//2	Integer speed restrictions/set, in 10*nmi/hr. knots, so 3200 nmi/hr is maximum.
			short heading_100deg;	//2	heading in 100's of degrees. Note this value goes from -18000 ... 18000, NOT 0..36000

			ushort reserved2;	//2
			uchar legtype;		//1 Leg type to use for this interpoint
			uchar reserved1;	//1 eight flags you can use!

			//int fixid;			//4	Index of fix to use
			//float alt_ft;		//4	0 = MSL altitude in ft

			enum{
				VALID_POINT = 1,
				VALID_POSITION = 1<<1,
				VALID_ALTITUDE = 1<<2,
				VALID_MAX,
			};

			inline void clear()
			{
				x_cm = 0;
				y_cm = 0;
				z_cm = 0;
				param = 0;
				legtype = 0;
				flags = 0;
				speedset_10nmihr = -32760;
				heading_100deg = -32760;
				fixid = -1;
				//alt_ft = -8192;
			}

			//Logical access (data view)
			inline void setSpeedNmiperhr( float v ) { speedset_10nmihr = v*10; }
			inline void setAltitudeFt( float v )
			{
				double P[3] = { CONVERT_CM_TO_NMI*(double)x_cm, CONVERT_CM_TO_NMI*(double)y_cm, CONVERT_CM_TO_NMI*(double)z_cm };
				double R[3];
				cdr::XYZChangeAltitude( P, CONVERT_FT_TO_NMI * (double)v, R );
				x_cm = R[0] * CONVERT_NMI_TO_CM;
				y_cm = R[1] * CONVERT_NMI_TO_CM;
				z_cm = R[2] * CONVERT_NMI_TO_CM;
				setFlags( VALID_ALTITUDE );
			}
			inline void setHeadingDegrees( float v ) {
				if( v > 180 ){
					v -= 360.0f;
					//additional modulous
				}else if( v < -180 ){
					v += 360.0f;
					//additional modulous
				}
				heading_100deg = v*100;
			}
			inline void setFixID( int fid ) { fixid = fid;}
			inline void setLegType( uchar legt ) { legtype = legt; }
			inline void setFlags( uchar whichflags ) { flags |= whichflags; }

			inline void setPositioncm( int ux, int uy, int uz )
			{
				double setalt = 0;
				bool usesetalt = validAlt();
				if( usesetalt ){
					setalt = getAltitudeFt();
				}
				x_cm = ux;
				y_cm = uy;
				z_cm = uz;
				if( usesetalt ){
					setAltitudeFt( setalt );
				}
				setFlags( VALID_POSITION );
			}

			inline void clearFlags( uchar whichflags ) { flags &= ~whichflags; }

			inline bool validLeg() const { return legtype > 0; }
			inline bool validSpeed() const { return speedset_10nmihr > -32760; }
			inline bool validPosition() const { return (flags & VALID_POSITION) != 0; }
			inline bool validAlt() const { return (flags & VALID_ALTITUDE) != 0; }
			inline bool validHeading() const { return heading_100deg > -32760; }
			inline bool validFixID() const { return fixid >= 0; }

			inline float getSpeedNmiperhr() const { return ((float)speedset_10nmihr)/10; }
			inline float getAltitudeFt() const
			{
				double P[3] = { CONVERT_CM_TO_NMI*(double)x_cm, CONVERT_CM_TO_NMI*(double)y_cm, CONVERT_CM_TO_NMI*(double)z_cm };
				double Palt = cdr::XYZToAltitude( P );
				return Palt;
			}
			inline float getHeadingDegrees() const { return ((float)heading_100deg)/100.0; }
			inline int getFixID() const { return fixid;}
			inline uchar getLegType() const { return legtype; }
			inline uint getFlags() const { return flags; }
		};

		std::vector< interpoint > m_points;

		static std::vector< std::string > m_legtypes_caseless;
};

};

#endif //CDRVERHEADER_H

