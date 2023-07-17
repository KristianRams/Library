#ifndef String_H
#define String_H

#include "Types.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define MAX_STRING_INDEX 0x7fffffff
// Carrying around this state of allocated = true is very annoying
// and easy to get wrong -> leading to memory leakage.
struct string {
    string() = default;
    ~string();

    string(s32 _count, char fill);
    string(const char *rhs);
    string(const string &rhs);
    string &operator=(string rhs);

    string &operator+=(string &rhs);
    string &operator+=(const char *rhs);

    bool operator==(string &rhs);
    bool operator==(const char *rhs);

    u8 &operator [](s32 index) {
        assert(index >= 0);
        assert(index < MAX_STRING_INDEX);
        assert((s32)index < count);
        return data[index];
    }

    const u8 &operator [](s32 index) const {
        assert(index >= 0);
        assert (index < MAX_STRING_INDEX);
        assert((s32)index < count);
        return data[index];
    }

    u32     size();
    u32     length();
    char   *raw();
    bool    empty();
    void    reset();
    string &to_lower();
    string &to_upper();
    s32     compare(string *rhs);
    bool    compare_and_ignore_case();
    void    reserve(s32 bytes);
    string  substring(s32 low, s32 high);

    char *begin() {
        if (data) { return (char *)&data[0]; }
        assert(false);
    }

    char *begin() const {
        if (data) { return (char *)&data[0]; }
        assert(false);
    }

    char *end() {
        if (data) { return (char *)&data[count]; } // count+1 holds nul termination
        assert(false);
    }
    char *end() const {
        if (data) { return (char *)&data[count]; } // count+1 holds nul termination
        assert(false);
    }

    u8  *data        = NULL;
    s32  count       = 0;
    bool allocated   = false;
};

char char_to_lower(char character) {
    if (character >= 'A' && character <='Z') {
        return character + ' ';
    }
    return character;
}

char char_to_upper(char character) {
    if (character >= 'a' && character <= 'z') {
        return character - ' ';
    }
    return character;
}

s32 strcmp(const char *p1, const char *p2) {
    if (p1 == NULL && p2 != NULL) { return -1; }
    if (p1 == NULL && p2 == NULL) { return  0; }
    if (p1 != NULL && p2 == NULL) { return  1; }

    u8 *s1 = (u8 *)p1;
    u8 *s2 = (u8 *)p2;
    u8 c1, c2;

    do {
        c1 = *s1++;
        c2 = *s2++;
        if (c1 == '\0') { return c1 - c2; }
    } while (c1 == c2);

    return c1 - c2;
}

string make_literal(const char *s) {
    string str;
    if (s == NULL) { return str; }
    str.count = strlen(s);
    str.data  = (u8 *)s;
    return str;
}

string make_literal(const char *s, s32 count) {
    string str;
    if (count == 0) {
        count = strlen(s);
    }
    str.count = count;
    str.data  = (u8 *)s;
    return str;
}

