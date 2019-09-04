
const ns = require( './nsutils' );

//------------------------------------------------------------------------------------------------------------------

var DATA_DIRECTORY = '../../data/';

var LRU_FILECACHE = {};
var LRU_FILECACHELIST = [];	//Huh..
var LRU_FILECACHELISTMAX = 30;

//get all files & breakdowns/data sources

var allrawfiles = ns.getEntireHeirarchyOf( DATA_DIRECTORY, undefined, 2, undefined );

var adddatasources = {};
//Auto process allfiles -> Remove DATA_DIRECTORY prefix
for( var fname in allrawfiles ){
	var subname = allrawfiles[fname].substr( DATA_DIRECTORY.length );
	var firstslash = subname.indexOf( '/' );
	var dsname = subname.substr( 0, firstslash );
	var filename = subname.substr( firstslash+1 );
	
	//Process filename into something meaningful:
	//	[ dsname ][ latitude ][ longitude ] = filename	(signed, btw)
	//
	uselat = 0;
	uselong = 0;
	valid = false;
		
	//Process formatting of filenames (dammit)
	//ned
	//	imgn25w099_1min.json
	//	"img" + sign( n/s) + int( lat ) + sign( w/e ) + int( long ) + "_1min.json"
	//dted2
	//	pavgs60_-67_17.json
	//	"pavgs60_" + int( long ) + "_" + int( lat ) + ".json"

	if( filename.substr(0,3) == 'img' ){
		
		//ned
		//	imgn25w099_1min.json
		//	"img" + sign( n/s) + int( lat ) + sign( w/e ) + int( long ) + "_1min.json"
		
		var depname = filename;
		var istart = depname.indexOf( "img" )
		depname = depname.substr( istart + 3 );
		
		var iend = depname.lastIndexOf( "_1min.json" )
		depname = depname.substr( 0, iend );
		
		//n25w099
		numbers = depname.split( /[^0-9.]/ )	//split by NON numbers => [ '', lat, long ]
		symbols = depname.split( /[0-9.]/ )	//split by NUMBERS => [ 'n', '', 'w', '' ]
		
		if( numbers.length > 2 ){
			if( symbols.length > 2 ){
				
				//ns.trace( numbers, symbols );
				uselong = parseInt( numbers[2], 10 );
				uselat = parseInt( numbers[1], 10 );
				if( symbols[0] =='s' || symbols[0] == 'S' ){
					uselat = -uselat;
				}
				if( symbols[2] =='w' || symbols[2] == 'W' ){
					uselong = -uselong;
				}
				
				valid = true;
			}
		}
		
	}else if( filename.substr(0,3) == 'pav' ){
		
		//dted2
		//	pavgs60_-67_17.json
		//	"pavgs60_" + int( long ) + "_" + int( lat ) + ".json"
		
		var depname = filename;
		var iend = depname.lastIndexOf( ".json" )
		depname = depname.substr( 0, iend );
		
		//pavgs60_-67_17
		var parts = depname.split( '_' );
		if( parts.length > 2 ){
			uselong = parseInt( parts[1], 10 );
			uselat = parseInt( parts[2], 10 );
			valid = true;
		}
	}
	
	//
	//with a valid, processed filename:
	//
	
	if( valid ){
		//adddatasources[ dsname ][ filename ] = {};
		
		if( ! adddatasources.hasOwnProperty( dsname ) ){
			adddatasources[ dsname ] = {};
		}
		
		if( ! adddatasources[ dsname ].hasOwnProperty( uselat ) ){
			adddatasources[ dsname ][ uselat ] = {};
		}
		
		//if( ! adddatasources[ dsname ][ uselat ].hasOwnProperty( uselong ) ){
		//	adddatasources[ dsname ][ uselat ][ uselong ] = filename;
		//}
		adddatasources[ dsname ][ uselat ][ uselong ] = DATA_DIRECTORY+dsname+'/'+filename;
		
	}else{
		ns.trace( "Invalid file: ", filename );
	}
}

//ns.trace( Object.keys( adddatasources ) );
//ns.trace( adddatasources[ 'dted2' ][ 35 ][ -97 ] )
//ns.trace( adddatasources[ 'ned' ][ 35 ][ -97 ] )

