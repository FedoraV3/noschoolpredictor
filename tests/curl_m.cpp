#include "curl_m.h"

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {
using write_callback_t = size_t (*)(char *, size_t, size_t, void *);

struct mock_state {
    std::string response;
    std::string configured_url;
    std::string configured_post_fields;
    write_callback_t write_callback = nullptr;
    void *write_data = nullptr;

    CURLoption failing_option = CURLOPT_LASTENTRY;
    CURLcode setopt_error = CURLE_BAD_FUNCTION_ARGUMENT;
    CURLcode perform_result = CURLE_OK;
    size_t performs = 0;
    bool post = false;
};

mock_state state;
int fake_handle;
}

namespace curl_mock {
void reset() { state = mock_state{}; }
void set_response(const char *response) { state.response = response == nullptr ? "" : response; }
void fail_next_setopt(CURLoption option, CURLcode error)
{
    state.failing_option = option;
    state.setopt_error = error;
}
void set_perform_result(CURLcode result) { state.perform_result = result; }
size_t perform_count() { return state.performs; }
const char *url() { return state.configured_url.c_str(); }
const char *post_fields() { return state.configured_post_fields.c_str(); }
bool post_enabled() { return state.post; }
}

extern "C" {
CURLcode curl_global_init(long) { return CURLE_OK; }
CURL *curl_easy_init(void) { return reinterpret_cast<CURL *>(&fake_handle); }
void curl_easy_cleanup(CURL *) {}

CURLcode curl_easy_setopt(CURL *, CURLoption option, ...)
{
    if (state.failing_option == option) {
        state.failing_option = CURLOPT_LASTENTRY;
        return state.setopt_error;
    }

    va_list args;
    va_start(args, option);
    switch (option) {
        case CURLOPT_URL: {
            const char *value = va_arg(args, const char *);
            state.configured_url = value == nullptr ? "" : value;
            break;
        }
        case CURLOPT_POSTFIELDS: {
            const char *value = va_arg(args, const char *);
            state.configured_post_fields = value == nullptr ? "" : value;
            break;
        }
        case CURLOPT_WRITEDATA:
            state.write_data = va_arg(args, void *);
            break;
        case CURLOPT_WRITEFUNCTION:
            state.write_callback = va_arg(args, write_callback_t);
            break;
        case CURLOPT_POST:
            state.post = va_arg(args, long) != 0;
            break;
        default:
            (void)va_arg(args, void *);
            break;
    }
    va_end(args);
    return CURLE_OK;
}

CURLcode curl_easy_perform(CURL *)
{
    ++state.performs;
    if (state.perform_result != CURLE_OK) return state.perform_result;
    if (state.write_callback != nullptr && state.write_data != nullptr && !state.response.empty()) {
        const size_t written = state.write_callback(
            state.response.data(), 1, state.response.size(), state.write_data);
        if (written != state.response.size()) return CURLE_WRITE_ERROR;
    }
    return CURLE_OK;
}

struct curl_slist *curl_slist_append(struct curl_slist *list, const char *value)
{
    auto *node = static_cast<curl_slist *>(std::malloc(sizeof(curl_slist)));
    if (node == nullptr) return nullptr;
    const size_t length = value == nullptr ? 0 : std::strlen(value);
    node->data = static_cast<char *>(std::malloc(length + 1));
    if (node->data == nullptr) {
        std::free(node);
        return nullptr;
    }
    if (value != nullptr) std::memcpy(node->data, value, length);
    node->data[length] = '\0';
    node->next = nullptr;
    if (list == nullptr) return node;
    curl_slist *tail = list;
    while (tail->next != nullptr) tail = tail->next;
    tail->next = node;
    return list;
}

void curl_slist_free_all(struct curl_slist *list)
{
    while (list != nullptr) {
        curl_slist *next = list->next;
        std::free(list->data);
        std::free(list);
        list = next;
    }
}
}
