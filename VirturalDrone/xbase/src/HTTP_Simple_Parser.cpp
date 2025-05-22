#include "HTTP_Simple_Parser.h"

#include <string>
#include <assert.h>
#include <stdlib.h>


/**
 *	@brief	Gets one HTTP line from a buffer.
 *
 *	@param[in,out]
 *				buf
 *					The buffer containing the string to be parsed. On
 *					successful return, it will point to the first 
 *					character of the next line. Otherwise it will remain
 *					unchanged.
 *	@param[in,out]
 *				len
 *					The length of the input buffer. On successful return,
 *					it will be set to the remaining buffer length. 
 *					Otherwise it will remain unchanged.
 *	@param[out]
 *				line
 *					The line which has been extracted from the buffer.
 *
 *	@return		True for success, false for failure.
 */
static bool __get_http_line__(
							  const char *&buf,
							  size_t &len,
							  std::string &line
							  );


/**
 *	@brief	Trims the heading and trailing whitespace/s in string @a s.
 *
 *	@param[in,out]
 *				s
 *					The string to be trimmed. On return, the trimmed
 *					result.
 */
static void __trim__(std::string &s);


/// Collection of white spaces.
static const char *SP = " ";


//========================================================================


/**
 *	@brief	Parses one HTTP line containing the "Pragma" directive.
 *
 *	@param[in]	line
 *					The input HTTP line.
 *	@param[out]	pragma
 *					The map to hold the result.
 */
static void parse_http_pragma(
		const std::string &line,
		std::map<std::string, std::string> &pragma
	);


/**
 *	@brief	Parses one HTTP line containing the "Content-Length" directive
 *
 *	@param[in]	line
 *					The input HTTP line.
 *	@param[out]	content_length
 *					The integer to hold the result.
 */
static void parse_http_content_length(
									  const std::string &line,
									  int &content_length
									  );


//========================================================================


int  parse_http_request_header(
		const char *buf,
		size_t len,
		std::string &method,
		std::string &uri,
		std::string &version,
		std::map<std::string, std::string> &pragma,
		std::string &content
	)
{
	// record the original value of len
	const size_t original_len = len;

	// Step 1:
	//   parse the request-line --- skip all empty heading lines
	std::string request_line;
	do
	{
		if (!__get_http_line__(buf, len, request_line))
			return 0;
		__trim__(request_line);
	}
	while (request_line.empty());

	// request_line = method SP uri SP version

	// method = request_line [ 0 : r_method )
	std::string::size_type
		r_method = request_line.find_first_of(SP);
	if (r_method == std::string::npos)
		return -1;
	method = request_line.substr(0, r_method - 0);

	// uri = request_line [ l_uri : r_uri )
	std::string::size_type
		l_uri = request_line.find_first_not_of(SP, r_method + 1);
	if (l_uri == std::string::npos)
		return -1;
	std::string::size_type
		r_uri = request_line.find_first_of(SP, l_uri + 1);
	if (r_uri == std::string::npos)
		return -1;
	uri = request_line.substr(l_uri, r_uri - l_uri);

	// version = request_line [ l_version : r_version )
	std::string::size_type
		l_version = request_line.find_first_not_of(SP, r_uri + 1);
	if (l_version == std::string::npos)
		return -1;
	std::string::size_type
		r_version = request_line.find_first_of(SP, l_version + 1);
	if (r_version == std::string::npos)
		version = request_line.substr(l_version);
	else
		version = request_line.substr(l_version, r_version - l_version);

	// the Content-Length property
	int content_length = 0;

	// Step 2:
	//   parse the request-headers
	while (true)
	{
		// get request-header line by line
		std::string line;
		if (!__get_http_line__(buf, len, line))
			return 0;
		__trim__(line);
		// an empty request-header line terminates the HTTP request
		if (line.empty())
			break;
		// parses the line
		if (line.substr(0, 6) == "Pragma")
			parse_http_pragma(line, pragma);
		else if (line.substr(0, 14) == "Content-Length")
			parse_http_content_length(line, content_length);
	}

	// Step 3:
	//   parse the content
	if (len < (size_t)content_length)
		return 0;

	// assign content.
	content.assign(buf, content_length);

	// return success.
	return (int)(original_len + content_length - len);
}


