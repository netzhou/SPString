/*
*  C Implementation: SPString
*
* Description:
*    This class provides a safe and convenient, albeit slower, alternative
*    to C character chains.
*
*    One can easily create a String from a C string with newString(char *).
*    String.str always returns a null-terminated C chain, and String.len
*    returns its length.
*    One should always avoid to manipulate the String structure members
*    directly in order to prevent corruption (i.e incorrect sz and len values).
*
*    Most functions mirror standard C functions.
*    stringcpy and stringcat can increase their buffer size if necessary in
*    order to prevent buffer overflow.
*
* Author: Nicolas Janin <>, (C) 2009
*
* Copyright: LGPL
*
*/
 
#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "SPString.h"
 
#undef  TIDY               /* Tidy up buffers by filling them with zeroes       */
                           /* Slows things down a little, so uncomment this     */
                           /* only if needed.                                   */
 
/* private (internal mechanics) */
static int ST_ERR = ST_NO_ERR;
static int incsz ( String *string, size_t len );
 
/* --- Dynamic allocation
 
 String can dynamically increase in size as necessary. */
String * newString ( const char *chars )
{
        String *newS = NULL;
 
        if ( chars != NULL )
        {
                size_t len = strlen ( chars );
                newS = ( String * ) malloc ( sizeof ( String ) );
                if ( newS != NULL )
                {
                        newS->len = len;
                        newS->sz = len + 1;
                        newS->str = strdup ( chars );
                        if ( newS->str == NULL )
                        {
                                ST_ERR = ST_ALLOC_ERR;
                        }
                }
                else
                {
                        ST_ERR = ST_ALLOC_ERR;
                }
        }
        else
        {
                ST_ERR = ST_NULLPARAM_ERR;
        }
        return newS;
}
 
void delString ( String *string )
{
#ifdef NDEBUG
        stringcheck ( string, __FILE__, __LINE__ );
#endif
        if ( string != NULL )
        {
                if ( string->str != NULL )
                {
                        free ( string->str );
                        string->str = NULL;
                }
                else
                {
                        ST_ERR = ST_NULLSTR_ERR;
                }
                free ( string );
                string = NULL;
        }
        else
        {
                ST_ERR = ST_NULLPARAM_ERR;
        }
}
 
/* Copy - Will increase dst allocation if src is too large */
size_t stringcpy ( String *dst, const String *src )
{
#ifdef NDEBUG
        stringcheck ( src, __FILE__, __LINE__ );
        stringcheck ( dst, __FILE__, __LINE__ );
#endif
        size_t len = src->len;
        if ( dst->sz <= len )
        {
                if ( incsz ( dst, len + 1 ) == ST_ALLOC_ERR )
                {
                        return ST_ALLOC_ERR;
                }
        }
 
        dst->len = len;
#ifdef TIDY
        strncpy ( dst->str, src->str, len );
#else
        strcpy ( dst->str, src->str );
#endif
        return len;
}
 
/* Copy - Will increase dst allocation if src is too large */
size_t stringchcpy ( String *dst, const char *src )
{
#ifdef NDEBUG
        stringcheck ( dst, __FILE__, __LINE__ );
#endif
        if ( src == NULL )
        {
                ST_ERR = ST_NULLPARAM_ERR;
                return ST_NULLPARAM_ERR;
        }
        size_t len = strlen ( src );
        if ( dst->sz <= len )
        {
                if ( incsz ( dst, len + 1 ) == ST_ALLOC_ERR )
                {
                        return ST_ALLOC_ERR;
                }
        }
 
        dst->len = len;
#ifdef TIDY
        strncpy ( dst->str, src, len );
#else
        strcpy ( dst->str, src );
#endif
        return len;
}
 
/* Concatenation - Will increase dst allocation when necessary */
size_t stringcat ( String *dst, const String *src )
{
#ifdef NDEBUG
        stringcheck ( src, __FILE__, __LINE__ );
        stringcheck ( dst, __FILE__, __LINE__ );
#endif
        size_t len = src->len;
        size_t finallen = dst->len + len;
        if ( dst->sz <= finallen )
        {
                if ( incsz ( dst, finallen + 1 ) == ST_ALLOC_ERR )
                {
                        return ST_ALLOC_ERR;
                }
        }
 
        dst->len = finallen;
        strcat ( dst->str, src->str );
#ifdef TIDY
        size_t l = dst->len;
        char * end =  dst->str + l;
        for ( ; l < dst->sz; l++ ) *end++ = 0;
#endif
        return len;
}
 