//callback( err, jsonobj );
function acquireFileAsync( longdeg, latdeg, callback )
{
	var pleasewait = false;
	var floorlat = Math.floor( latdeg );
	var flootlong = Math.floor( longdeg );
	
	if( adddatasources[ datasourcekey ].hasOwnProperty( floorlat ) ){
		if( adddatasources[ datasourcekey ][ floorlat ].hasOwnProperty( flootlong ) ){
			var filename = adddatasources[ datasourcekey ][ floorlat ][ flootlong ];
			
			//do we have this file LRU cached? (no need to read&parse MULTIPLE times...)
			if( LRU_FILECACHE.hasOwnProperty( filename ) ){
				
				return callback( undefined, LRU_FILECACHE[ filename ] );
			}else{
				
				pleasewait = true;
			
				ns.readFileAsync( filename, function (err, data) {
					if (err) {
						ns.serverRespondJSON( res, {"error":"SERVER: Parsing error on internal file "+filename} )
						return;	//throw err
					}
					
					LRU_FILECACHELIST = [];	//Huh..
								
					var job = JSON.parse( data )
					
					LRU_FILECACHE[ filename ] = job;
					LRU_FILECACHELIST.push( filename );
					if( LRU_FILECACHELIST.length > LRU_FILECACHELISTMAX ){
						lastfile = LRU_FILECACHELIST[ LRU_FILECACHELIST.length-1 ]
						LRU_FILECACHELIST.splice( 0, 1 );
						delete LRU_FILECACHE[ filename ];
					}
				
					return callback( undefined, job );
				} );
				
				return;
			}
		}
	}
	
	//http://localhost:8123/dted2/get?long=-90.03&lat=40.01
	if( !pleasewait ){
		return callback( {"error":"Missing file or error"}, undefined );
	}
}

/**
 * Vincenty inverse calculation.
 *
 * @private
 * @param   {LatLon} point - Latitude/longitude of destination point. (dependant on geoid units of distance)
 * @returns {Object} Object including distance, initialBearing, finalBearing.
 * @throws  {Error}  If λ > π or formula failed to converge.
 *
 * https://www.movable-type.co.uk/scripts/latlong-vincenty.html
 */
