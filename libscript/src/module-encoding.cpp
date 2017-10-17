/* Copyright (C) 2003-2015 LiveCode Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include <foundation.h>
#include "foundation-auto.h"
#include "foundation-filters.h"

/*
void MCEncodingEvalEncodedOfValue(MCValueRef p_target, MCDataRef& r_output)
{
    
}

void MCEncodingEvalDecodedOfValue(MCDataRef p_target, MCValueRef& r_output)
{
    
}
*/

extern "C" MC_DLLEXPORT_DEF MCDataRef MCEncodingExecCompressUsingZlib(MCDataRef p_target)
{
    MCAutoDataRef t_compressed;
    if (!MCFiltersCompress(p_target, &t_compressed))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not compress data"), nil);
        return nil;
    }
    
    return MCValueRetain(*t_compressed);
}

extern "C" MC_DLLEXPORT_DEF MCDataRef MCEncodingExecDecompressUsingZlib(MCDataRef p_target)
{
    MCAutoDataRef t_decompressed;
    if (!MCFiltersDecompress(p_target, &t_decompressed))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not decompress data"), nil);
        return nil;
    }
    
    return MCValueRetain(*t_decompressed);
}

extern "C" MC_DLLEXPORT_DEF void MCEncodingEvalIsCompressedUsingZlib(MCDataRef p_data, bool& r_result)
{
    r_result = MCFiltersIsCompressed(p_data);
}

extern "C" MC_DLLEXPORT_DEF MCStringRef MCEncodingExecEncodeUsingBase64(MCDataRef p_target)
{
    MCAutoStringRef t_encoded;
    if (!MCFiltersBase64Encode(p_target, &t_encoded))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not encode data"), nil);
        return nil;
    }
    
    return MCValueRetain(*t_encoded);
}

extern "C" MC_DLLEXPORT_DEF MCDataRef MCEncodingExecDecodeUsingBase64(MCStringRef p_target)
{
    MCAutoDataRef t_decoded;
    if (!MCFiltersBase64Decode(p_target, &t_decoded))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not decode string"), nil);
        return nil;
    }
    
    return MCValueRetain(*t_decoded);
}

/*
void MCEncodingExecEncodeUsingBinary(MCStringRef p_target, MCStringRef p_format, MCDataRef& r_output)
{
    //  TODO: Move binary encode/decode to foundation
    //  Does this take a list?
    //    ctxt . Throw();
}

void MCEncodingExecDecodeUsingBinary(MCDataRef p_target, MCStringRef p_format, MCStringRef& r_output)
{
    //  TODO: Move binary encode/decode to foundation
    //  Does this take a list?
    //    ctxt . Throw();
}
*/
////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 // We don't yet do the necessary validation for text encoding 
void MCEncodingExecEncodeUsingUTF8(MCStringRef p_target, MCDataRef& r_output)
{
    if (MCStringEncode(p_target, kMCStringEncodingUTF8, false, r_output))
        return;
    
//    ctxt . Throw();
}

