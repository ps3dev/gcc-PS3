
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_geom_Rectangle2D$Double__
#define __java_awt_geom_Rectangle2D$Double__

#pragma interface

#include <java/awt/geom/Rectangle2D.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
      namespace geom
      {
          class Rectangle2D;
          class Rectangle2D$Double;
      }
    }
  }
}

class java::awt::geom::Rectangle2D$Double : public ::java::awt::geom::Rectangle2D
{

public:
  Rectangle2D$Double();
  Rectangle2D$Double(jdouble, jdouble, jdouble, jdouble);
  virtual jdouble getX();
  virtual jdouble getY();
  virtual jdouble getWidth();
  virtual jdouble getHeight();
  virtual jboolean isEmpty();
  virtual void setRect(jdouble, jdouble, jdouble, jdouble);
  virtual void setRect(::java::awt::geom::Rectangle2D *);
  virtual jint outcode(jdouble, jdouble);
  virtual ::java::awt::geom::Rectangle2D * getBounds2D();
  virtual ::java::awt::geom::Rectangle2D * createIntersection(::java::awt::geom::Rectangle2D *);
  virtual ::java::awt::geom::Rectangle2D * createUnion(::java::awt::geom::Rectangle2D *);
  virtual ::java::lang::String * toString();
  jdouble __attribute__((aligned(__alignof__( ::java::awt::geom::Rectangle2D)))) x;
  jdouble y;
  jdouble width;
  jdouble height;
  static ::java::lang::Class class$;
};

#endif // __java_awt_geom_Rectangle2D$Double__
