#ifndef __MATH_H__
#define __MATH_H__

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

inline void Vec3Sub( const SxVector3 &a, const SxVector3 &b, SxVector3 *out )
{
    out->x = a.x - b.x;
    out->y = a.y - b.y;
    out->z = a.z - b.z;
}

inline void IdentityOrientation( SxOrientation *o )
{
    Vec3Clear( &o->origin );
    Vec3Clear( &o->angles );
    Vec3Clear( &o->scale );
}

#endif
