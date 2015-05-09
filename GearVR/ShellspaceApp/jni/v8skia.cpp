#include "common.h"

#include <include/v8.h>
#include <include/libplatform/libplatform.h>

#include <core/SkBitmap.h>
#include <core/SkCanvas.h>
#include <core/SkError.h>

using namespace v8;


void V8_Throw( Isolate *isolate, const char *format, ... );


namespace node {

    class ObjectWrap {
    public:
      ObjectWrap() {
        refs_ = 0;
    }


    virtual ~ObjectWrap() {
        if (persistent().IsEmpty())
          return;
      assert(persistent().IsNearDeath());
      persistent().ClearWeak();
      persistent().Reset();
  }


  template <class T>
  static inline T* Unwrap(v8::Handle<v8::Object> handle) {
    assert(!handle.IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    // Cast to ObjectWrap before casting to T.  A direct cast from void
    // to T won't work right when T has more than one base class.
    void* ptr = handle->GetAlignedPointerFromInternalField(0);
    ObjectWrap* wrap = static_cast<ObjectWrap*>(ptr);
    return static_cast<T*>(wrap);
}


inline v8::Local<v8::Object> handle() {
    return handle(v8::Isolate::GetCurrent());
}


inline v8::Local<v8::Object> handle(v8::Isolate* isolate) {
    return v8::Local<v8::Object>::New(isolate, persistent());
}


inline v8::Persistent<v8::Object>& persistent() {
    return handle_;
}


protected:
  inline void Wrap(v8::Handle<v8::Object> handle) {
    assert(persistent().IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    handle->SetAlignedPointerInInternalField(0, this);
    persistent().Reset(v8::Isolate::GetCurrent(), handle);
    MakeWeak();
}


inline void MakeWeak(void) {
    persistent().SetWeak(this, WeakCallback);
    persistent().MarkIndependent();
}

  /* Ref() marks the object as being attached to an event loop.
   * Refed objects will not be garbage collected, even if
   * all references are lost.
   */
   virtual void Ref() {
    assert(!persistent().IsEmpty());
    persistent().ClearWeak();
    refs_++;
}

  /* Unref() marks an object as detached from the event loop.  This is its
   * default state.  When an object with a "weak" reference changes from
   * attached to detached state it will be freed. Be careful not to access
   * the object after making this call as it might be gone!
   * (A "weak reference" means an object that only has a
   * persistant handle.)
   *
   * DO NOT CALL THIS FROM DESTRUCTOR
   */
   virtual void Unref() {
    assert(!persistent().IsEmpty());
    assert(!persistent().IsWeak());
    assert(refs_ > 0);
    if (--refs_ == 0)
      MakeWeak();
}

  int refs_;  // ro

private:
  static void WeakCallback(
      const v8::WeakCallbackData<v8::Object, ObjectWrap>& data) {
    v8::Isolate* isolate = data.GetIsolate();
    v8::HandleScope scope(isolate);
    ObjectWrap* wrap = data.GetParameter();
    assert(wrap->refs_ == 0);
    assert(wrap->handle_.IsNearDeath());
    assert(
        data.GetValue() == v8::Local<v8::Object>::New(isolate, wrap->handle_));
    wrap->handle_.Reset();
    delete wrap;
}

v8::Persistent<v8::Object> handle_;
};

}  // namespace node


class V8SkBitmap : public node::ObjectWrap 
{
public:
    static void Initialize( Isolate *isolate, Handle<ObjectTemplate> target );
    static void New( const FunctionCallbackInfo<Value>& args );
    static void AllocPixels( const FunctionCallbackInfo<Value>& args );
    static void SetInfo( const FunctionCallbackInfo<Value>& args );

public:
    V8SkBitmap() {
        bitmap_ = new SkBitmap();
    }

    ~V8SkBitmap() {
        delete bitmap_;
    }

