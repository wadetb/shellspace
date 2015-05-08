#include "common.h"
#include "skia.h"
#include "SkExample.h"
#include "SkDevice.h"

class HelloTutorial : public SkExample {
    public:
        HelloTutorial(SkExampleWindow* window)
            : SkExample(window)
        {
            fName = "Tutorial";  // This is how Skia will find your example.
            
            fWindow->setupBackend(SkExampleWindow::kGPU_DeviceType);
           // Another option is the CPU backend:  fWindow->setupBackend(kRaster_DeviceType);
        }

    protected:
        virtual void draw(SkCanvas* canvas) SK_OVERRIDE {
            // Clear background
            canvas->drawColor(SK_ColorWHITE);

            SkPaint paint;
            // Draw a message with a nice black paint.
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setColor(SK_ColorBLACK);
            paint.setTextSize(SkIntToScalar(20));

            static const char message[] = "Hello World!";

            // Translate and draw the text:
            canvas->save();
            canvas->translate(SkIntToScalar(50), SkIntToScalar(100));
            canvas->drawText(message, strlen(message), SkIntToScalar(0), SkIntToScalar(0), paint);
            canvas->restore();

            // If you ever want to do animation. Use the inval method to trigger a redraw.
            this->fWindow->inval(NULL);
        }
};

static SkExample* MyFactory(SkExampleWindow* window) {
    return new HelloTutorial(window);
}
static SkExample::Registry registry(MyFactory);