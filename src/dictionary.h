//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2012 the Regents of the University of California
#ifndef IBIS_DICTIONARY_H
#define IBIS_DICTIONARY_H
///@file
/// Define a dictionary data structure used by ibis::category.
#include "util.h"
#include "array_t.h"

/// Provide a dual-directional mapping between strings and integers.  A
/// utility class used by ibis::category.  Both the NULL string and the
/// empty string are mapped to 0.
///
/// @note If FASTBIT_CASE_SENSITIVE_COMPARE is defined to be 0, the values
/// stored in a dictionary will be folded to the upper case.  This will
/// allow the words in the dictionary to be stored in a simple sorted
/// order.  By default, the dictionary is case sensitive.
class FASTBIT_CXX_DLLSPEC ibis::dictionary {
public:
    ~dictionary() {clear();}
    dictionary(const dictionary& dic);
    /// Default constructor.  Generates one (NULL) entry.
    dictionary() : raw_(1) {raw_[0] = 0;}

    /// Return the number of valid (not null) strings in the dictionary.
    uint32_t size() const {return key_.size();}

    const char* operator[](uint32_t i) const;
    uint32_t operator[](const char* str) const;
    const char* find(const char* str) const;
    void patternSearch(const char* pat, array_t<uint32_t>& matches) const;

    uint32_t insert(const char* str);
    uint32_t insertRaw(char* str);

    void clear();
    void swap(dictionary&);

    int  read(const char* name);
    int  write(const char* name) const;

    void sort(array_t<uint32_t>&);
    int  merge(const dictionary&);
    int  morph(const dictionary&, array_t<uint32_t>&) const;

    bool equal_to(const ibis::dictionary&) const;

    void copy(const dictionary& rhs);

protected:

    /// Member variable raw_ contains the string values in the order of the
    /// code assignment.
    array_t<const char*> raw_;
    /// Member variable key_ contains the string values in alphabetic order.
    array_t<const char*> key_;
    /// Member variable code_ contains the integer code for each string in
    /// key_.
    array_t<uint32_t> code_;
    /// Member varaible buffer_ contains a list of pointers to the memory
    /// that holds the strings.
    array_t<char*> buffer_;

    int readRaw(const char*, FILE *);
    int readKeys(const char*, FILE *);

private:
    dictionary& operator=(const dictionary&);
}; // ibis::dictionary

/// Swap the content of two dictionaries.
inline void ibis::dictionary::swap(ibis::dictionary& rhs) {
    raw_.swap(rhs.raw_);
    key_.swap(rhs.key_);
    code_.swap(rhs.code_);
    buffer_.swap(rhs.buffer_);
} // ibis::dictionary::swap

/// Return a string corresponding to the integer.  If the index is beyond
/// the valid range, i.e., i > size(), then a null pointer will be
/// returned.
inline const char* ibis::dictionary::operator[](uint32_t i) const {
    return (i < raw_.size() ? raw_[i] : raw_[0]);
} // int to string

/// Find the given string in the dictionary.  If the input string is found
/// in the dictionary, it returns the string.  Otherwise it returns null
/// pointer.  This function makes a little easier to determine whether a
/// string is in a dictionary.
inline const char* ibis::dictionary::find(const char* str) const {
    const char* ret = 0;
    const uint32_t ind = operator[](str);
    if (ind < raw_.size())
	ret = raw_[ind];
    return ret;
} // ibis::dictionary::find
#endif // IBIS_DICTIONARY_H