/* Concatenation - Will increase dst allocation when necessary */
size_t stringchcat ( String *dst, const char *src )
{
#ifdef NDEBUG
        stringcheck ( dst, __FILE__, __LINE__ );
#endif
        if ( src == NULL )
        {
                ST_ERR = ST_NULLPARAM_ERR;
                return ST_NULLPARAM_ERR;
        }
        size_t len = strlen ( src );
        size_t finallen = dst->len + len;
        if ( dst->sz <= finallen )
        {
                if ( incsz ( dst, finallen + 1 ) == ST_ALLOC_ERR )
                {
                        return ST_ALLOC_ERR;
                }
        }
 
        dst->len = finallen;
        strcat ( dst->str, src );
#ifdef TIDY
        size_t l = dst->len;
        char * end =  dst->str + l;
        for ( ; l < dst->sz; l++ ) *end++ = 0;
#endif
 
        return len;
}
 
/* Duplication - allocate a new String */
String * stringdup ( const String *src )
{
#ifdef NDEBUG
        stringcheck ( src, __FILE__, __LINE__ );
#endif
        String *string = NULL;
        if ( src != NULL )
        {
                string = newString ( src->str );
        }
        else
        {
                ST_ERR = ST_NULLPARAM_ERR;
        }
        return string;
}
 
 
/* ---- Allocation on the stack (non dynamic) */
 
/*
    Allocation on the stack
    Here, szMax is a maximum size of the buffer, which is allocated
    once and for all, and cannot be increased at runtime.
    Furthermore, the chars array must not be deallocated before the release
    of the LString (else mangling pointer).
    If you need fully extensible Strings, use dynamic allocation instead.
*/
LString localString ( char *chars, size_t szMax )
{
        LString newS;
        if ( chars != NULL )
        {
                newS = ( LString ) {szMax, strlen ( chars ), chars};
#ifdef TIDY
                size_t l = newS.len;
                char * end =  chars + l;
                for ( ; l < szMax; l++ ) *end++ = 0;
#endif
        }
        else
        {
                ST_ERR = ST_NULLPARAM_ERR;
                newS = ( LString ) {0, 0, NULL};
        }
        return newS;
}
 
/* Copy of LString - checks that there is no buffer overflow */
size_t lstringcpy ( LString *dst, const LString *src )
{
#ifdef NDEBUG
        stringcheck ( ( String * ) src, __FILE__, __LINE__ );
        stringcheck ( ( String * ) dst, __FILE__, __LINE__ );
#endif
        size_t len = src->len;
        if ( dst->sz <= len )
        {
                ST_ERR = ST_OVERWRITE_ERR;
                return ST_OVERWRITE_ERR;
        }
        dst->len = len;
#ifdef TIDY
        strncpy ( dst->str, src->str, dst->sz );
#else
        strcpy ( dst->str, src->str );
#endif
        return len;
}
 
/* Copy of a fixed character chain in a LString
 * checks that there is no buffer overflow
 */
size_t lstringchcpy ( LString *dst, const char *src )
{
#ifdef NDEBUG
        stringcheck ( ( String * ) dst, __FILE__, __LINE__ );
#endif
        if ( src == NULL )
        {
                ST_ERR = ST_NULLPARAM_ERR;
                return ST_NULLPARAM_ERR;
        }
        size_t len = strlen ( src );
        if ( dst->sz <= len )
        {
                ST_ERR = ST_OVERWRITE_ERR;
                return ST_OVERWRITE_ERR;
        }
        dst->len = len;
#ifdef TIDY
        strncpy ( dst->str, src, dst->sz );
#else
        strcpy ( dst->str, src );
#endif
        return len;
}
 
size_t lstringcat ( LString *dst, const LString *src )
{
#ifdef NDEBUG
        stringcheck ( ( String * ) src, __FILE__, __LINE__ );
        stringcheck ( ( String * ) dst, __FILE__, __LINE__ );
#endif
        size_t len = src->len;
        size_t finallen = dst->len + len;
        if ( finallen >= dst->sz )
        {
                ST_ERR = ST_OVERWRITE_ERR;
                return ST_OVERWRITE_ERR;
        }
        dst->len = finallen;
        strcat ( dst->str, src->str );
#ifdef TIDY
        size_t l = dst->len;
        char * end =  dst->str + l;
        for ( ; l < dst->sz; l++ ) *end++ = 0;
#endif
        return len;
}
 
/* Cooncatenation of a fixed character chain to a LString */
size_t lstringchcat ( LString *dst, const char *src )
{
#ifdef NDEBUG
        stringcheck ( ( String * ) dst, __FILE__, __LINE__ );
#endif
        if ( src == NULL )
        {
                ST_ERR = ST_NULLPARAM_ERR;
                return ST_NULLPARAM_ERR;
        }
        size_t len = strlen ( src );
        size_t finallen = dst->len + len;
        if ( finallen >= dst->sz )
        {
                ST_ERR = ST_OVERWRITE_ERR;
                return ST_OVERWRITE_ERR;
        }
        dst->len = finallen;
        strcat ( dst->str, src );
#ifdef TIDY
        size_t l = dst->len;
        char * end =  dst->str + l;
        for ( ; l < dst->sz; l++ ) *end++ = 0;
#endif
        return len;
}
 
