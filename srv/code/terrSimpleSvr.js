
const ns = require( './nsutils' );

//------------------------------------------------------------------------------------------------------------------

const DATA_DIRECTORY = '../../data/';
const METRICS_DIRECTORY = '../../metrics/';

// const DATA_DIRECTORY = 'N:/User/afs450lr Lane R/dtiles/data/';
// const METRICS_DIRECTORY = 'N:/User/afs450lr Lane R/dtiles/metrics/';

var LRU_FILECACHE = {};
var LRU_FILECACHELIST = [];	//Huh..
var LRU_FILECACHELISTMAX = 30;

//get all files & breakdowns/data sources

var allrawfiles = ns.getEntireHeirarchyOf( DATA_DIRECTORY, undefined, 2, undefined );

var adddatasources = {};
var adddatasources_meta = {};
//Auto process allfiles -> Remove DATA_DIRECTORY prefix
for( let fname in allrawfiles ){
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
	negatelong = false;	//Badly done.
	applyoffset = 0;
		
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
				negatelong = true;
				applyoffset = -60;
			}
		}
		
	}else if( filename.substr(0,7) == 'pavgs60' || filename.substr(0,8) == 'nedpfx60' ){
		
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
		
		adddatasources_meta[ dsname ] = { 'negate_lon':negatelong, 'offset_lon':applyoffset }
		
	}else{
		ns.trace( "Invalid file: ", filename );
	}
}

//ns.trace( Object.keys( adddatasources ) );
//ns.trace( adddatasources[ 'dted2' ][ 35 ][ -97 ] )
//ns.trace( adddatasources[ 'ned' ][ 35 ][ -97 ] )