var RADIANS_TO_DEG = 57.295779513082320876798154814105;
function vincentyInverse( p1, p2, datum, tolerance=1e-12, max_iterations=1000 ) {
    if (p1.lon == -180) p1.lon = 180;
    var φ1 = p1.lat / RADIANS_TO_DEG, λ1 = p1.lon / RADIANS_TO_DEG;
    var φ2 = p2.lat / RADIANS_TO_DEG, λ2 = p2.lon / RADIANS_TO_DEG;

    var a = datum.ellipsoid.a, b = datum.ellipsoid.b, f = datum.ellipsoid.f;

    var L = λ2 - λ1;
    var tanU1 = (1-f) * Math.tan(φ1), cosU1 = 1 / Math.sqrt((1 + tanU1*tanU1)), sinU1 = tanU1 * cosU1;
    var tanU2 = (1-f) * Math.tan(φ2), cosU2 = 1 / Math.sqrt((1 + tanU2*tanU2)), sinU2 = tanU2 * cosU2;

    var sinλ, cosλ, sinSqσ, sinσ=0, cosσ=0, σ=0, sinα, cosSqα=0, cos2σM=0, C;

    var λ = L, λʹ, iterations = 0, antimeridian = Math.abs(L) > Math.PI;
    do {
        sinλ = Math.sin(λ);
        cosλ = Math.cos(λ);
        sinSqσ = (cosU2*sinλ) * (cosU2*sinλ) + (cosU1*sinU2-sinU1*cosU2*cosλ) * (cosU1*sinU2-sinU1*cosU2*cosλ);
        if (sinSqσ == 0) break; // co-incident points
        sinσ = Math.sqrt(sinSqσ);
        cosσ = sinU1*sinU2 + cosU1*cosU2*cosλ;
        σ = Math.atan2(sinσ, cosσ);
        sinα = cosU1 * cosU2 * sinλ / sinσ;
        cosSqα = 1 - sinα*sinα;
        cos2σM = (cosSqα != 0) ? (cosσ - 2*sinU1*sinU2/cosSqα) : 0; // equatorial line: cosSqα=0 (§6)
        C = f/16*cosSqα*(4+f*(4-3*cosSqα));
        λʹ = λ;
        λ = L + (1-C) * f * sinα * (σ + C*sinσ*(cos2σM+C*cosσ*(-1+2*cos2σM*cos2σM)));
        var iterationCheck = antimeridian ? Math.abs(λ)-Math.PI : Math.abs(λ);
        if (iterationCheck > Math.PI) {
			break;	//throw new Error('λ > π');
		}
    } while (Math.abs(λ-λʹ) > tolerance && ++iterations<max_iterations);
    if (iterations >= max_iterations){
		//throw new Error('Formula failed to converge');
	}

    var uSq = cosSqα * (a*a - b*b) / (b*b);
    var A = 1 + uSq/16384*(4096+uSq*(-768+uSq*(320-175*uSq)));
    var B = uSq/1024 * (256+uSq*(-128+uSq*(74-47*uSq)));
    var Δσ = B*sinσ*(cos2σM+B/4*(cosσ*(-1+2*cos2σM*cos2σM)-
        B/6*cos2σM*(-3+4*sinσ*sinσ)*(-3+4*cos2σM*cos2σM)));

    var s = b*A*(σ-Δσ);

    //var α1 = Math.atan2(cosU2*sinλ,  cosU1*sinU2-sinU1*cosU2*cosλ);
    //var α2 = Math.atan2(cosU1*sinλ, -sinU1*cosU2+cosU1*sinU2*cosλ);

    //α1 = (α1 + 2*Math.PI) % (2*Math.PI); // normalise to 0..360
    //α2 = (α2 + 2*Math.PI) % (2*Math.PI); // normalise to 0..360

    return {
        distance:       s
        //,initialBearing: α1*RADIANS_TO_DEG	//if s==0, NaN (no bearing)
        //,finalBearing:   α2*RADIANS_TO_DEG
        ,iterations:     iterations
    };
};

//Returns the distance in kilometers / vincenty object between two lat longs in degrees (- is west and south)
function vincentyInverseWGS84( long1, lat1, long2, lat2 )
{
	return vincentyInverse( { lat:lat1, lon:long1 }, { lat:lat2, lon:long2 }, {
		"ellipsoid":{
			"a":6378.137
			,"f":1/298.257223563
			,"b":6356.7523142
		}
	} )
}
/*
var test1 = vincentyInverseWGS84( //005° 42′ 53.10″W, 50° 03′ 58.76″N,  	003° 04′ 12.34″W, 58° 38′ 38.48″N );
	-(005 + 42/60 + 53.10/3600)
	,(50 + 03/60 + 58.76/3600)
	,-(003 + 04/60 + 12.34/3600)
	,(58 + 38/60 + 38.48/3600)
)
//Should match: 969.95411445 distance, 
ns.trace( test1.distance );
ns.trace( test1 );
*/

/**
	latzero must be between -80 and 80 degrees...
	lonzero can be anywhere in the -180...180 range; but it may act strange around equators
	distancenmi should be in the 3..30 range, as a integer.
*/
function vincentyDistanceRangeComputeAt( distancenmi, latzero, lonzero, londelta=1/60, latdelta=1/60 )
{
	var distancekm = 1.852 * distancenmi;
	
	//now we have QUANTIZED our lat/lon... to 1/60th 
	
	//To compute this for A SINGLE lat lon...
	
	
	//now start there, create LOWER hemisphere, then UPPER hemisphere:
	//Move lon left until bleh.
	var lon = lonzero;
	var decrcount = 0;
	while( true ){
		
		var res = vincentyInverseWGS84( lonzero, latgrid, lon, latgrid );
		var dist_kilometers = res.distance;
		if( res.distance > distancekm ){
			break;
		}
		decrcount += 1;
		lon -= londelta;
	}
	
	//decrcount is the number of londelta steps LEFT (subtract) required to preserve the distancenmi metric.
	//this will be symmetrical l/r ideally. (assume for gridded points)
	//Now we have the x bounds.
	//Compute the y bounds (2 loops, up and down)
	
	var lat = latzero;
	var incrcount = 0;
	while( true ){
		
		var res = vincentyInverseWGS84( lonzero, latgrid, lon, latgrid );
		var dist_kilometers = res.distance;
		if( res.distance > distancekm ){
			break;
		}
		incrcount += 1;
		lon -= londelta;
	}
	
	
	
}

