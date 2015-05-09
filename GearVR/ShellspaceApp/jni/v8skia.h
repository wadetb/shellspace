#ifndef __V8SKIA_H__
#define __V8SKIA_H__

void V8Skia_Init( Isolate *isolate, Handle<ObjectTemplate> target );

SkBitmap *V8_UnwrapBitmap( Isolate *isolate, const Handle<Value> value );

#endif