int  parse_http_response_header(
		const char *buf,
		size_t len,
		std::string &version,
		std::string &status_code,
		std::string &content
	)
{
	// record the original value of len
	const size_t original_len = len;

	// Step 1:
	//   parse the status-line --- skip all empty heading lines
	std::string status_line;
	do
	{
		if (!__get_http_line__(buf, len, status_line))
			return 0;
		__trim__(status_line);
	}
	while (status_line.empty());

	// status_line = version SP status_code SP reason_phrase

	// version = status_line [ 0 : r_version ]
	std::string::size_type
		r_version = status_line.find_first_of(SP);
	if (r_version == std::string::npos)
		return -1;
	version = status_line.substr(0, r_version - 0);

	// status_code = status_line [ l_status_code : r_status_code ]
	std::string::size_type
		l_status_code = status_line.find_first_not_of(SP, r_version + 1);
	if (l_status_code == std::string::npos)
		return -1;
	std::string::size_type
		r_status_code = status_line.find_first_of(SP, l_status_code + 1);
	if (r_status_code == std::string::npos)
		return -1;
	status_code =
		status_line.substr(l_status_code, r_status_code - l_status_code);

	// the Content-Length property
	int content_length = 0;

	// Step 2:
	//   parse the response-headers
	while (true)
	{
		// get the response-header line by line
		std::string line;
		if (!__get_http_line__(buf, len, line))
			return 0;
		__trim__(line);
		// an empty response-header line terminates the HTTP response
		if (line.empty())
			break;
		// parse the line
		if (line.substr(0, 14) == "Content-Length")
			parse_http_content_length(line, content_length);
	}

	// Step 3:
	//   parse the content
	if (len < (size_t)content_length)
		return 0;

	// assign content.
	content.assign(buf, content_length);

	// return.
	return (int)(original_len + content_length - len);
}


//========================================================================


static void parse_http_pragma(
							  const std::string &line,
							  std::map<std::string, std::string> &pragma
							  )
{
	// find the ":"
	std::string::size_type
		l_colon = line.find_first_of(":", 6);
	if (l_colon == std::string::npos)
		return;

	// extract pragma directives one by one
	std::string::size_type
		l_directive = l_colon;
	do
	{
		std::string::size_type r_directive =
			line.find_first_of(",", l_directive + 1);
		std::string directive;
		// get the directive
		if (r_directive == std::string::npos)
			directive = line.substr(l_directive + 1);
		else
			directive =
				line.substr(
					l_directive + 1,
					r_directive - (l_directive + 1)
				);
		// advance l_directive
		l_directive = r_directive;
		// split the directive by "=" into key and value
		std::string::size_type
			l_equal = directive.find_first_of("=");
		std::string directive_key;
		std::string directive_val;
		if (l_equal == std::string::npos)
		{
			directive_key = directive;
			__trim__(directive_key);
			directive_val = "";
		}
		else
		{
			directive_key = directive.substr(0, l_equal);
			__trim__(directive_key);
			directive_val = directive.substr(l_equal+1);
			__trim__(directive_val);
		}
		// insert the key-value pair into the pragma map
		pragma.insert(std::make_pair(directive_key, directive_val));
	}
	while (l_directive != std::string::npos);
}


static void parse_http_content_length(
									  const std::string &line,
									  int &content_length
									  )
{
	// find the ":"
	std::string::size_type
		l_colon = line.find_first_of(":", 14);
	if (l_colon == std::string::npos)
		return;

	std::string number = line.substr(l_colon + 1);
	__trim__(number);

	char *end_ptr;
	long n = strtol(number.c_str(), &end_ptr, 10);
	if ((*end_ptr == '\0') && (n >= 0))
	{
		content_length = n;
	}
}


//========================================================================


static bool __get_http_line__(
							  const char *&buf,
							  size_t &len,
							  std::string &line
							  )
{
	int pos_crlf = -1;
	for (int i = 0; i < (int)(len - 1); ++i)
	{
		if ((buf[i] == '\r') && (buf[i+1] == '\n'))
		{
			pos_crlf = i;
			break;
		}
	}

	if (pos_crlf == -1)
		return false;

	line.assign(buf, pos_crlf);

	buf += (pos_crlf + 2);
	len -= (pos_crlf + 2);
	return true;
}


static void __trim__(std::string &s)
{
	std::string::size_type l = s.find_first_not_of(SP);

	if (l == std::string::npos)
	{
		s.clear();
	}
	else
	{
		std::string::size_type r = s.find_last_not_of(SP);
		assert(r != std::string::npos);
		s = s.substr(l, r + 1 - l);
	}
}