//callback( err, jsonobj );
function acquireFileAsync( longdeg, latdeg, datasourcekey, callback )
{
	var pleasewait = false;
	var floorlat = Math.floor( latdeg );
	var flootlong = Math.floor( longdeg );
	
	if( adddatasources[ datasourcekey ].hasOwnProperty( floorlat ) ){
		if( adddatasources[ datasourcekey ][ floorlat ].hasOwnProperty( flootlong ) ){
			var filename = adddatasources[ datasourcekey ][ floorlat ][ flootlong ];
			var lrufilename = datasourcekey + filename;
			
			//do we have this file LRU cached? (no need to read&parse MULTIPLE times...)
			if( LRU_FILECACHE.hasOwnProperty( lrufilename ) ){
				
				return callback( undefined, LRU_FILECACHE[ lrufilename ] );
			}else{
				
				pleasewait = true;
			
				ns.readFileAsync( filename, function (err, data) {
					if (err) {
						ns.serverRespondJSON( res, {"error":"SERVER: Parsing error on internal file "+filename} )
						return;	//throw err
					}
					
					LRU_FILECACHELIST = [];	//Huh..
								
					var job = JSON.parse( data )
					
					LRU_FILECACHE[ lrufilename ] = job;
					LRU_FILECACHELIST.push( lrufilename );
					if( LRU_FILECACHELIST.length > LRU_FILECACHELISTMAX ){
						var lastfile = LRU_FILECACHELIST[ LRU_FILECACHELIST.length-1 ]
						LRU_FILECACHELIST.splice( 0, 1 );
						delete LRU_FILECACHE[ lrufilename ];
					}
					
					//Ugly. Reformat data source please!
					if( adddatasources_meta[ datasourcekey ].negate_lon ){
						
						//This is pretty strange.
						//We have to offset the value, then FLIP the values?
						//
						//flipped = min + (max - parseInt(klon))
						//
						//Then the longitude values should be right? Hm...
						//
						//Why is the LATITUDE value stuff screwy?
						//
						//Querying:
						//	http://172.25.110.85:8123/ned/getWithin?lat=44&lon=-104&radius=3
						//Results:
						//	43.933333333333333333333333333333 ... 44.066666666666666666666666666667
						//	-104.08333333333333333333333333333 .. -103.91666666666666666666666666667
						//	derived from:
						//	2636 .. 2644
						//	-6245 .. -6235
						//This exactly falls in what we would expect, perfectly.
						//
						//THEREFORE, the server is NOT giving us bogus data.
						//
						//It must be on the client side! what is WRONG?
						//
						//http://172.25.110.85:8123/ned/getWithin?lat=44.99&lon=-104&radius=3
						//
						//http://172.25.110.85:8123/ned/getWithin?lat=44.283&lon=-102.783&radius=10
						//	Bad latitude data:
						//	2657 -6167
						//DEFINITE errors in the NED data. Looks like a constant row offset.
						//
						
						var useoffset = 0;
						if( adddatasources_meta[ datasourcekey ].offset_lon ){
							useoffset = adddatasources_meta[ datasourcekey ].offset_lon;
						}
						
						//Bummer...
						for( var klat in job ){
							var olo = job[ klat ];
							var newlat = {};
							for( var klon in olo ){
								var oldi = -Math.floor( parseInt(klon) + useoffset );
								newlat[ oldi ] = olo[ klon ];
							}
							job[ klat ] = newlat;
						}
					}
				
					return callback( undefined, job );
				} );
				
				return;
			}
		}
	}
	
	//http://localhost:8123/dted2/get?long=-90.03&lat=40.01
	if( !pleasewait ){
		return callback( {"error":"Missing file or error"}, {} );
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
function vincentyDistanceRangeBoxComputeAt( distancenmi, lonzero, latzero, londelta=1/60, latdelta=1/60 )
{
	var distancekm = 1.852 * distancenmi;
	
	//now we have QUANTIZED our lat/lon... to 1/60th 
	
	//To compute this for A SINGLE lat lon...
	
	//now start there, create LOWER hemisphere, then UPPER hemisphere:
	//Move lon left until bleh.
	var lon = lonzero;
	var londecrcount = 0;
	var maxsize = 60;
	while( maxsize > 0 ){
		
		var res = vincentyInverseWGS84( lonzero, latzero, lon, latzero );
		if( res.distance > distancekm ){
			break;
		}
		londecrcount += 1;
		lon -= londelta;
		maxsize -= 1;
	}
	
	var lon = lonzero;
	var lonincrcount = 0;
	var maxsize = 60;
	while( maxsize > 0 ){
		
		var res = vincentyInverseWGS84( lonzero, latzero, lon, latzero );
		if( res.distance > distancekm ){
			break;
		}
		lonincrcount += 1;
		lon += londelta;
		maxsize -= 1;
	}
	
	//londecrcount is the number of londelta steps LEFT (subtract) required to preserve the distancenmi metric.
	//this will be symmetrical l/r ideally. (assume for gridded points)
	//Now we have the x bounds.
	//Compute the y bounds (2 loops, up and down)
	
	var lat = latzero;
	var latincrcount = 0;
	var maxsize = 60;
	while( maxsize > 0 ){
		
		var res = vincentyInverseWGS84( lonzero, latzero, lonzero, lat );
		if( res.distance > distancekm ){
			break;
		}
		latincrcount += 1;
		lat += latdelta;
		maxsize -= 1;
	}
	
	//latincrcount is the number of latdelta steps to add to latzero until the NEXT addition would be out of range.
	
	var lat = latzero;
	var latdecrcount = 0;
	var maxsize = 60;
	while( maxsize > 0 ){
		
		var res = vincentyInverseWGS84( lonzero, latzero, lonzero, lat );
		if( res.distance > distancekm ){
			break;
		}
		latdecrcount += 1;
		lat -= latdelta;
		maxsize -= 1;
	}
	
	var distrange = { lon:{ incr:lonincrcount, decr:londecrcount }, lat:{ incr:latincrcount, decr:latdecrcount } }
	
	var grid = [];
	var lat = -latdecrcount;
	while( lat <= latincrcount ){
		grid.push( [ londecrcount,lonincrcount ] );	// [ decr, incr ]
		lat += 1;
	}
	distrange.grid = grid;
	
	return distrange;
	
	//latdecrcount is the number of latdelta steps to subtract from latzero until the NEXT addition would be out of range.
	
	//now we have our bounding box!
	//			latincrcount
	//-londecrcount .. 0 .. londecrcount
	//			-latdecrcount
	//note this BOX is not necessarily a CIRCLE.
	//However, we have 2 arcs to follow
	//Meaning, the storage should include these bounds,
	//then a array of "counts" from the zero axis...
	//
}


function vincentyDistanceRangeCircleComputeAt( distancenmi, lonzero, latzero, londelta=1/60, latdelta=1/60 )
{
	
	var distrange = vincentyDistanceRangeBoxComputeAt( distancenmi, lonzero, latzero, londelta, latdelta );
	var distancekm = 1.852 * distancenmi;
	
	//return distrange;
	
	//return distrange;	//disabled (square only)
	
	//the output of the CIRCLE algorithm is different:
	//we have to store EACH latitude row in a
	//[
	//	[ min, max ]
	//	, [ min, max ]
	//	...
	//]
	//way, so that we know how to run through the rows and get those elements.
	//Basically we are drawing an arc in a nonlinear space
	//
	
	//fill default circle array (offset lateral array row set...)
	//Array is from MINIMUM lat to MAXIMUM lat...
	var grid = [];
	var lat = -distrange.lat.decr;
	while( lat <= distrange.lat.incr ){
		grid.push( [ -1,-1 ] );	// [ decr, incr ]
		lat += 1;
	}
	
	//distrange.grid[0] = lat-latdecrcount;
	//distrange.grid[latdecrcount] = lat
	//distrange.grid[latdecrcount+latincrcount] = lat+incrcount
	
	//Upper hemisphere
	var loncounter = distrange.lon.decr;
	var lon = lonzero - distrange.lon.decr*londelta;	//left/decreasing longitude FIRST (left side)
	var latincr = distrange.lat.decr;
	var lat = latzero - latdelta*(latincr-distrange.lat.decr);	//CENTER latitude
	while( loncounter >= 0 ){
		var res = vincentyInverseWGS84( lonzero, latzero, lon, lat );
		if( res.distance > distancekm ){
			loncounter -= 1;
			lon = lonzero - loncounter*londelta	//Move RIGHT from left side one step
		}else{
			grid[ latincr ] = [ loncounter, loncounter ];	//Assumes symmetry!
			latincr += 1;
			lat = latzero + latdelta*(latincr-distrange.lat.decr);	//Move UP one step
		}
	}
	
	
	//Lower hemisphere
	var loncounter = distrange.lon.decr;
	var lon = lonzero - distrange.lon.decr*londelta;
	var latincr = distrange.lat.decr - 1;
	var lat = latzero - latdelta*(latincr-distrange.lat.decr);	//center latitude DOWN one step
	while( (loncounter >= 0) && (latincr >= 0) ){
		var res = vincentyInverseWGS84( lonzero, latzero, lon, lat );
		if( res.distance > distancekm ){
			loncounter -= 1;
			lon = lonzero - loncounter*londelta
		}else{
			grid[ latincr ] = [ loncounter, loncounter ];	//Assumes symmetry!
			latincr -= 1;
			lat = latzero - latdelta*(latincr-distrange.lat.decr);	//move DOWN one step
		}
	}
	
	distrange.grid = grid;
	
	return distrange;
}

function ned_correction_latitude( celllist )
{
	let R = []
	
	//llist.push( [ lati, loni ] );
					
	for( let k in celllist ){
		
		let t = celllist[ k ];
		var purelat = 60*Math.floor(t[0]/60)
		var deltacell = 60 - (t[0] - purelat);	//Source data is OFF by 1 row?? not always! Strange! (likely some files have DIFFERENT formats/meta in the ned... like different padding?
		
		R.push( [ purelat + deltacell, t[1] ] );
	}
	
	return R;
}

/*
var res = vincentyDistanceRangeBoxComputeAt( 10, -80, 0 );
//
//res.lon.decr	inclusive count of 1/60th steps to subtract to lon
//res.lon.incr	inclusive count of 1/60th steps to add to lon
//res.lat.decr	inclusive count of 1/60th steps to subtract to lat
//res.lat.incr	inclusive count of 1/60th steps to add to lon
//
//ellipse rendering is following this test from the diamond corners... 4 quadrents to follow.
//
ns.trace( res );
*/

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
		var maxsize = 60;
		while( maxsize > 0 ){
			
			var res = vincentyInverseWGS84( lonzero, latgrid, lon, latgrid );
			var dist_meters = res.distance;
			
			if( dist_meters > distancenmi ){
				break;
			}
			decrcount += 1;
			lon -= londelta;
			maxsize -= 1;
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

//-103.79, 39.26 => discontinuous longitude values, extreme slope
//	http://localhost:8123/ned/getWithin?lat=39.26&lon=-103.79&radius=10
//-104.0, 39.583 => continuous, no visible problem
//	http://localhost:8123/ned/getWithin?lat=39.583&lon=-104.0&radius=10
//-102.19, 39.58 => Vertical stripe only, visible problem
//	http://localhost:8123/ned/getWithin?lat=39.58&lon=-102.19&radius=10


function process_cell_get( parameters, user_callback )
{
	var zerolon = parameters.lon
	var zerolat = parameters.lat
	var use_radius = parameters.radius
	var datasourcekey = parameters.datakey

	var fixedlondelta = 1/60;
	var fixedlatdelta = 1/60;
	
	//this hsould be a LOOKUP based only on latitude block.
	var getllbounds = { lon:{decr:0,incr:0},lat:{decr:0,incr:0}, grid:[[0,0]] };
	//if( usemethod == 1 ){
		
	//CACHE gridrowsource per EACH LATITUDE VALUE & radius combo so we don't have to compute it twice...
	if( true ){
			
		getllbounds = vincentyDistanceRangeCircleComputeAt( use_radius, zerolon, zerolat, fixedlondelta, fixedlatdelta );
		
		//getllbounds.grid = [ ] len = -getllbounds.lat.decr .. getllbounds.lat.incr
		//each element is [ minlondecr, maxlonincr ] like [ -4, 4 ]
		//
		
		//}else{
		//	getllbounds = vincentyDistanceRangeBoxComputeAt( flootradiusnmi, zerolon, zerolat, fixedlondelta, fixedlatdelta );
		//}
	}
	
	//Cache this type of request?
	
		//Not as efficient since we are constructing memory to do this...
		var celllist = [];
		var lati = Math.floor( 60*( zerolat - getllbounds.lat.decr*fixedlatdelta ) );	//-getllbounds.lat.decr
		//var latimax = Math.floor( 60*( zerolat + getllbounds.lat.incr*fixedlatdelta ) );
		for( var elmi in getllbounds.grid ){
			
			var elm = getllbounds.grid[ elmi ];
			
			if( elm[0] >= 0 ){
			
				var lonimin = Math.floor( 60*( zerolon - elm[0]*fixedlondelta ) );
				var lonimax = Math.floor( 60*( zerolon + elm[1]*fixedlondelta ) );
			
				var loni = lonimin;
				while( loni <= lonimax ){
					
					celllist.push( [ lati, loni ] );
					
					loni += 1;
				}
			}
			
			lati += 1;
		}
		getllbounds.celllist = celllist;
		
	var minlon = zerolon - getllbounds.lon.decr*fixedlondelta;	//Easy for this value to be LESS than -180
	var minlat = zerolat - getllbounds.lat.decr*fixedlatdelta;	//should be -80..80
	var maxlon = zerolon + getllbounds.lon.incr*fixedlondelta;
	var maxlat = zerolat + getllbounds.lat.incr*fixedlatdelta;
	
	//Well, now we MAKE SURE we have those tiles ready if needed...
	//Only edge case is the -180/+180 boundary / antiequator
	//
	//How many files do we NEED to acquire? Hm... (at most 4)
	//
	var minGlon = Math.floor( minlon );
	var minGlat = Math.floor( minlat );
	var maxGlon = Math.floor( maxlon );
	var maxGlat = Math.floor( maxlat );
	
	var tileboundsminGlon = Math.floor( minlon*60 );
	var tileboundsminGlat = Math.floor( minlat*60 );
	var tileboundsmaxGlon = Math.floor( maxlon*60 );
	var tileboundsmaxGlat = Math.floor( maxlat*60 );
	
	var failedsource = undefined;	//{};
	//failedsource[ datasourcekey ] = {}
	
	
		
	var celliorlist = ned_correction_latitude( getllbounds.celllist )
		
	function doproc( datamm, datamM, dataMm, dataMM ){
		
		//ns.trace( datammK );
		//ns.trace( datamm );
		
		//the modification here would be to LOOKUP the query table,
		//then extract only those points within the distance circle thingy via the vincenty algorithm
		//	Adding a distance epsilon is a reasonable idea (probably on the order of 0.68 or so, since 1x1 nmi ish squares blah)
		//
		
		//gridrowsource
		
		let results = {};
		
		var localcelllist = celliorlist
		function addorprop( ds )	//, results )
		{
			//Better structured loop:
			//	//
			var lastlat = -9999;
			var lastsubdK = undefined;
			var subdK = undefined;
		
			for( var celli in localcelllist ){
				var cell = localcelllist[ celli ];
				
				if( lastlat != cell[0] ){
					lastlat = cell[0];
					if( ds.hasOwnProperty( lastlat ) ){
						
						if( ! results.hasOwnProperty( lastlat ) ){
							results[ lastlat ] = {};
						}
						
						subdK = ds[ lastlat ];
					}else{
						subdK = undefined;
					}
				}
				if( subdK ){
					var lastlon = cell[1];
					if( subdK.hasOwnProperty( lastlon ) ){
						results[ lastlat ][ lastlon ] = subdK[ lastlon ];
					}
				}
			}
			
			return results;
		}
		
		if( datamm ){ addorprop( datamm, results ); }
		if( datamM ){ addorprop( datamM, results ); }
		if( dataMm ){ addorprop( dataMm, results ); }
		if( dataMM ){ addorprop( dataMM, results ); }
		
		user_callback( {
			"data":results
			, "bounds":[tileboundsminGlon,tileboundsminGlat,tileboundsmaxGlon,tileboundsmaxGlat] 
			, "coords":[minlon,minlat,maxlon,maxlat] 
			} );
	}
	
	if( minGlon != maxGlon ){
		
		if( minGlat != maxGlat ){
		
			//4 file get
			acquireFileAsync( minGlon, minGlat, datasourcekey, function( err, datamm ){ 
				if( err ){ datamm = failedsource; }
				acquireFileAsync( minGlon, maxGlat, datasourcekey, function( err, datamM ){ 
					if( err ){ datamM = failedsource; }
					acquireFileAsync( maxGlon, minGlat, datasourcekey, function( err, dataMm ){ 
						if( err ){ dataMm = failedsource; }
						acquireFileAsync( maxGlon, maxGlat, datasourcekey, function( err, dataMM ){ 
							if( err ){ dataMM = failedsource; }
						
							//NOW we have ALL the data needed:
							//datamm = minGlon, minGlat
							//datamM = minGlon, maxGlat
							//dataMm = maxGlon, minGlat
							//dataMM = maxGlon, maxGlat
							
							//Tile existence check costs log2n per check...
							//	Hm. Not that efficient.
						
							//var tileboundsminGlon = Math.floor( minlon*60 );
							//var tileboundsminGlat = Math.floor( minlat*60 );
							//var tileboundsmaxGlon = Math.floor( maxlon*60 );
							//var tileboundsmaxGlat = Math.floor( maxlat*60 );
							
							doproc( datamm, datamM, dataMm, dataMM );
							
						} );
					} );
				} );
			} );
		}else{
			
			//2 file get
			acquireFileAsync( minGlon, minGlat, datasourcekey, function( err, datamm ){ 
				if( err ){ datamm = failedsource; }
				acquireFileAsync( maxGlon, minGlat, datasourcekey, function( err, dataMm ){ 
					if( err ){ dataMm = failedsource; }
				
					//NOW we have ALL the data needed:
					//datamm = minGlon, minGlat
					//dataMm = maxGlon, minGlat
		
					doproc( datamm, failedsource, dataMm, failedsource );
						
				} );
			} );
		}
		
	}else{
		if( minGlat != maxGlat ){
		
			//2 file get
			acquireFileAsync( minGlon, minGlat, datasourcekey, function( err, datamm ){ 
				if( err ){ datamm = failedsource; }
				acquireFileAsync( minGlon, maxGlat, datasourcekey, function( err, datamM ){ 
					if( err ){ datamM = failedsource; }
				
					//NOW we have ALL the data needed:
					//datamm = minGlon, minGlat
					//datamM = minGlon, maxGlat		
					
					doproc( datamm, datamM, failedsource, failedsource );	
					
				} );
			} );
		}else{
			
			//SINGLE FILE get
			acquireFileAsync( minGlon, minGlat, datasourcekey, function( err, datamm ){ 
				if( err ){ datamm = failedsource; }
			
				//NOW we have ALL the data needed:
				//datamm = minGlon, minGlat
						
				doproc( datamm, failedsource, failedsource, failedsource );
				
			} );
		}
	}
}


function sanicheckValue( obj, latkey, lonkey )
{
	var adat = obj.data[ latkey ][ lonkey ];	//check PROXIMOUS values...
	
	if( adat[0] < -1000 ){	//huh. -413m jordan river is minish in meters.
		return false;
	}
	if( adat[1] > 10000 ){	//huh. 8,850 everest is maxish in meters.
		return false;
	}
	
	if( obj.data.hasOwnProperty( latkey-1 ) ){
		var adatpu = obj.data[ latkey-1 ][ lonkey ];
		//Difference in 
	}
	if( obj.data.hasOwnProperty( latkey+1 ) ){
		var adatpd = obj.data[ latkey+1 ][ lonkey ];
		
	}
	if( obj.data[ latkey ].hasOwnProperty( lonkey-1 ) ){
		var adatpl = obj.data[ latkey ][ lonkey-1 ];
		
	}
	if( obj.data[ latkey ].hasOwnProperty( lonkey+1 ) ){
		var adatpr = obj.data[ latkey ][ lonkey+1 ];
		
	}
	
	return true;
}

function extracmas_computerations( obj )
{
	//requestsInMemory[ usekey ] = obj
	//do a COMPUTE on this cell:
	//
	var totalmin = 0;
	var totalmax = 0;
	var totalavg = 0;
	var v_count = 0;
	//v_avg = ( v_avg * (v_count/(v_count+1)) ) + v/(v_count+1)
	
	//New computation:
	//	Sanity check proximous values (DTED is full of weird holes:
	//
	//	"-6330": [-16041.0, 3304.0, 2530.246425567703, 3567],
	//	"-6320": [-16216.0, 2703.0, 2225.925105189343, 3565],
	//	"-6310": [-16224.0, 32690.0, 2869.506811231582, 3597],
	//
	//
	
	for( let latkey in obj.data ){
		var latdat = obj.data[ latkey ];
		for( let londat in latdat ){
			var adat = latdat[ londat ];
			if( adat[3] > 0 ){
				if( sanicheckValue( obj, latkey, londat ) ){
					totalmin = adat[0];
					totalmax = adat[1];
					totalavg = adat[2];
					v_count += 1;
					break;
				}
			}
		}
	}
	
	for( let latkey in obj.data ){
		var latdat = obj.data[ latkey ];
		for( let londat in latdat ){
			var adat = latdat[ londat ];
			if( adat[3] > 0 ){
				if( sanicheckValue( obj, latkey, londat ) ){
					totalmin = Math.min( totalmin, adat[0] );
					totalmax = Math.max( totalmax, adat[1] );
					totalavg = ( totalavg * (v_count/(v_count+1)) ) + adat[2]/(v_count+1);
					v_count += 1;
				}
			}
		}
	}
	
	return [ totalmin, totalmax, totalavg, v_count ];
}



function process_entire_tile( stateobject, dsasdkey, finalcallback ){
	
	function tryUntilSuccess( options, endcall ) {
		
		process_cell_get( {
			"lon":stateobject.loniter
			,"lat":stateobject.latiter
			,"radius":stateobject.radiuslist[ stateobject.radiuscount ]	//options.radius
			,"method":1
			,"datakey":dsasdkey
		}, function( celldats ){
			
				stateobject.radius = stateobject.radiuslist[ stateobject.radiuscount ];
				
				var curr_radius = stateobject.radius;
				var curr_lonkey = Math.floor( 60*stateobject.loniter );
				var curr_latkey = Math.floor( 60*stateobject.latiter );
				
				var deeseresults = extracmas_computerations( celldats );
				
				if( ! stateobject.data.hasOwnProperty( curr_latkey ) ){
					stateobject.data[ curr_latkey ] = {};
				}
				if( ! stateobject.data[ curr_latkey ].hasOwnProperty( curr_lonkey ) ){
					stateobject.data[ curr_latkey ][ curr_lonkey ] = [];
				}
				stateobject.data[ curr_latkey ][ curr_lonkey ].push( deeseresults );
				
				//resultdata = [ min, max, avg, count ]
				//options.data[ curr_latkey ][ curr_lonkey ].push( resultdata );
				
				//do computation with tile dataset:
				//	per_radius => 60x60 grid of delta values { "mapping":[5,8,9,10,11,12,15,30], "data":{ "lat":{ "lon": [ [min,max,avg,delta], ... ] } } }
				//	therefore, each keyed radius corresponds to a [min,max,avg,delta] value. (same basic type of data, just 8 times larger.)
				//	this will pre-process ALL the data, so we can QUICKLY build a generated image based on user selected radius + threshold. (optional view avg, min, max elevations)
				//
				//r iterator first (to get all the R variants...)
				
				//do some calculations on celldats (it's NOW the correct cell list:)
				//console.log( celldats );
			
				stateobject.radiuscount += 1;
				if( stateobject.radiuscount >= stateobject.radiuslist.length ){
					stateobject.radiuscount = 0;
			
					stateobject.xcount += 1;
					stateobject.loniter += stateobject.londelta;
					if( stateobject.xcount > stateobject.xmax ){
						stateobject.xcount = 0;
						stateobject.loniter = stateobject.lon;
						
						stateobject.ycount += 1;
						stateobject.latiter += stateobject.latdelta;
						if( stateobject.ycount > stateobject.ymax ){
							stateobject.ycount = 0;
							stateobject.latiter = stateobject.lat;
							
							console.log( "Finished ", stateobject.lon, stateobject.lat );
							var result = stateobject;
							
							var difftime = process.hrtime( result.starttime )
							
							result.totaltime = difftime[0];
							
							console.log( "Took " + difftime[0] + " seconds" );
							
							var jayson = JSON.stringify( result );
							ns.writeFileAsync( './out/'+ result.resultkey +'.json', jayson, 'utf8', function(err){
								
								ns.serverRespondJSON( res, { "result":result.resultkey } );
							} );
							
							finalcallback( );
							
							return;// endcall( options );
						}
					}
				}
				
				//Proceed to NEXT tile element until set is empty
				//If set is empty, dump resultant metaanalysis to a .json file on disk
				
				//console.log( stateobject.xcount, stateobject.ycount );
				
				//tryUntilSuccess( options, endcall );
				setImmediate( tryUntilSuccess, {}, endcall );
			}
		);
	}
	
	//tryUntilSuccess( {}, function( result ){} );
	setImmediate( tryUntilSuccess, {}, function( result ){} );

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

for( let datasourcekeyiter in adddatasources ){
	
	let datasourcekey = datasourcekeyiter;

	/*
	const myEmitter = new MyEmitter();
	myEmitter.on('event_'+datasourcekey, () => {
		
		
		myEmitter.emit( 'event_'+datasourcekey );
	});
	*/
	
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
		
		acquireFileAsync( flootlong, floorlat, datasourcekey, function( err, data ){
			
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
		var usemethod = 0;
		
		var zerolon = 1.0*parseFloat(params.lon);
		var zerolat = 1.0*parseFloat(params.lat);
		
		if( params.hasOwnProperty( 'lat' ) ){
			floorlat = Math.floor( zerolat );	//parsing crash?
		}
		if( params.hasOwnProperty( 'long' ) ){
			flootlong = Math.floor( zerolon );	//parsing crash?
		}
		if( params.hasOwnProperty( 'radius' ) ){
			flootradiusnmi = Math.floor( params.radius );	//parsing crash?
		}
		if( params.hasOwnProperty( 'method' ) ){
			usemethod = Math.floor( params.method );
		}
		
		if( floorlat < -80 ){ ns.serverRespondJSON( res, { "error":"invalid latitude (-80..80) "+floorlat } ); return ns.VALID_REQUEST; }
		if( floorlat > 80 ){ ns.serverRespondJSON( res, { "error":"invalid latitude (-80..80) "+floorlat } ); return ns.VALID_REQUEST; }
		
		if( flootradiusnmi < 3 ){ ns.serverRespondJSON( res, { "error":"invalid radius (3..30) "+flootradiusnmi } ); return ns.VALID_REQUEST; }
		if( flootradiusnmi > 30 ){ ns.serverRespondJSON( res, { "error":"invalid radius (3..30) "+flootradiusnmi } ); return ns.VALID_REQUEST; }
		
		//"process cell" function... should call a CALLBACK when finished!
		process_cell_get( {
				"lon":zerolon
				,"lat":zerolat
				,"radius":flootradiusnmi
				,"method":usemethod
				,"datakey":datasourcekey
			}, function( result ){
				ns.serverRespondJSON( res, result );
			}
		);
		
		/*
		
		var fixedlondelta = 1/60;
		var fixedlatdelta = 1/60;
		
		//this hsould be a LOOKUP based only on latitude block.
		var getllbounds = { lon:{decr:0,incr:0},lat:{decr:0,incr:0}, grid:[[0,0]] };
		//if( usemethod == 1 ){
			
		//CACHE gridrowsource per EACH LATITUDE VALUE & radius combo so we don't have to compute it twice...
		if( true ){
				
			getllbounds = vincentyDistanceRangeCircleComputeAt( flootradiusnmi, zerolon, zerolat, fixedlondelta, fixedlatdelta );
			
			//getllbounds.grid = [ ] len = -getllbounds.lat.decr .. getllbounds.lat.incr
			//each element is [ minlondecr, maxlonincr ] like [ -4, 4 ]
			//
			
			//}else{
			//	getllbounds = vincentyDistanceRangeBoxComputeAt( flootradiusnmi, zerolon, zerolat, fixedlondelta, fixedlatdelta );
			//}
		}
		
		//Cache this type of request?
		
			//Not as efficient since we are constructing memory to do this...
			var celllist = [];
			var lati = Math.floor( 60*( zerolat - getllbounds.lat.decr*fixedlatdelta ) );	//-getllbounds.lat.decr
			//var latimax = Math.floor( 60*( zerolat + getllbounds.lat.incr*fixedlatdelta ) );
			for( var elmi in getllbounds.grid ){
				
				var elm = getllbounds.grid[ elmi ];
				
				if( elm[0] >= 0 ){
				
					var lonimin = Math.floor( 60*( zerolon - elm[0]*fixedlondelta ) );
					var lonimax = Math.floor( 60*( zerolon + elm[1]*fixedlondelta ) );
				
					var loni = lonimin;
					while( loni <= lonimax ){
						
						celllist.push( [ lati, loni ] );
						
						loni += 1;
					}
				}
				
				lati += 1;
			}
			getllbounds.celllist = celllist;
			
		var minlon = zerolon - getllbounds.lon.decr*fixedlondelta;	//Easy for this value to be LESS than -180
		var minlat = zerolat - getllbounds.lat.decr*fixedlatdelta;	//should be -80..80
		var maxlon = zerolon + getllbounds.lon.incr*fixedlondelta;
		var maxlat = zerolat + getllbounds.lat.incr*fixedlatdelta;
		
		//Well, now we MAKE SURE we have those tiles ready if needed...
		//Only edge case is the -180/+180 boundary / antiequator
		//
		//How many files do we NEED to acquire? Hm... (at most 4)
		//
		var minGlon = Math.floor( minlon );
		var minGlat = Math.floor( minlat );
		var maxGlon = Math.floor( maxlon );
		var maxGlat = Math.floor( maxlat );
		
		var tileboundsminGlon = Math.floor( minlon*60 );
		var tileboundsminGlat = Math.floor( minlat*60 );
		var tileboundsmaxGlon = Math.floor( maxlon*60 );
		var tileboundsmaxGlat = Math.floor( maxlat*60 );
		
		var failedsource = undefined;	//{};
		//failedsource[ datasourcekey ] = {}
		
		
			
		var celliorlist = getllbounds.celllist
			
		function doproc( datamm, datamM, dataMm, dataMM ){
			
			//ns.trace( datammK );
			//ns.trace( datamm );
			
			//the modification here would be to LOOKUP the query table,
			//then extract only those points within the distance circle thingy via the vincenty algorithm
			//	Adding a distance epsilon is a reasonable idea (probably on the order of 0.68 or so, since 1x1 nmi ish squares blah)
			//
			
			//gridrowsource
			
			let results = {};
			
			var localcelllist = celliorlist
			function addorprop( ds )	//, results )
			{
				//Better structured loop:
				//	//
				var lastlat = -9999;
				var lastsubdK = undefined;
				var subdK = undefined;
			
				for( var celli in localcelllist ){
					var cell = localcelllist[ celli ];
					if( lastlat != cell[0] ){
						lastlat = cell[0];
						if( ds.hasOwnProperty( lastlat ) ){
							
							if( ! results.hasOwnProperty( lastlat ) ){
								results[ lastlat ] = {};
							}
							
							subdK = ds[ lastlat ];
						}else{
							subdK = undefined;
						}
					}
					if( subdK ){
						var lastlon = cell[1];
						if( subdK.hasOwnProperty( lastlon ) ){
							results[ lastlat ][ lastlon ] = subdK[ lastlon ];
						}
					}
				}
				
				return results;
			}
			
			//if( datamm ){ results = addorprop( datamm, results ); }
			//if( datamM ){ results = addorprop( datamM, results ); }
			//if( dataMm ){ results = addorprop( dataMm, results ); }
			//if( dataMM ){ results = addorprop( dataMM, results ); }
			
			if( datamm ){ addorprop( datamm, results ); }
			if( datamM ){ addorprop( datamM, results ); }
			if( dataMm ){ addorprop( dataMm, results ); }
			if( dataMM ){ addorprop( dataMM, results ); }
			
			//ns.trace( datamm );
			//ns.trace( results );
			//ns.trace( [tileboundsminGlon,tileboundsminGlat,tileboundsmaxGlon,tileboundsmaxGlat] )
			//ns.trace( [minlon,minlat,maxlon,maxlat] );
			
			//this is the range in the dted files (matches correctly)
			//-106.7166666 => -6402.99996
			//-106.2833333 => -6376.99998
			//in the ned files,
			//
			
			ns.serverRespondJSON( res, {
				"data":results
				, "bounds":[tileboundsminGlon,tileboundsminGlat,tileboundsmaxGlon,tileboundsmaxGlat] 
				, "coords":[minlon,minlat,maxlon,maxlat] 
				} );
		}
		
		if( minGlon != maxGlon ){
			
			if( minGlat != maxGlat ){
			
				//4 file get
				acquireFileAsync( minGlon, minGlat, datasourcekey, function( err, datamm ){ 
					if( err ){ datamm = failedsource; }
					acquireFileAsync( minGlon, maxGlat, datasourcekey, function( err, datamM ){ 
						if( err ){ datamM = failedsource; }
						acquireFileAsync( maxGlon, minGlat, datasourcekey, function( err, dataMm ){ 
							if( err ){ dataMm = failedsource; }
							acquireFileAsync( maxGlon, maxGlat, datasourcekey, function( err, dataMM ){ 
								if( err ){ dataMM = failedsource; }
							
								//NOW we have ALL the data needed:
								//datamm = minGlon, minGlat
								//datamM = minGlon, maxGlat
								//dataMm = maxGlon, minGlat
								//dataMM = maxGlon, maxGlat
								
								//Tile existence check costs log2n per check...
								//	Hm. Not that efficient.
							
								//var tileboundsminGlon = Math.floor( minlon*60 );
								//var tileboundsminGlat = Math.floor( minlat*60 );
								//var tileboundsmaxGlon = Math.floor( maxlon*60 );
								//var tileboundsmaxGlat = Math.floor( maxlat*60 );
								
								doproc( datamm, datamM, dataMm, dataMM );
								
							} );
						} );
					} );
				} );
			}else{
				
				//2 file get
				acquireFileAsync( minGlon, minGlat, datasourcekey, function( err, datamm ){ 
					if( err ){ datamm = failedsource; }
					acquireFileAsync( maxGlon, minGlat, datasourcekey, function( err, dataMm ){ 
						if( err ){ dataMm = failedsource; }
					
						//NOW we have ALL the data needed:
						//datamm = minGlon, minGlat
						//dataMm = maxGlon, minGlat
			
						doproc( datamm, failedsource, dataMm, failedsource );
							
					} );
				} );
			}
			
		}else{
			if( minGlat != maxGlat ){
			
				//2 file get
				acquireFileAsync( minGlon, minGlat, datasourcekey, function( err, datamm ){ 
					if( err ){ datamm = failedsource; }
					acquireFileAsync( minGlon, maxGlat, datasourcekey, function( err, datamM ){ 
						if( err ){ datamM = failedsource; }
					
						//NOW we have ALL the data needed:
						//datamm = minGlon, minGlat
						//datamM = minGlon, maxGlat		
						
						doproc( datamm, datamM, failedsource, failedsource );	
						
					} );
				} );
			}else{
				
				//SINGLE FILE get
				acquireFileAsync( minGlon, minGlat, datasourcekey, function( err, datamm ){ 
					if( err ){ datamm = failedsource; }
				
					//NOW we have ALL the data needed:
					//datamm = minGlon, minGlat
							
					doproc( datamm, failedsource, failedsource, failedsource );
					
				} );
			}
		}
		
		//If the tiles are ready, compute the bounds intersection in them? (wrapped intersection for some...)
		
		//Step 1: uh. abs( floorlat ) is under 80.
		
		//Step 2: radius is >= 3 and radius <= 30
		
		//Compute y delta based on distance estimation grid
		//Compute x delta based on distance estimation grid
		//	
		//Go through all points and add to Return object...
		//
		//Return json of all keys and data [ int(lat*60) ][ int(long*60) ] = data
		//
		*/
		
		return ns.VALID_REQUEST;
	} );
	
	//http://localhost:8123/ned1/generateTile?lon=-104&lat=39
	dp = dp.add( '/'+datasourcekey+'/generateTile',{ docstring:"" 
	}
	,function ( req, res, params ){
		
		var floorlat = 0;
		var flootlong = 0;
		if( params.hasOwnProperty( 'lat' ) ){
			floorlat = Math.floor( params.lat );
		}
		if( params.hasOwnProperty( 'lon' ) ){
			flootlong = Math.floor( params.lon );
		}
		
		//var latiter = floorlat;
		//var loniter = flootlong;
		//var latitermax = floorlat + 1;
		//var lonitermin = flootlong + 1;
		
		var stateobject = {
			lat:floorlat
			,lon:flootlong
			,latiter:floorlat
			,loniter:flootlong
			,xcount:0
			,ycount:0
			,xmax:60
			,ymax:60
			,latdelta:(1.0/60)
			,londelta:(1.0/60)
			,radius:10
			,radiuslist:[0,3,5,7,9,10,11,13,15,20]	//NEW radius list
			,radiuscount:0
			,state:0
			,data:{}
			,resultkey:datasourcekey+'_'+floorlat+'_'+flootlong
			,starttime:process.hrtime()
		}
		
		process_entire_tile( stateobject, datasourcekey );
		
		return ns.VALID_REQUEST;
	} );
	
	//Must generateTile first...
	//http://localhost:8123/ned1/getTile?lon=-104&lat=39
	dp = dp.add( '/'+datasourcekey+'/getTile',{ docstring:"" 
	}
	,function ( req, res, params ){
		
		if( !params.hasOwnProperty( 'lat' ) ){
			ns.serverRespondJSON( res, {"error":"Missing integer lat"} )
		}else{
			if( !params.hasOwnProperty( 'lon' ) ){
				ns.serverRespondJSON( res, {"error":"Missing integer lon"} )
			}else{
				
				let floorlat = Math.floor( params.lat );
				let floorlong = Math.floor( params.lon );
				
				let filename = METRICS_DIRECTORY + 'tiles/' + datasourcekey+'_'+floorlat+'_'+floorlong+'.json'
				
				ns.readFileAsync( filename, function (err, data) {
					if (err) {
						ns.serverRespondJSON( res, {"error":"SERVER: Parsing error on internal file "+filename} )
						return;	//throw err
					}
					
					ns.serverRespondJSONString( res, data );
				} )
			}
		}
		
		return ns.VALID_REQUEST;
	} )
	
	//http://localhost:8123/visX.html
	//http://localhost:8123/visT.html
	//http://localhost:8123/ned1/generateTiles?lonmin=-105&latmin=38&lonmax=-103&latmax=40
	//http://localhost:8123/ned1/generateTiles?lonmin=-106&latmin=37&lonmax=-101&latmax=42	#Rado
	//http://localhost:8123/ned1/generateTiles?lonmin=-125&latmin=24&lonmax=-65&latmax=50	#CONUS
	dp = dp.add( '/'+datasourcekey+'/generateTiles',{ docstring:"" 
	}
	,function ( req, res, params ){
		
		var latmin = 0;
		var lonmin = 0;
		var latmax = 0;
		var lonmax = 0;
		
		if( params.hasOwnProperty( 'latmin' ) ){
			latmin = Math.floor( params.latmin );
		}else{ return ns.INVALID_REQUEST; }
		if( params.hasOwnProperty( 'lonmin' ) ){
			lonmin = Math.floor( params.lonmin );
		}else{ return ns.INVALID_REQUEST; }
		if( params.hasOwnProperty( 'latmax' ) ){
			latmax = Math.floor( params.latmax );
		}else{ return ns.INVALID_REQUEST; }
		if( params.hasOwnProperty( 'lonmax' ) ){
			lonmax = Math.floor( params.lonmax );
		}else{ return ns.INVALID_REQUEST; }
		
		//construct list to go through...
		statelister = {
			tiles:[]
		}
		for( var ilat = latmin; ilat <= latmax; ilat += 1 ){
			for( var ilon = lonmin; ilon <= lonmax; ilon += 1 ){
				statelister.tiles.push( [ ilon, ilat ] );
			}
		}
		
		function doThatNextTile( options, endcall ) {
			
			if( statelister.tiles.length > 0 ){
					
				var flootlong = statelister.tiles[0][0];
				var floorlat = statelister.tiles[0][1];
				
				console.log( "Next tile: ", floorlat, flootlong );
				
				statelister.tiles.splice(0,1);
				
				var stateobject = {
					lat:floorlat
					,lon:flootlong
					,latiter:floorlat
					,loniter:flootlong
					,xcount:0
					,ycount:0
					,xmax:60
					,ymax:60
					,latdelta:(1.0/60)
					,londelta:(1.0/60)
					,radius:10
					,radiuslist:[0,3,5,7,9,10,11,13,15,20]
					,radiuscount:0
					,state:0
					,data:{}
					,resultkey:datasourcekey+'_'+floorlat+'_'+flootlong
					,starttime:process.hrtime()
				}
				
				process_entire_tile( stateobject, datasourcekey, function(){
					
					setImmediate( doThatNextTile, {}, endcall );
					
				} );
				
			}else{
				
				console.log( "Completed Batch " );
			}
			
		}
		
		//tryUntilSuccess( {}, function( result ){} );
		setImmediate( doThatNextTile, {}, function( result ){} );
		
		return ns.VALID_REQUEST;
		
	} );
	
}

//Next up is GENERATING an ENTIRE TILE
//Well, how interesting. this means we do our OWN request/process
//	


dp = dp.add( '/api',{ docstring:"" 
	}
	,function ( req, res, params ){
		let listy = []
		for( let datasourcekey in adddatasources ){
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

