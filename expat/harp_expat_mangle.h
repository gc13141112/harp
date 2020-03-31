#ifndef HARP_EXPAT_MANGLE_H
#define HARP_EXPAT_MANGLE_H

/*
 * This header file mangles all symbols exported from the expat library.
 * This is needed on some platforms because of nameresolving conflicts when
 * HARP is used as a module in an application that has its own version of expat.
 * (this problem was seen with the HARP Python interface on Linux).
 * Even though name mangling is not needed for every platform or HARP
 * interface, we always perform the mangling for consitency reasons.
 *
 * The following command was used to obtain the symbol list:
 * nm .libs/libexpat.a | grep " [TR] "
 *
 */

#ifdef HARP_EXPAT_NAME_MANGLE

#define _INTERNAL_trim_to_complete_utf8_characters harp_INTERNAL_trim_to_complete_utf8_characters
#define XML_DefaultCurrent harp_XML_DefaultCurrent
#define XML_ErrorString harp_XML_ErrorString
#define XML_ExpatVersion harp_XML_ExpatVersion
#define XML_ExpatVersionInfo harp_XML_ExpatVersionInfo
#define XML_ExternalEntityParserCreate harp_XML_ExternalEntityParserCreate
#define XML_FreeContentModel harp_XML_FreeContentModel
#define XML_GetBase harp_XML_GetBase
#define XML_GetBuffer harp_XML_GetBuffer
#define XML_GetCurrentByteCount harp_XML_GetCurrentByteCount
#define XML_GetCurrentByteIndex harp_XML_GetCurrentByteIndex
#define XML_GetCurrentColumnNumber harp_XML_GetCurrentColumnNumber
#define XML_GetCurrentLineNumber harp_XML_GetCurrentLineNumber
#define XML_GetErrorCode harp_XML_GetErrorCode
#define XML_GetFeatureList harp_XML_GetFeatureList
#define XML_GetIdAttributeIndex harp_XML_GetIdAttributeIndex
#define XML_GetInputContext harp_XML_GetInputContext
#define XML_GetParsingStatus harp_XML_GetParsingStatus
#define XML_GetSpecifiedAttributeCount harp_XML_GetSpecifiedAttributeCount
#define XML_MemFree harp_XML_MemFree
#define XML_MemMalloc harp_XML_MemMalloc
#define XML_MemRealloc harp_XML_MemRealloc
#define XML_Parse harp_XML_Parse
#define XML_ParseBuffer harp_XML_ParseBuffer
#define XML_ParserCreate harp_XML_ParserCreate
#define XML_ParserCreateNS harp_XML_ParserCreateNS
#define XML_ParserCreate_MM harp_XML_ParserCreate_MM
#define XML_ParserFree harp_XML_ParserFree
#define XML_ParserReset harp_XML_ParserReset
#define XML_ResumeParser harp_XML_ResumeParser
#define XML_SetAttlistDeclHandler harp_XML_SetAttlistDeclHandler
#define XML_SetBase harp_XML_SetBase
#define XML_SetCdataSectionHandler harp_XML_SetCdataSectionHandler
#define XML_SetCharacterDataHandler harp_XML_SetCharacterDataHandler
#define XML_SetCommentHandler harp_XML_SetCommentHandler
#define XML_SetDefaultHandler harp_XML_SetDefaultHandler
#define XML_SetDefaultHandlerExpand harp_XML_SetDefaultHandlerExpand
#define XML_SetDoctypeDeclHandler harp_XML_SetDoctypeDeclHandler
#define XML_SetElementDeclHandler harp_XML_SetElementDeclHandler
#define XML_SetElementHandler harp_XML_SetElementHandler
#define XML_SetEncoding harp_XML_SetEncoding
#define XML_SetEndCdataSectionHandler harp_XML_SetEndCdataSectionHandler
#define XML_SetEndDoctypeDeclHandler harp_XML_SetEndDoctypeDeclHandler
#define XML_SetEndElementHandler harp_XML_SetEndElementHandler
#define XML_SetEndNamespaceDeclHandler harp_XML_SetEndNamespaceDeclHandler
#define XML_SetEntityDeclHandler harp_XML_SetEntityDeclHandler
#define XML_SetExternalEntityRefHandler harp_XML_SetExternalEntityRefHandler
#define XML_SetExternalEntityRefHandlerArg harp_XML_SetExternalEntityRefHandlerArg
#define XML_SetHashSalt harp_XML_SetHashSalt
#define XML_SetNamespaceDeclHandler harp_XML_SetNamespaceDeclHandler
#define XML_SetNotStandaloneHandler harp_XML_SetNotStandaloneHandler
#define XML_SetNotationDeclHandler harp_XML_SetNotationDeclHandler
#define XML_SetParamEntityParsing harp_XML_SetParamEntityParsing
#define XML_SetProcessingInstructionHandler harp_XML_SetProcessingInstructionHandler
#define XML_SetReturnNSTriplet harp_XML_SetReturnNSTriplet
#define XML_SetSkippedEntityHandler harp_XML_SetSkippedEntityHandler
#define XML_SetStartCdataSectionHandler harp_XML_SetStartCdataSectionHandler
#define XML_SetStartDoctypeDeclHandler harp_XML_SetStartDoctypeDeclHandler
#define XML_SetStartElementHandler harp_XML_SetStartElementHandler
#define XML_SetStartNamespaceDeclHandler harp_XML_SetStartNamespaceDeclHandler
#define XML_SetUnknownEncodingHandler harp_XML_SetUnknownEncodingHandler
#define XML_SetUnparsedEntityDeclHandler harp_XML_SetUnparsedEntityDeclHandler
#define XML_SetUserData harp_XML_SetUserData
#define XML_SetXmlDeclHandler harp_XML_SetXmlDeclHandler
#define XML_StopParser harp_XML_StopParser
#define XML_UseForeignDTD harp_XML_UseForeignDTD
#define XML_UseParserAsHandlerArg harp_XML_UseParserAsHandlerArg
#define XmlPrologStateInit harp_XmlPrologStateInit
#define XmlPrologStateInitExternalEntity harp_XmlPrologStateInitExternalEntity
#define XmlGetUtf16InternalEncoding harp_XmlGetUtf16InternalEncoding
#define XmlGetUtf16InternalEncodingNS harp_XmlGetUtf16InternalEncodingNS
#define XmlGetUtf8InternalEncoding harp_XmlGetUtf8InternalEncoding
#define XmlGetUtf8InternalEncodingNS harp_XmlGetUtf8InternalEncodingNS
#define XmlInitEncoding harp_XmlInitEncoding
#define XmlInitEncodingNS harp_XmlInitEncodingNS
#define XmlInitUnknownEncoding harp_XmlInitUnknownEncoding
#define XmlInitUnknownEncodingNS harp_XmlInitUnknownEncodingNS
#define XmlParseXmlDecl harp_XmlParseXmlDecl
#define XmlParseXmlDeclNS harp_XmlParseXmlDeclNS
#define XmlSizeOfUnknownEncoding harp_XmlSizeOfUnknownEncoding
#define XmlUtf16Encode harp_XmlUtf16Encode
#define XmlUtf8Encode harp_XmlUtf8Encode

#endif

#endif
