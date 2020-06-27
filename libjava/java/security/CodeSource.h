
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_security_CodeSource__
#define __java_security_CodeSource__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace net
    {
        class URL;
    }
    namespace security
    {
        class CodeSource;
      namespace cert
      {
          class Certificate;
      }
    }
  }
}

class java::security::CodeSource : public ::java::lang::Object
{

public:
  CodeSource(::java::net::URL *, JArray< ::java::security::cert::Certificate * > *);
  virtual jint hashCode();
  virtual jboolean equals(::java::lang::Object *);
  virtual ::java::net::URL * getLocation();
  virtual JArray< ::java::security::cert::Certificate * > * getCertificates();
  virtual jboolean implies(::java::security::CodeSource *);
  virtual ::java::lang::String * toString();
private:
  void readObject(::java::io::ObjectInputStream *);
  void writeObject(::java::io::ObjectOutputStream *);
  static const jlong serialVersionUID = 4977541819976013951LL;
  ::java::net::URL * __attribute__((aligned(__alignof__( ::java::lang::Object)))) location;
  ::java::util::HashSet * certs;
public:
  static ::java::lang::Class class$;
};

#endif // __java_security_CodeSource__