    SkBitmap *GetSkBitmap() {
        return bitmap_;
    }

private:
    SkBitmap* bitmap_;
};


void V8SkBitmap::New( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkBitmap* bitmap = new V8SkBitmap();
    bitmap->Wrap( args.This() );

    args.GetReturnValue().Set( args.This() );
}


void V8SkBitmap::AllocPixels( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkBitmap* bitmapWrap = ObjectWrap::Unwrap<V8SkBitmap>( args.This() );
    SkBitmap *bitmap = bitmapWrap->GetSkBitmap();

    bitmap->allocPixels();
}


SkImageInfo V8Skia_MakeImageInfo( Isolate *isolate, const Handle<Value>& object )
{
    Handle<Array> array = Handle<Array>::Cast( object );

    int width = array->Get( String::NewFromUtf8( isolate, "width" ) )->Int32Value();
    int height = array->Get( String::NewFromUtf8( isolate, "height" ) )->Int32Value();

    Handle<Value> colorTypeValue = array->Get( String::NewFromUtf8( isolate, "colorType" ) );
    SkColorType colorType = colorTypeValue->IsUndefined() ? 
        kN32_SkColorType : (SkColorType)colorTypeValue->Int32Value();

    Handle<Value> alphaTypeValue = array->Get( String::NewFromUtf8( isolate, "alphaType" ) );
    SkAlphaType alphaType = alphaTypeValue->IsUndefined() ? 
        kOpaque_SkAlphaType : (SkAlphaType)alphaTypeValue->Int32Value();

    Handle<Value> profileTypeValue = array->Get( String::NewFromUtf8( isolate, "profileType" ) );
    SkColorProfileType profileType = profileTypeValue->IsUndefined() ? 
        kLinear_SkColorProfileType : (SkColorProfileType)profileTypeValue->Int32Value();

    // return SkImageInfo::MakeN32( 400, 300, kOpaque_SkAlphaType );
    return SkImageInfo::Make( width, height, colorType, alphaType, profileType );
}


void V8SkBitmap::SetInfo( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkBitmap* bitmapWrap = ObjectWrap::Unwrap<V8SkBitmap>( args.This() );
    SkBitmap *bitmap = bitmapWrap->GetSkBitmap();

    bitmap->setInfo( 
        V8Skia_MakeImageInfo( args.GetIsolate(), args[0] ) );
}


void V8SkBitmap::Initialize( Isolate *isolate, Handle<ObjectTemplate> target ) 
{
    HandleScope scope( isolate );
 
    Local<FunctionTemplate> bitmapTemplate = FunctionTemplate::New( isolate, New );
    bitmapTemplate->SetClassName( String::NewFromUtf8( isolate, "Bitmap" ) );
    bitmapTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );

    bitmapTemplate->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "allocPixels" ),
        FunctionTemplate::New( isolate, AllocPixels ) );
    
    bitmapTemplate->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "setInfo" ),
        FunctionTemplate::New( isolate, SetInfo ) );

    target->Set( 
        String::NewFromUtf8( isolate, "Bitmap" ), 
        bitmapTemplate );
}


SkBitmap *V8_UnwrapBitmap( Isolate *isolate, const Handle<Value> value )
{
    String::Utf8Value constructor( value->ToObject()->GetConstructorName() );
    if ( *constructor == NULL || !S_streq( *constructor, "Bitmap" ) )
    {
        V8_Throw( isolate, "Expected a Bitmap object" );
        return NULL;
    }

    V8SkBitmap* bitmapWrap = node::ObjectWrap::Unwrap<V8SkBitmap>( value->ToObject() );
    SkBitmap *bitmap = bitmapWrap->GetSkBitmap();

    return bitmap;
}


class V8SkPaint : public node::ObjectWrap 
{
public:
    static void Initialize( Isolate *isolate, Handle<ObjectTemplate> target );
    static void New( const FunctionCallbackInfo<Value>& args );
    static void SetAntiAlias( const FunctionCallbackInfo<Value>& args );
    static void SetTextSize( const FunctionCallbackInfo<Value>& args );
    static void SetFlags( const FunctionCallbackInfo<Value>& args );
    static void SetColor( const FunctionCallbackInfo<Value>& args );

public:
    V8SkPaint() {
        paint_ = new SkPaint();
    }

