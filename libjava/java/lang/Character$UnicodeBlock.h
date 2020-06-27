
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_lang_Character$UnicodeBlock__
#define __java_lang_Character$UnicodeBlock__

#pragma interface

#include <java/lang/Character$Subset.h>
#include <gcj/array.h>


class java::lang::Character$UnicodeBlock : public ::java::lang::Character$Subset
{

  Character$UnicodeBlock(jint, jint, ::java::lang::String *, ::java::lang::String *);
public:
  static ::java::lang::Character$UnicodeBlock * of(jchar);
  static ::java::lang::Character$UnicodeBlock * of(jint);
  static ::java::lang::Character$UnicodeBlock * forName(::java::lang::String *);
public: // actually package-private
  static JArray< jint > * $SWITCH_TABLE$java$lang$Character$UnicodeBlock$NameType();
private:
  jint __attribute__((aligned(__alignof__( ::java::lang::Character$Subset)))) start;
  jint end;
  ::java::lang::String * canonicalName;
public:
  static ::java::lang::Character$UnicodeBlock * BASIC_LATIN;
  static ::java::lang::Character$UnicodeBlock * LATIN_1_SUPPLEMENT;
  static ::java::lang::Character$UnicodeBlock * LATIN_EXTENDED_A;
  static ::java::lang::Character$UnicodeBlock * LATIN_EXTENDED_B;
  static ::java::lang::Character$UnicodeBlock * IPA_EXTENSIONS;
  static ::java::lang::Character$UnicodeBlock * SPACING_MODIFIER_LETTERS;
  static ::java::lang::Character$UnicodeBlock * COMBINING_DIACRITICAL_MARKS;
  static ::java::lang::Character$UnicodeBlock * GREEK;
  static ::java::lang::Character$UnicodeBlock * CYRILLIC;
  static ::java::lang::Character$UnicodeBlock * CYRILLIC_SUPPLEMENTARY;
  static ::java::lang::Character$UnicodeBlock * ARMENIAN;
  static ::java::lang::Character$UnicodeBlock * HEBREW;
  static ::java::lang::Character$UnicodeBlock * ARABIC;
  static ::java::lang::Character$UnicodeBlock * SYRIAC;
  static ::java::lang::Character$UnicodeBlock * THAANA;
  static ::java::lang::Character$UnicodeBlock * DEVANAGARI;
  static ::java::lang::Character$UnicodeBlock * BENGALI;
  static ::java::lang::Character$UnicodeBlock * GURMUKHI;
  static ::java::lang::Character$UnicodeBlock * GUJARATI;
  static ::java::lang::Character$UnicodeBlock * ORIYA;
  static ::java::lang::Character$UnicodeBlock * TAMIL;
  static ::java::lang::Character$UnicodeBlock * TELUGU;
  static ::java::lang::Character$UnicodeBlock * KANNADA;
  static ::java::lang::Character$UnicodeBlock * MALAYALAM;
  static ::java::lang::Character$UnicodeBlock * SINHALA;
  static ::java::lang::Character$UnicodeBlock * THAI;
  static ::java::lang::Character$UnicodeBlock * LAO;
  static ::java::lang::Character$UnicodeBlock * TIBETAN;
  static ::java::lang::Character$UnicodeBlock * MYANMAR;
  static ::java::lang::Character$UnicodeBlock * GEORGIAN;
  static ::java::lang::Character$UnicodeBlock * HANGUL_JAMO;
  static ::java::lang::Character$UnicodeBlock * ETHIOPIC;
  static ::java::lang::Character$UnicodeBlock * CHEROKEE;
  static ::java::lang::Character$UnicodeBlock * UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS;
  static ::java::lang::Character$UnicodeBlock * OGHAM;
  static ::java::lang::Character$UnicodeBlock * RUNIC;
  static ::java::lang::Character$UnicodeBlock * TAGALOG;
  static ::java::lang::Character$UnicodeBlock * HANUNOO;
  static ::java::lang::Character$UnicodeBlock * BUHID;
  static ::java::lang::Character$UnicodeBlock * TAGBANWA;
  static ::java::lang::Character$UnicodeBlock * KHMER;
  static ::java::lang::Character$UnicodeBlock * MONGOLIAN;
  static ::java::lang::Character$UnicodeBlock * LIMBU;
  static ::java::lang::Character$UnicodeBlock * TAI_LE;
  static ::java::lang::Character$UnicodeBlock * KHMER_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * PHONETIC_EXTENSIONS;
  static ::java::lang::Character$UnicodeBlock * LATIN_EXTENDED_ADDITIONAL;
  static ::java::lang::Character$UnicodeBlock * GREEK_EXTENDED;
  static ::java::lang::Character$UnicodeBlock * GENERAL_PUNCTUATION;
  static ::java::lang::Character$UnicodeBlock * SUPERSCRIPTS_AND_SUBSCRIPTS;
  static ::java::lang::Character$UnicodeBlock * CURRENCY_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * COMBINING_MARKS_FOR_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * LETTERLIKE_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * NUMBER_FORMS;
  static ::java::lang::Character$UnicodeBlock * ARROWS;
  static ::java::lang::Character$UnicodeBlock * MATHEMATICAL_OPERATORS;
  static ::java::lang::Character$UnicodeBlock * MISCELLANEOUS_TECHNICAL;
  static ::java::lang::Character$UnicodeBlock * CONTROL_PICTURES;
  static ::java::lang::Character$UnicodeBlock * OPTICAL_CHARACTER_RECOGNITION;
  static ::java::lang::Character$UnicodeBlock * ENCLOSED_ALPHANUMERICS;
  static ::java::lang::Character$UnicodeBlock * BOX_DRAWING;
  static ::java::lang::Character$UnicodeBlock * BLOCK_ELEMENTS;
  static ::java::lang::Character$UnicodeBlock * GEOMETRIC_SHAPES;
  static ::java::lang::Character$UnicodeBlock * MISCELLANEOUS_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * DINGBATS;
  static ::java::lang::Character$UnicodeBlock * MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A;
  static ::java::lang::Character$UnicodeBlock * SUPPLEMENTAL_ARROWS_A;
  static ::java::lang::Character$UnicodeBlock * BRAILLE_PATTERNS;
  static ::java::lang::Character$UnicodeBlock * SUPPLEMENTAL_ARROWS_B;
  static ::java::lang::Character$UnicodeBlock * MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B;
  static ::java::lang::Character$UnicodeBlock * SUPPLEMENTAL_MATHEMATICAL_OPERATORS;
  static ::java::lang::Character$UnicodeBlock * MISCELLANEOUS_SYMBOLS_AND_ARROWS;
  static ::java::lang::Character$UnicodeBlock * CJK_RADICALS_SUPPLEMENT;
  static ::java::lang::Character$UnicodeBlock * KANGXI_RADICALS;
  static ::java::lang::Character$UnicodeBlock * IDEOGRAPHIC_DESCRIPTION_CHARACTERS;
  static ::java::lang::Character$UnicodeBlock * CJK_SYMBOLS_AND_PUNCTUATION;
  static ::java::lang::Character$UnicodeBlock * HIRAGANA;
  static ::java::lang::Character$UnicodeBlock * KATAKANA;
  static ::java::lang::Character$UnicodeBlock * BOPOMOFO;
  static ::java::lang::Character$UnicodeBlock * HANGUL_COMPATIBILITY_JAMO;
  static ::java::lang::Character$UnicodeBlock * KANBUN;
  static ::java::lang::Character$UnicodeBlock * BOPOMOFO_EXTENDED;
  static ::java::lang::Character$UnicodeBlock * KATAKANA_PHONETIC_EXTENSIONS;
  static ::java::lang::Character$UnicodeBlock * ENCLOSED_CJK_LETTERS_AND_MONTHS;
  static ::java::lang::Character$UnicodeBlock * CJK_COMPATIBILITY;
  static ::java::lang::Character$UnicodeBlock * CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A;
  static ::java::lang::Character$UnicodeBlock * YIJING_HEXAGRAM_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * CJK_UNIFIED_IDEOGRAPHS;
  static ::java::lang::Character$UnicodeBlock * YI_SYLLABLES;
  static ::java::lang::Character$UnicodeBlock * YI_RADICALS;
  static ::java::lang::Character$UnicodeBlock * HANGUL_SYLLABLES;
  static ::java::lang::Character$UnicodeBlock * HIGH_SURROGATES;
  static ::java::lang::Character$UnicodeBlock * HIGH_PRIVATE_USE_SURROGATES;
  static ::java::lang::Character$UnicodeBlock * LOW_SURROGATES;
  static ::java::lang::Character$UnicodeBlock * PRIVATE_USE_AREA;
  static ::java::lang::Character$UnicodeBlock * CJK_COMPATIBILITY_IDEOGRAPHS;
  static ::java::lang::Character$UnicodeBlock * ALPHABETIC_PRESENTATION_FORMS;
  static ::java::lang::Character$UnicodeBlock * ARABIC_PRESENTATION_FORMS_A;
  static ::java::lang::Character$UnicodeBlock * VARIATION_SELECTORS;
  static ::java::lang::Character$UnicodeBlock * COMBINING_HALF_MARKS;
  static ::java::lang::Character$UnicodeBlock * CJK_COMPATIBILITY_FORMS;
  static ::java::lang::Character$UnicodeBlock * SMALL_FORM_VARIANTS;
  static ::java::lang::Character$UnicodeBlock * ARABIC_PRESENTATION_FORMS_B;
  static ::java::lang::Character$UnicodeBlock * HALFWIDTH_AND_FULLWIDTH_FORMS;
  static ::java::lang::Character$UnicodeBlock * SPECIALS;
  static ::java::lang::Character$UnicodeBlock * LINEAR_B_SYLLABARY;
  static ::java::lang::Character$UnicodeBlock * LINEAR_B_IDEOGRAMS;
  static ::java::lang::Character$UnicodeBlock * AEGEAN_NUMBERS;
  static ::java::lang::Character$UnicodeBlock * OLD_ITALIC;
  static ::java::lang::Character$UnicodeBlock * GOTHIC;
  static ::java::lang::Character$UnicodeBlock * UGARITIC;
  static ::java::lang::Character$UnicodeBlock * DESERET;
  static ::java::lang::Character$UnicodeBlock * SHAVIAN;
  static ::java::lang::Character$UnicodeBlock * OSMANYA;
  static ::java::lang::Character$UnicodeBlock * CYPRIOT_SYLLABARY;
  static ::java::lang::Character$UnicodeBlock * BYZANTINE_MUSICAL_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * MUSICAL_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * TAI_XUAN_JING_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * MATHEMATICAL_ALPHANUMERIC_SYMBOLS;
  static ::java::lang::Character$UnicodeBlock * CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B;
  static ::java::lang::Character$UnicodeBlock * CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT;
  static ::java::lang::Character$UnicodeBlock * TAGS;
  static ::java::lang::Character$UnicodeBlock * VARIATION_SELECTORS_SUPPLEMENT;
  static ::java::lang::Character$UnicodeBlock * SUPPLEMENTARY_PRIVATE_USE_AREA_A;
  static ::java::lang::Character$UnicodeBlock * SUPPLEMENTARY_PRIVATE_USE_AREA_B;
  static ::java::lang::Character$UnicodeBlock * SURROGATES_AREA;
private:
  static JArray< ::java::lang::Character$UnicodeBlock * > * sets;
  static JArray< jint > * $SWITCH_TABLE$java$lang$Character$UnicodeBlock$NameType__;
public:
  static ::java::lang::Class class$;
};

#endif // __java_lang_Character$UnicodeBlock__