function vincentyDistanceRangesCompute( distancenmi )
{
	
	//Start at lat = 0; move up to lat = 80 and down to lat = -80 by 1/60th degree increments.
	//At each lat point, move left in longitude (from say, 90) until our distance exceeds distancenmi.
	//move back 1 step
	//then move up by 1/60th of lat, compute distance and move BACK until it passess...
	//repeat until we are back at the original lat.
	//this is the UPPER hemisphere list of ranges (symmetric)
	//Repeat process for moving DOWN latitudes
	//this is the LOWER hemisphere list of ranges.
	//so each lat value maps to a list of [ latjump, longjump, longlength ] starting at the MINIMUM coord.
	//These will be the points to sample at each location. (should be roughly a circle)
	//
	var latgriddata = [];
	var latgridmin = -80;
	var latgridmax = 80;
	var latdelta = 1/60;
	var londelta = 1/60;
	var latmin = latgridmin;
	while( latmin <= latgridmax ){
		latgriddata.push( [] );		
		latmin += latdelta;
	}
	//latgrid is now -80*60 .. 80*60
	var latgrid = latgridmin;
	while( latgrid <= latgridmax ){
		
		//now start there, create LOWER hemisphere, then UPPER hemisphere:
		//Move lon left until bleh.
		var lonzero = 90;
		var lon = lonzero;
		var decrcount = 0;
		while( true ){
			
			var res = vincentyInverseWGS84( lonzero, latgrid, lon, latgrid );
			var dist_meters = res.distance;
			
			if( dist_meters > distancenmi ){
				break;
			}
			decrcount += 1;
			lon -= londelta;
		}
		
		var lonmindelta = decrcount*londelta;
		
		//now we have the LEFT bounds ( decrcount )
		//Trace the contour UP until we get BACK to the start... (rendering a circle... but follow the estimate)
		
		//[ -lonmindelta .. lonzero .. lonmindelta ]	At STARTING latitude...
		/*
		var initialdecount = decrcount;
		
		var lat = latgrid + latdelta
		while( initialdecount > 0 ){
			
			var rowcount = initialdecount;
			var lon = lonzero - rowcount*londelta;
			
			while( rowcount > 0 ){
			
				var res = vincentyInverseWGS84( lonzero, latgrid, lon, lat );
				var dist_meters = res.distance;
				
				if( dist_meters >= distancenmi ){
					//decrease lon
				}else{
					//Valid for this lat row.
					break;
				}
				rowcount -= 1;
				lon += latdelta;
			}
			
			//??
			
		}
		*/
		
		
		//Save resultant grid:
		//latgriddata[ Math.floor( lat*60 ) ] = []
		
		latgrid += 1/60;
	}
	
}



//------------------------------------------------------------------------------------------------------------------

var serverOptions = {
	port:8123	//8123, 1723, 
	,servePublicDirectory:'public'
	//key:ns.readFileSync('keys/dwPrivateKey')
	//,cert:ns.readFileSync('keys/dwCert.cer' )
	//,passphrase:'DSFLh)(#$8ew9rhjg9ed'
};

//Programmatic dispatcher
dp = ns.createDispatcher();

dp = dp.add( '/DoThis',{ docstring:"" 
	}
	,function ( req, res, params ){
		ns.serverRespondJSON( res, { "nope":"with the crash bombs", v:Math.random() } )
		
		return ns.VALID_REQUEST;
	}
)