    ~V8SkPaint() {
        delete paint_;
    }

    SkPaint *GetSkPaint() {
        return paint_;
    }

private:
    SkPaint* paint_;
};


void V8SkPaint::New( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkPaint* paint = new V8SkPaint();
    paint->Wrap( args.This() );

    args.GetReturnValue().Set( args.This() );
}


void V8SkPaint::SetAntiAlias( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkPaint* paintWrap = ObjectWrap::Unwrap<V8SkPaint>( args.This() );
    SkPaint *paint = paintWrap->GetSkPaint();

    paint->setAntiAlias(
        args[0]->ToBoolean()->Value() );
}


void V8SkPaint::SetTextSize( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkPaint* paintWrap = ObjectWrap::Unwrap<V8SkPaint>( args.This() );
    SkPaint *paint = paintWrap->GetSkPaint();

    paint->setTextSize(
        args[0]->ToUint32()->Value() );
}


void V8SkPaint::SetFlags( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkPaint* paintWrap = ObjectWrap::Unwrap<V8SkPaint>( args.This() );
    SkPaint *paint = paintWrap->GetSkPaint();

    paint->setFlags(
        args[0]->ToUint32()->Value() );
}


SkColor V8Skia_MakeColor( Isolate *isolate, const Handle<Value>& object )
{
    Handle<Array> array = Handle<Array>::Cast( object );

    int r = array->Get( 0 )->Int32Value();
    int g = array->Get( 1 )->Int32Value();
    int b = array->Get( 2 )->Int32Value();

    Handle<Value> alphaValue = array->Get( Integer::New( isolate, 3 ) );
    int a = alphaValue->IsUndefined() ?
        255 : alphaValue->Int32Value();

    return SkColorSetARGB( a, r, g, b );
}


void V8SkPaint::SetColor( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkPaint* paintWrap = ObjectWrap::Unwrap<V8SkPaint>( args.This() );
    SkPaint *paint = paintWrap->GetSkPaint();

    paint->setFlags(
        V8Skia_MakeColor( args.GetIsolate(), args[0] ) );
}


void V8SkPaint::Initialize(Isolate *isolate, Handle<ObjectTemplate> target) {

    Local<FunctionTemplate> tmpl = FunctionTemplate::New( isolate, New );
    tmpl->SetClassName( String::NewFromUtf8( isolate, "Paint" ) );
    tmpl->InstanceTemplate()->SetInternalFieldCount( 1 );

    tmpl->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "setAntiAlias" ),
        FunctionTemplate::New( isolate, SetAntiAlias ) );
    
    tmpl->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "setTextSize" ),
        FunctionTemplate::New( isolate, SetTextSize ) );
    
    tmpl->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "setFlags" ),
        FunctionTemplate::New( isolate, SetTextSize ) );
    
    tmpl->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "setColor" ),
        FunctionTemplate::New( isolate, SetTextSize ) );
    
    target->Set( String::NewFromUtf8( isolate, "Paint" ), tmpl );
}


SkPaint *V8_UnwrapPaint( Isolate *isolate, const Handle<Value> value )
{
    String::Utf8Value constructor( value->ToObject()->GetConstructorName() );
    if ( *constructor == NULL || !S_streq( *constructor, "Paint" ) )
    {
        V8_Throw( isolate, "Expected a Paint object" );
        return NULL;
    }

    V8SkPaint* paintWrap = node::ObjectWrap::Unwrap<V8SkPaint>( value->ToObject() );
    SkPaint *paint = paintWrap->GetSkPaint();

    return paint;
}


class V8SkCanvas : public node::ObjectWrap 
{
public:
    static void Initialize( Isolate *isolate, Handle<ObjectTemplate> target );
    static void New( const FunctionCallbackInfo<Value>& args );
    static void Save( const FunctionCallbackInfo<Value>& args );
    static void Restore( const FunctionCallbackInfo<Value>& args );
    static void Translate( const FunctionCallbackInfo<Value>& args );
    static void DrawColor( const FunctionCallbackInfo<Value>& args );
    static void DrawText( const FunctionCallbackInfo<Value>& args );

public:
    V8SkCanvas( SkBitmap *bitmap ) {
        canvas_ = new SkCanvas( *bitmap );
    }