LString lstringdup ( const LString *src )
{
#ifdef NDEBUG
        stringcheck ( ( String * ) src, __FILE__, __LINE__ );
#endif
        LString string;
        if ( src != NULL )
        {
                string = localString ( src->str, src->len );
        }
        else
        {
                ST_ERR = ST_NULLPARAM_ERR;
                string = ( LString ) {0, 0, NULL};
        }
        return string;
}
 
/* The following functions apply both on String and LString */
 
size_t stringlen ( const String *string )
{
        return string->len;
}
 
/*
    Check for an internal inconsistency in a LString.
    You can use this in a debugging session.
*/
int stringcheck ( const String *string, char *file, int line )
{
#ifdef NDEBUG
        #define ASSERT(cond) if ( !(cond) ) { \
                        printf("stringcheck error %s line %d:String(%d/%d)=\"%s\"\n", \
                        file, line, string->len, string->sz, string->str); \
                        assert( string != NULL ); }
        ASSERT ( string != NULL );
        ASSERT ( string->str != NULL );
        ASSERT ( string->len < string->sz );
        ASSERT ( string->len == strlen ( string->str ) );
        return 0;
        #undef ASSERT
#else
        if ( string == NULL )
                return ST_NULLPARAM_ERR;
        if ( string->str == NULL )
                return ST_NULLSTR_ERR;
        if ( string->len >= string->sz )
                return ST_INCONSISTENTSZ_ERR;
        if ( string->len != strlen ( string->str ) )
                return ST_INCONSISTENTLEN_ERR;
        return 0;
#endif
}
 
/* String comparison */
int stringcmp ( const String *st1, const String *st2 )
{
#ifdef NDEBUG
        stringcheck ( ( String * ) st1, __FILE__, __LINE__ );
        stringcheck ( ( String * ) st2, __FILE__, __LINE__ );
#endif
        if ( st1->len != st2->len )
        {
                return ( int ) ( st2->len - st1->len );
        }
 
        return strcmp ( st1->str, st2->str );
}
 
/* Comparison with a C chain */
int stringchcmp ( const String *st1, const char *st2 )
{
#ifdef NDEBUG
        stringcheck ( ( String * ) st1, __FILE__, __LINE__ );
#endif
        size_t lt2 = strlen ( st2 );
        if ( st1->len != lt2 )
        {
                return ( int ) ( lt2 - st1->len );
        }
 
        return strcmp ( st1->str, st2 );
}
 
/* Truncation to the Nth character */
String * stringtrunc ( String *string, size_t N )
{
#ifdef NDEBUG
        stringcheck ( string, __FILE__, __LINE__ );
#endif
        string->len = N;
        string->str[N-1] = '\0';
#ifdef TIDY
        size_t l = string->len;
        char * end =  string->str + l;
        for ( ; l < string->sz; l++ ) *end++ = 0;
#endif
        return string;
}
 
 
/* Formatting */
void stringprintf ( String *dst, const char * format, ... )
{
#ifdef NDEBUG
        stringcheck ( dst, __FILE__, __LINE__ );
#endif
        va_list argp;
        va_start ( argp, format );
        dst->len = vsnprintf ( dst->str, dst->sz, format, argp ) ;
        va_end ( argp ) ;
}
 
/* Returns error type and resets ST_ERR */
int get_stringerr()
{
        int err = ST_ERR;
        ST_ERR = ST_NO_ERR;
        return err;
}
 
/*************************************************
* PRIVATE
**************************************************/
 
/* Increase string.sz to desired size */
static int incsz ( String *string, size_t size )
{
        ST_ERR = ST_NO_ERR;
 
        if ( string->sz < size )
        {
            /* double as necessary */
                size_t newsz = string->sz;
        do
                {
                newsz *= 2;
                } while (newsz < size);
               
                char *tmp = realloc ( string->str, (size_t)newsz );
 
                if ( tmp != NULL )
                {
#ifdef TIDY
                        size_t l = string->len;
                        char * end =  tmp + l;
                        for ( ; l < size; l++ ) *end++ = 0;
#endif
                        string->str = tmp;
                        string->sz = newsz;
                }
                else
                {
                        ST_ERR = ST_ALLOC_ERR;
                }
        }
        return ST_ERR;
}
 
#undef TIDY