for( var datasourcekey in adddatasources ){
	
	dp = dp.add( '/'+datasourcekey+'/get',{ docstring:"" 
	}
	,function ( req, res, params ){
		
		var floorlat = 0;
		var flootlong = 0;
		
		if( params.hasOwnProperty( 'lat' ) ){
			floorlat = Math.floor( params.lat );
		}
		if( params.hasOwnProperty( 'long' ) ){
			flootlong = Math.floor( params.long );
		}
		
		acquireFileAsync( flootlong, floorlat, function( err, data ){
			
			if (err) {
				ns.serverRespondJSON( res, { "error":err } )
				return;	//throw err; 
			}
			
			ns.serverRespondJSON( res, data );
			
		} );
		
		return ns.VALID_REQUEST;
	} );
	
	//Compute "within distance"
	//	{"2100":{ "5820":[292,315,292,3600],"5821":[292,324,292,3600],"5822":[291,329,291,3600],"5823":[289,335,289,3600],"5824":[283,325,283,3600],"5825":[290,332,290,3600],"5826":[292,328,292,3600],"5827":[295,334,295,3600],"5828":[298,339,298,3600],"5829":[298,350,299,3600],"5830":[292,339,292,3600],"5831":[293,337,293,3600],"5832":[279,326,279,3600],"5833":[276,313,276,3600],"5834":[274,315,274,3600],"5835":[274,318,275,3600],"5836":[284,311,284,3600],"5837":[267,314,267,3600],"5838":[261,298,262,3600],"5839":[270,304,270,3600],"5840":[258,292,258,3600],"5841":[255,299,256,3600],"5842":[249,302,249,3600],"5843":[239,291,239,3600],"5844":[231,286,231,3600],"5845":[231,261,231,3600],"5846":[228,267,228,3600],"5847":[234,280,234,3600],"5848":[226,291,226,3600],"5849":[236,279,236,3600],"5850":[235,296,235,3600],"5851":[219,296,219,3600],"5852":[219,246,219,3600],"5853":[219,255,219,3600],"5854":[219,263,219,3600],"5855":[218,270,218,3600],"5856":[216,261,216,3600],"5857":[216,272,216,3600],"5858":[214,241,214,3600],"5859":[217,248,217,3600],"5860":[213,242,213,3600],"5861":[211,226,211,3600],"5862":[211,270,211
	//
	//Json objects are SIGNED [ Math.floor( lat*60 ) ][ Math.floor( long*60 ) ] = [ min, max, avg, count ] datablocks.
	//So, we want to return (given a lat/long) the points APPROXIMATELY within x nmi from that point (default is 10.)
	//Obviously our maximum radius is a bit weird near the poles.
	//
	dp = dp.add( '/'+datasourcekey+'/getWithin',{ docstring:"" 
	}
	,function ( req, res, params ){
		var floorlat = 0;
		var flootlong = 0;
		var flootradiusnmi = 0;
		
		if( params.hasOwnProperty( 'lat' ) ){
			floorlat = Math.floor( params.lat );
		}
		if( params.hasOwnProperty( 'long' ) ){
			flootlong = Math.floor( params.long );
		}
		if( params.hasOwnProperty( 'radius' ) ){
			flootradiusnmi = Math.floor( params.radius );
		}
		
		//Step 1: uh. abs( floorlat ) is under 80.
		
		//Step 2: radius is >= 3 and radius <= 30
		
		//Compute y delta based on distance estimation grid
		//Compute x delta based on distance estimation grid
		//	
		//Go through all points and add to Return object...
		//
		//Return json of all keys and data [ int(lat*60) ][ int(long*60) ] = data
		//
		
	} );
}

dp = dp.add( '/api',{ docstring:"" 
	}
	,function ( req, res, params ){
		var listy = []
		for( var datasourcekey in adddatasources ){
			listy.push( { url:datasourcekey+'/get', params:{"lat":"float latitude in degrees","long":"float longitude in degrees"} } );
			listy.push( { url:datasourcekey+'/getWithin', params:{"lat":"float latitude in degrees","long":"float longitude in degrees", "radius":"int distance in nautical miles (3..30)"} } );
		}
		ns.serverRespondJSON( res, listy );
		return ns.VALID_REQUEST;
	}
)

dp = dp.end();

//server
var server = ns.serverSimpleCreate( serverOptions, dp );


//------------------------------------------------------------------------------------------------------------------