string &string::operator+=(const char *rhs) {
    s32 rhs_count = strlen(rhs);
    if (rhs_count == 0 && count == 0) { return *this; }
    s32 total_count = rhs_count + count;
    u8 *buffer      = new u8[total_count+1]; // for nul terminator.
    memmove(buffer, data, count);
    memmove(buffer+count, rhs, rhs_count);
    buffer[total_count] = '\0';
    if (allocated) {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
    data  = buffer;
    count = total_count;
    allocated = true;
    return *this;
}

string &string::operator+=(string &rhs) {
    if (rhs.count == 0 && count == 0) { return *this; }
    s32 total_count = count + rhs.count;
    u8 *buffer      = new u8[total_count+1]; // for nul terminator.
    memmove(buffer, data, count);
    memmove(buffer+count, rhs.data, rhs.count);
    buffer[total_count] = '\0';
    if (allocated) {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
    data  = buffer;
    count = total_count;
    allocated = true;
    return *this;
}

bool string::operator==(string &rhs) {
    if (strcmp((const char *)data, (const char *)rhs.data)) { return false; }
    return true;
}

bool string::operator==(const char *rhs) {
    if (strcmp((const char *)data, rhs)) { return false; }
    return true;
}

string::string(s32 _count, char fill) {
    data  = new u8[_count+1]; // nul terminator
    count = _count;
    memset(data, fill, count);
    allocated = true;
}

string::string(const string &rhs) {
    count = rhs.count;
    data  = new u8[count+1]; // nul terminator
    memmove(data, rhs.data, rhs.count);
    allocated = true;
}

// This only gets called once per instance so we don't need to delete
// the data before we allocate it. We just need to set the allocated flag.
string::string(const char *rhs) {
    auto rhs_count = strlen(rhs);
    data = new u8[rhs_count+1]; // nul terminator
    memmove(data, rhs, rhs_count);
    data[rhs_count] = '\0';
    count           = rhs_count;
    allocated       = true;
}

string &string::operator=(string rhs) {
    if (rhs.count == 0) {
        count = strlen((const char *)rhs.data);
    }
    count = rhs.count;
    delete[] data;
    data = new u8[count+1]; // nul terminator
    memmove((void *)data, (void *)rhs.data, rhs.count);
    data[count] = '\0';
    allocated   = true;
    return *this;
}

string::~string() {
    if (allocated) {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
    count = 0;
}

char *string::raw()    { return (char *)data; }
u32   string::size()   { return count; }
u32   string::length() { return count; }
bool  string::empty()  { return count == 0; }

void string::reset() {
    if (allocated) {
        if (data) {
            delete[] data;
        }
    }
    data      = NULL;
    count     = 0;
    allocated = false;
}

void string::reserve(s32 bytes) {
    if (bytes < 0) { return; }
    if (allocated) {
        if (data) {
            delete[] data;
            data = NULL;
        }
    }
    data      = new u8[bytes];
    allocated = true;
}

string &string::to_lower() {
    if (count == 0) { return *this; }
    for (int i = 0; i < count; ++i) {
        data[i] = char_to_lower((char)data[i]);
    }
    return *this;
}

string &string::to_upper() {
    if (count == 0) { return *this; }
    for (int i = 0; i < count; ++i) {
        data[i] = char_to_upper((char)data[i]);
    }
    return *this;
}

string string::substring(s32 low, s32 high) {
    return make_literal((const char *)data+low, high-low);
}

string substring(string *str, s32 low, s32 high) {
        return make_literal((const char *)(str->data+low), high-low);
}

u64 string_length(string *str) {
    if (str == NULL) { return 0; }
    return strlen((const char *)str->data);
}

u64 string_size(string *str) {
    return string_length(str);
}

s32 string_compare(string *str1, string *str2) {
   if (strcmp((const char *)str1->data, (const char *)str2->data)) { return false; }
   return true;
}

s32 string_compare(string *str1, const char *str2) {
    if (strcmp((const char *)str1->data, str2)) { return false; }
    return true;
}

s32 string_compare_and_ignore_case(string *str1, string *str2) {
    if (str1->count != str2->count) { return false; }
    u8 *str1_data_pointer = str1->data;
    u8 *str2_data_pointer = str2->data;
    s32 counter = 0;
    while ((counter < str1->count) && (*str1_data_pointer != '\0') && (*str2_data_pointer != '\0')) {
        char c1 = char_to_lower((char)*str1_data_pointer);
        char c2 = char_to_lower((char)*str2_data_pointer);
        if (c1 != c2) {
            return false;
        }
        str1_data_pointer++;
        str2_data_pointer++;
        counter++;
    }
    return true;
}

s32 string_compare_and_ignore_case(string *str1, const char *str2) {
    if (str1->count != strlen(str2)) { return false; }
    char *str1_data_pointer = (char *)str1->data;
    char *str2_data_pointer = (char *)str2;
    s32 counter = 0;
    while ((counter < str1->count) && (*str1_data_pointer != '\0') && (*str2_data_pointer != '\0')) {
        char c1 = char_to_lower((char)*str1_data_pointer);
        char c2 = char_to_lower((char)*str2_data_pointer);
        if (c1 != c2) {
            return false;
        }
        str1_data_pointer++;
        str2_data_pointer++;
        counter++;
    }
    return 0;
}

#endif
