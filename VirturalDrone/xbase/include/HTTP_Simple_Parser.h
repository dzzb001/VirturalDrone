#ifndef HTTP_SIMPLE_PARSER_H
#define HTTP_SIMPLE_PARSER_H


#include <string>
#include <map>


/**
 *	@brief	Parses an HTTP request header.
 *
 *	@param[in]	buf
 *					The input buffer containing the HTTP request header.
 *	@param[in]	len
 *					The length of the input buffer.
 *	@param[out]	method
 *					Returns the "method" field in the HTTP request header.
 *	@param[out]	uri
 *					Returns the "uri" field in the HTTP request header.
 *	@param[out]	version
 *					Returns the "version" field in the HTTP request header
 *	@param[out]	pragma
 *					Returns the "pragma" pairs in the HTTP request header.
 *	@param[out]	content
 *					Returns the the HTTP request content.
 *
 *	@return
 *		-		0	to indicate packet not fully received.
 *		-		-1	to indicate error.
 *		-		>0	the number of bytes which has been successfully parsed
 */
int  parse_http_request_header(
		const char *buf,
		size_t len,
		std::string &method,
		std::string &uri,
		std::string &version,
		std::map<std::string, std::string> &pragma,
		std::string &content
	);


/**
 *	@brief	Parses an HTTP response header.
 *
 *	@param[in]	buf
 *					The input buffer containing the HTTP response header.
 *	@param[in]	len
 *					The length of the input buffer.
 *	@param[out]	version
 *					Returns the "version" field in the HTTP response
 *					header.
 *	@param[out]	status_code
 *					Returns the "status-code" field in the HTTP response
 *					header.
 *	@param[out]	content
 *					Returns the the HTTP request content.
 *
 *	@return
 *		-		0	to indicate packet not fully received.
 *		-		-1	to indicate error.
 *		-		>0	the number of bytes which has been successfully parsed
 */
int  parse_http_response_header(
		const char *buf,
		size_t len,
		std::string &version,
		std::string &status_code,
		std::string &content
	);


#endif // HTTP_SIMPLE_PARSER_H
