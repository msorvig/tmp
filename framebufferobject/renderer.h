#ifndef _RENDERER_
#define _RENDERER_

class HelloWindow;

class Renderer
{
public:
    virtual QSurfaceFormat format() const = 0;
    virtual void setAnimating(HelloWindow *window, bool animating) = 0;
};

#endif
