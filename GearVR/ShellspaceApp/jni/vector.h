/*
    Shellspace - One tiny step towards the VR Desktop Operating System
    Copyright (C) 2015  Wade Brainerd

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef __MATH_H__
#define __MATH_H__

struct SxVector4
{
    float   x;
    float   y;
    float   z;
    float   w;
};

struct SxAxes
{
    SxVector3   x;
    SxVector3   y;
    SxVector3   z;
};

struct SxTransform
{
    SxAxes      axes;
    SxVector3   origin;
    SxVector3   scale;
};

inline void Vec3Add( const SxVector3 &a, const SxVector3 &b, SxVector3 *out )
{
    out->x = a.x + b.x;
    out->y = a.y + b.y;
    out->z = a.z + b.z;
}

inline void Vec3Clear( SxVector3 *out )
{
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
}

inline void Vec3Copy( const SxVector3 &a, SxVector3 *out )
{
    out->x = a.x;
    out->y = a.y;
    out->z = a.z;
}

inline void Vec3Mad( const SxVector3 &a, float b, const SxVector3 &c, SxVector3 *out )
{
    out->x = a.x * b * c.x;
    out->y = a.y * b * c.y;
    out->z = a.z * b * c.z;
}

inline void Vec3Mul( const SxVector3 &a, const SxVector3 &b, SxVector3 *out )
{
    out->x = a.x * b.x;
    out->y = a.y * b.y;
    out->z = a.z * b.z;
}

inline void Vec3Reciprocal( const SxVector3 &a, SxVector3 *out )
{
    out->x = 1.0f / a.x;
    out->y = 1.0f / a.y;
    out->z = 1.0f / a.z;
}

inline void Vec3Scale( const SxVector3 &a, float b, SxVector3 *out )
{
    out->x = a.x * b;
    out->y = a.y * b;
    out->z = a.z * b;
}

inline void Vec3Set( SxVector3 *out, float x, float y, float z )
{
    out->x = x;
    out->y = y;
    out->z = z;
}

inline void Vec3Sub( const SxVector3 &a, const SxVector3 &b, SxVector3 *out )
{
    out->x = a.x - b.x;
    out->y = a.y - b.y;
    out->z = a.z - b.z;
}

inline void Vec3TransformByAxes( const SxVector3 &a, const SxAxes &b, SxVector3 *out )
{
    assert( out != &a );
    out->x = a.x * b.x.x + a.y * b.y.x + a.z * b.z.x;
    out->y = a.x * b.x.y + a.y * b.y.y + a.z * b.z.y;
    out->z = a.x * b.x.z + a.y * b.y.z + a.z * b.z.z;
}

inline void Vec3Transform( const SxVector3 &a, const SxTransform &b, SxVector3 *out )
{
    SxVector3 result;

    Vec3TransformByAxes( a, b.axes, &result );
    Vec3Mul( result, b.scale, &result );

    *out = result;
}

inline void Vec3TransformPoint( const SxVector3 &a, const SxTransform &b, SxVector3 *out )
{
    SxVector3 result;

    Vec3TransformByAxes( a, b.axes, &result );
    Vec3Mul( result, b.scale, &result );
    Vec3Add( result, b.origin, &result );

    *out = result;
}

inline void IdentityAxes( SxAxes *out )
{
    Vec3Set( &out->x, 1.0f, 0.0f, 0.0f );
    Vec3Set( &out->y, 0.0f, 1.0f, 0.0f );
    Vec3Set( &out->z, 0.0f, 0.0f, 1.0f );
}

inline void ConcatenateAxes( const SxAxes &a, const SxAxes &b, SxAxes *out )
{
    Vec3TransformByAxes( b.x, a, &out->x );
    Vec3TransformByAxes( b.y, a, &out->y );
    Vec3TransformByAxes( b.z, a, &out->z );
}

inline void IdentityTransform( SxTransform *out )
{
    IdentityAxes( &out->axes );
    Vec3Clear( &out->origin );
    Vec3Set( &out->scale, 1.0f, 1.0f, 1.0f );
}

inline void ConcatenateTransforms( const SxTransform &a, const SxTransform &b, SxTransform *out )
{
    ConcatenateAxes( b.axes, a.axes, &out->axes );
    Vec3TransformPoint( b.origin, a, &out->origin );    
    Vec3Mul( b.scale, a.scale, &out->scale );
}

inline void AnglesClear( SxAngles *out )
{
    out->yaw = 0.0f;
    out->pitch = 0.0f;
    out->roll = 0.0f;
}

inline void AnglesToAxes( const SxAngles &a, SxAxes *out )
{
    float   sy, sp, sr;
    float   cy, cp, cr;

    S_SinCos( a.yaw * (S_PI / 180.0f), &sy, &cy );
    S_SinCos( a.pitch * (S_PI / 180.0f), &sp, &cp );
    S_SinCos( a.roll * (S_PI / 180.0f), &sr, &cr );

    Vec3Set( &out->x, cp*cr, cp*sr, -sp );
    Vec3Set( &out->y, sy*sp*cr + -cy*sr, sy*sp*sr + cy*cr, sy*cp );
    Vec3Set( &out->z, cy*sp*cr + -sy*-sr, cy*sp*sr + -sy*cr, cy*cp );
}

inline void IdentityOrientation( SxOrientation *out )
{
    Vec3Clear( &out->origin );
    AnglesClear( &out->angles );
    Vec3Set( &out->scale, 1.0f, 1.0f, 1.0f );
}

inline void OrientationToTransform( const SxOrientation &a, SxTransform *out )
{
    AnglesToAxes( a.angles, &out->axes );
    Vec3Copy( a.origin, &out->origin );
    Vec3Copy( a.scale, &out->scale );
}

#endif
