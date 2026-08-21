#ifndef PROBABILITYGUESSER_CURL_M_H
#define PROBABILITYGUESSER_CURL_M_H

#include <curl/curl.h>
#include <stddef.h>

namespace curl_mock {
void reset();
void set_response(const char *response);
void fail_next_setopt(CURLoption option, CURLcode error = CURLE_BAD_FUNCTION_ARGUMENT);
void set_perform_result(CURLcode result);
size_t perform_count();
const char *url();
const char *post_fields();
bool post_enabled();
}

#endif