void MCEncodingExecDecodeUsingUTF8(MCDataRef p_target, MCStringRef& r_output)
{
    if (MCStringDecode(p_target, kMCStringEncodingUTF8, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecEncodeUsingUTF16(MCStringRef p_target, MCDataRef& r_output)
{
    if (MCStringEncode(p_target, kMCStringEncodingUTF16, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecDecodeUsingUTF16(MCDataRef p_target, MCStringRef& r_output)
{
    if (MCStringDecode(p_target, kMCStringEncodingUTF16, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecEncodeUsingUTF32(MCStringRef p_target, MCDataRef& r_output)
{
    if (MCStringEncode(p_target, kMCStringEncodingUTF32, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecDecodeUsingUTF32(MCDataRef p_target, MCStringRef& r_output)
{
    if (MCStringDecode(p_target, kMCStringEncodingUTF32, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecEncodeUsingASCII(MCStringRef p_target, MCDataRef& r_output)
{
    if (MCStringEncode(p_target, kMCStringEncodingASCII, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecDecodeUsingASCII(MCDataRef p_target, MCStringRef& r_output)
{
    if (MCStringDecode(p_target, kMCStringEncodingASCII, false, r_output))
        return;
    
    //    ctxt . Throw();
}
*/

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF MCStringRef MCEncodingExecEncodeForLegacyUrl(MCStringRef p_target)
{
    MCAutoStringRef t_encoded;
    if (!MCFiltersUrlEncode(p_target, false, &t_encoded))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not encode url"), nil);
        return nil;
    }
    
    return MCValueRetain(*t_encoded);
}

extern "C" MC_DLLEXPORT_DEF MCStringRef MCEncodingExecDecodeFromLegacyUrl(MCStringRef p_target)
{
    MCAutoStringRef t_decoded;
    if (!MCFiltersUrlDecode(p_target, false, &t_decoded))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not decode url"), nil);
        return nil;
    }
    
    return MCValueRetain(*t_decoded);
}

extern "C" MC_DLLEXPORT_DEF MCStringRef MCEncodingExecEncodeForUrl(MCStringRef p_target)
{
    MCAutoStringRef t_encoded;
    if (!MCFiltersUrlEncode(p_target, true, &t_encoded))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not encode url"), nil);
        return nil;
    }
    
    return MCValueRetain(*t_encoded);
}

extern "C" MC_DLLEXPORT_DEF MCStringRef MCEncodingExecDecodeFromUrl(MCStringRef p_target)
{
    MCAutoStringRef t_decoded;
    if (!MCFiltersUrlDecode(p_target, true, &t_decoded))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not decode url"), nil);
        return nil;
    }
    
    return MCValueRetain(*t_decoded);
}
 
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("ENCODING MODULE", test, result)
void MCEncodingRunTests()
{
/*
 void MCEncodingEvalEncodedOfValue(MCValueRef p_target, MCDataRef& r_output)
 void MCEncodingEvalDecodedOfValue(MCDataRef p_target, MCValueRef& r_output)
 void MCEncodingExecEncodeUsingBase64(MCDataRef p_target, MCStringRef& r_output)
 void MCEncodingExecDecodeUsingBase64(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecEncodeUsingBinary(MCStringRef p_target, MCStringRef p_format, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingBinary(MCDataRef p_target, MCStringRef p_format, MCStringRef& r_output)
 void MCEncodingExecEncodeUsingUTF8(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingUTF8(MCDataRef p_target, MCStringRef& r_output)
 void MCEncodingExecEncodeUsingUTF16(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingUTF16(MCDataRef p_target, MCStringRef& r_output)
 void MCEncodingExecEncodeUsingUTF32(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingUTF32(MCDataRef p_target, MCStringRef& r_output)
 void MCEncodingExecEncodeUsingASCII(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingASCII(MCDataRef p_target, MCStringRef& r_output)
 */
 
/*
void MCEncodingExecCompress(MCDataRef& x_target)
void MCEncodingExecDecompress(MCDataRef& x_target)
*/
    MCAutoDataRef t_data;
    MCDataCreateWithBytes((const byte_t *)"hello world", 11, &t_data);
    
    MCDataRef t_to_compress;
    MCDataMutableCopy(*t_data, t_to_compress);
    MCEncodingExecCompress(t_to_compress);

    log_result("compress changes data", !MCDataIsEqualTo(*t_data, t_to_compress));
    
    MCEncodingExecDecompress(t_to_compress);
    
    log_result("compress/decompress round trip", MCDataIsEqualTo(*t_data, t_to_compress));
    
    MCValueRelease(t_to_compress);
    
    MCDataRef t_empty;
    MCDataMutableCopy(kMCEmptyData, t_empty);
    
    MCEncodingExecCompress(t_empty);
    MCEncodingExecDecompress(t_empty);
    
    log_result("compress/decompress empty", MCDataIsEqualTo(t_empty, kMCEmptyData));
    
    MCValueRelease(t_empty);
 /*
    void MCEncodingEvalURLEncoded(MCStringRef p_target, MCStringRef& r_output)
    void MCEncodingEvalURLDecoded(MCStringRef p_target, MCStringRef& r_output)
*/
    MCStringRef t_test_a, t_test_b;
    MCStringRef t_test_c, t_test_d;
    
    t_test_a = MCSTR(" ");
    t_test_b = MCSTR("+");
    
    t_test_c = MCSTR("?");
    t_test_d = MCSTR("%3F");
    
    MCAutoStringRef t_encoded, t_decoded;
    MCAutoStringRef t_encoded2, t_decoded2;
    
    MCEncodingEvalURLEncoded(t_test_a, &t_encoded);
    
    log_result("url encode space", MCStringIsEqualTo(*t_encoded, t_test_b, kMCStringOptionCompareCaseless));
    
    MCEncodingEvalURLEncoded(t_test_c, &t_encoded2);
    
    log_result("url encode ?", MCStringIsEqualTo(*t_encoded2, t_test_d, kMCStringOptionCompareCaseless));
    
    MCEncodingEvalURLDecoded(t_test_b, &t_decoded);
    
    log_result("url decode +", MCStringIsEqualTo(*t_decoded, t_test_a, kMCStringOptionCompareCaseless));
    
    MCEncodingEvalURLDecoded(t_test_d, &t_decoded2);
    
    log_result("url decode %3F", MCStringIsEqualTo(*t_decoded2, t_test_c, kMCStringOptionCompareCaseless));
    
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_encoding_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_encoding_Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
