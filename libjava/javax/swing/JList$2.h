
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_JList$2__
#define __javax_swing_JList$2__

#pragma interface

#include <javax/swing/AbstractListModel.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
        class JList;
        class JList$2;
    }
  }
}

class javax::swing::JList$2 : public ::javax::swing::AbstractListModel
{

public: // actually package-private
  JList$2(::javax::swing::JList *, ::java::util::Vector *);
public:
  virtual jint getSize();
  virtual ::java::lang::Object * getElementAt(jint);
public: // actually package-private
  ::javax::swing::JList * __attribute__((aligned(__alignof__( ::javax::swing::AbstractListModel)))) this$0;
private:
  ::java::util::Vector * val$items;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_JList$2__
