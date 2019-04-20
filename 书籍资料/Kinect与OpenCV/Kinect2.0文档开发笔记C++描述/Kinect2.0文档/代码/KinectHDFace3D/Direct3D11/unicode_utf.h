// ShaderObject类 着色器对象

#pragma once
// Unicode16 to UTF-8
UINT32 Unicode16toUTF8(const char16_t* pUnicode16String, char* pUTF8String, UINT32 uBufferLength);
// Unicode32 to UTF-8
UINT32 Unicode32toUTF8(const char32_t* pUnicode32String, char* pUTF8String, UINT32 uBufferLength);
// UTF-8 to Unicode16
UINT32 UTF8toUnicode16(const char* pUTF8String, char16_t* pUnicode16String, UINT32 uBufferLength);
// UTF-8 to Unicode32
UINT32 UTF8toUnicode32(const char* pUTF8String, char32_t* pUnicode32String, UINT32 uBufferLength);

static_assert(sizeof(wchar_t) == sizeof(char16_t), "change Unicode16 to Unicode32");
#if 1
#define UTF8toUnicode(a, b, c) UTF8toUnicode16((a), reinterpret_cast<char16_t*>(b), (c))
#define UnicodetoUTF8(a, b, c) Unicode16toUTF8(reinterpret_cast<char16_t*>(a), (b), (c))
#else
#define UTF8toUnicode(a, b, c) UTF8toUnicode16((a), reinterpret_cast<char32_t*>(b), (c))
#define UnicodetoUTF8(a, b, c) Unicode32toUTF8(reinterpret_cast<char32_t*>(a), (b), (c))
#endif