    ~V8SkCanvas() {
        delete canvas_;
    }

    SkCanvas *GetSkCanvas() {
        return canvas_;
    }

private:
    SkCanvas* canvas_;
};


void V8SkCanvas::New( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    SkBitmap *bitmap = V8_UnwrapBitmap( args.GetIsolate(), args[0] );
    if ( !bitmap )
        return;

    V8SkCanvas* canvas = new V8SkCanvas( bitmap );
    canvas->Wrap( args.This() );

    args.GetReturnValue().Set( args.This() );
}


void V8SkCanvas::Save( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkCanvas* canvasWrap = ObjectWrap::Unwrap<V8SkCanvas>( args.This() );
    SkCanvas *canvas = canvasWrap->GetSkCanvas();

    canvas->save();
}


void V8SkCanvas::Restore( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkCanvas* canvasWrap = ObjectWrap::Unwrap<V8SkCanvas>( args.This() );
    SkCanvas *canvas = canvasWrap->GetSkCanvas();

    canvas->restore();
}


void V8SkCanvas::Translate( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkCanvas* canvasWrap = ObjectWrap::Unwrap<V8SkCanvas>( args.This() );
    SkCanvas *canvas = canvasWrap->GetSkCanvas();

    canvas->translate( 
        args[0]->Int32Value(), 
        args[1]->Int32Value() );
}


void V8SkCanvas::DrawColor( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkCanvas* canvasWrap = ObjectWrap::Unwrap<V8SkCanvas>( args.This() );
    SkCanvas *canvas = canvasWrap->GetSkCanvas();

    canvas->drawColor( 
        args[0]->Uint32Value() );
}


void V8SkCanvas::DrawText( const FunctionCallbackInfo<Value>& args ) 
{
    HandleScope scope( args.GetIsolate() );

    V8SkCanvas* canvasWrap = ObjectWrap::Unwrap<V8SkCanvas>( args.This() );
    SkCanvas *canvas = canvasWrap->GetSkCanvas();

    String::Utf8Value arg0( args[0] );
    const char *arg0Str = *arg0 ? *arg0 : "<not a string>";

    SkPaint *paint = V8_UnwrapPaint( args.GetIsolate(), args[3] );
    if ( !paint )
        return;

    canvas->drawText(
        arg0Str,
        strlen( arg0Str ),
        args[1]->Int32Value(),
        args[2]->Int32Value(),
        *paint );
}


void V8SkCanvas::Initialize(Isolate *isolate, Handle<ObjectTemplate> target) {

    Local<FunctionTemplate> tmpl = FunctionTemplate::New( isolate, New );
    tmpl->SetClassName( String::NewFromUtf8( isolate, "Canvas" ) );
    tmpl->InstanceTemplate()->SetInternalFieldCount( 1 );

    tmpl->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "save" ),
        FunctionTemplate::New( isolate, Save ) );
    
    tmpl->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "restore" ),
        FunctionTemplate::New( isolate, Restore ) );
    
    tmpl->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "translate" ),
        FunctionTemplate::New( isolate, Translate ) );

    tmpl->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "drawColor" ),
        FunctionTemplate::New( isolate, DrawColor ) );

    tmpl->PrototypeTemplate()->Set(
        String::NewFromUtf8( isolate, "drawText" ),
        FunctionTemplate::New( isolate, DrawText ) );

    target->Set( String::NewFromUtf8( isolate, "Canvas" ), tmpl );
}


void V8Skia_Error( SkError error, void *context )
{
    LOG( SkGetLastErrorString() );
}


void V8Skia_Init( Isolate *isolate, Handle<ObjectTemplate> target )
{
    SkSetErrorCallback( V8Skia_Error, NULL );

    V8SkBitmap::Initialize( isolate, target );
    V8SkPaint::Initialize( isolate, target );
    V8SkCanvas::Initialize( isolate, target );
}

