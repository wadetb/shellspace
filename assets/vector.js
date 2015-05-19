Vec3 = {}

Vec3.create = function( x, y, z ) {
    return new Float32Array( [ x, y, z ] );
} 

Vec3.xAxis = Vec3.create( 1, 0, 0 );
Vec3.yAxis = Vec3.create( 0, 1, 0 );
Vec3.zAxis = Vec3.create( 0, 0, 1 );

Vec3.add = function( a, b, out ) {
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

Vec3.avg = function( a, b, out ) {
    out[0] = ( a[0] + b[0] ) * 0.5;
    out[1] = ( a[1] + b[1] ) * 0.5;
    out[2] = ( a[2] + b[2] ) * 0.5;
}

Vec3.clear = function( out ) {
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
}

Vec3.copy = function( a, out ) {
    out[0] = a[0];
    out[1] = a[1];
    out[2] = a[2];
}

Vec3.cross = function( a, b, out ) {
    var ax = a[0];
    var ay = a[1];
    var az = a[2];

    var bx = b[0];
    var by = b[1];
    var bz = b[2];

    out[0] = ay * bz - az * by;
    out[1] = az * bx - ax * bz;
    out[2] = ax * by - ay * bx;
}

Vec3.distance = function( a, b ) {
    var dx = a[0] - b[0];
    var dy = a[1] - b[1];
    var dz = a[2] - b[2];

    return Math.sqrt( dx*dx + dy*dy + dz*dz );
}

Vec3.dot = function( a, b ) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

Vec3.length = function( a ) {
    return Math.sqrt( a[0]*a[0] + a[1]*a[1] + a[2]*a[2] );
}

Vec3.lengthSqr = function( a ) {
    return a[0]*a[0] + a[1]*a[1] + a[2]*a[2];
}

Vec3.mad = function( a, b, c, out ) {
    out[0] = a[0] + b * c[0];
    out[1] = a[1] + b * c[1];
    out[2] = a[2] + b * c[2];
}

Vec3.mul = function( a, b, out ) {
    out[0] = a[0] * b[0];
    out[1] = a[1] * b[1];
    out[2] = a[2] * b[2];
}

Vec3.negate = function( a, out ) {
    out[0] = -a[0];
    out[1] = -a[1];
    out[2] = -a[2];
}

Vec3.normalize = function( a, out ) {
    var l = Math.sqrt( a[0]*a[0] + a[1]*a[1] + a[2]*a[2] );

    if ( l > 0.000001 ) {
        out[0] = a[0] * l;
        out[1] = a[1] * l;
        out[2] = a[2] * l;
    } else {
        out[0] = 0;
        out[1] = 0;
        out[2] = 1;
    }
}

Vec3.reciprocal = function( a, out ) {
    out[0] = 1.0 / a[0];
    out[1] = 1.0 / a[1];
    out[2] = 1.0 / a[2];
}

Vec3.scale = function( a, v, out ) {
    out[0] = a[0] * v;
    out[1] = a[1] * v;
    out[2] = a[2] * v;
}

Vec3.set = function( out, x, y, z ) {
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

Vec3.sub = function( a, b, out ) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}
