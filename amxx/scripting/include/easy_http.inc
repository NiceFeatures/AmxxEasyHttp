#if defined _easy_http_included
    #endinput
#endif
#define _easy_http_included

#pragma reqlib easy_http
#if !defined AMXMODX_NOAUTOLOAD
    #pragma loadlib easy_http
#endif

enum EzHttpErrorCode {
    EZH_OK = 0,
    EZH_CONNECTION_FAILURE,
    EZH_EMPTY_RESPONSE,
    EZH_HOST_RESOLUTION_FAILURE,
    EZH_INTERNAL_ERROR,
    EZH_INVALID_URL_FORMAT,
    EZH_NETWORK_RECEIVE_ERROR,
    EZH_NETWORK_SEND_FAILURE,
    EZH_OPERATION_TIMEDOUT,
    EZH_PROXY_RESOLUTION_FAILURE,
    EZH_SSL_CONNECT_ERROR,
    EZH_SSL_LOCAL_CERTIFICATE_ERROR,
    EZH_SSL_REMOTE_CERTIFICATE_ERROR,
    EZH_SSL_CACERT_ERROR,
    EZH_GENERIC_SSL_ERROR,
    EZH_UNSUPPORTED_PROTOCOL,
    EZH_REQUEST_CANCELLED,
    EZH_TOO_MANY_REDIRECTS,
    EZH_UNKNOWN_ERROR = 1000,
};

enum EzHttpProgress
{
    EZH_DownloadNow = 0,
    EZH_DownloadTotal,
    EZH_UploadNow,
    EZH_UploadTotal
};

enum EzHttpFtpSecurity
{
    EZH_UNSECURE = 0,
    EZH_SECURE_EXPLICIT
};

enum EzHttpRequest
{
    EzInvalid_Request = 0
};

enum EzHttpOptions
{
    EzInvalid_Options = 0
};

enum EzHttpQueue
{
    EzInvalid_Queue = 0
};

enum EzHttpPluginEndBehaviour
{
    EZH_CANCEL_REQUEST = 0,
    EZH_FORGET_REQUEST,
};

/*
 * JSON types
 */
enum EzJSONType
{
	EzJSONError   = -1,
	EzJSONNull    = 1,
	EzJSONString  = 2,
	EzJSONNumber  = 3,
	EzJSONObject  = 4,
	EzJSONArray   = 5,
	EzJSONBoolean = 6
};

/*
 * JSON invalid handle
 */
enum EzJSON
{
	EzInvalid_JSON = -1
}

/**
 * Helper macros for checking type
 */
#define ezjson_is_object(%1)   ((%1) != EzInvalid_JSON && ezjson_get_type(%1) == EzJSONObject)
#define ezjson_is_array(%1)    ((%1) != EzInvalid_JSON && ezjson_get_type(%1) == EzJSONArray)
#define ezjson_is_string(%1)   ((%1) != EzInvalid_JSON && ezjson_get_type(%1) == EzJSONString)
#define ezjson_is_number(%1)   ((%1) != EzInvalid_JSON && ezjson_get_type(%1) == EzJSONNumber)
#define ezjson_is_bool(%1)     ((%1) != EzInvalid_JSON && ezjson_get_type(%1) == EzJSONBoolean)
#define ezjson_is_null(%1)     ((%1) != EzInvalid_JSON && ezjson_get_type(%1) == EzJSONNull)
#define ezjson_is_true(%1)     ((%1) != EzInvalid_JSON && ezjson_is_bool(%1) && ezjson_get_bool(%1))
#define ezjson_is_false(%1)    ((%1) != EzInvalid_JSON && ezjson_is_bool(%1) && !ezjson_get_bool(%1))


/**
 * Creates new options object. This object allows you to configure your request by specifying 
 * such parameters as user agent, query parameters, headers, and etc.
 *
 * @return                  EzHttpOptions handle.
 */
native EzHttpOptions:ezhttp_create_options();

/**
 * Destroys options object.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().
 *
 * @noreturn
 */
native ezhttp_destroy_options(EzHttpOptions:options_id);

/**
 * Sets user-agent string for a request.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param user_agent        User-agent string.
 *
 * @noreturn
 */
native ezhttp_option_set_user_agent(EzHttpOptions:options_id, const user_agent[]);

/**
 * Adds a query parameter to the URL.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param key               Parameter key.
 * @param value             Parameter value.
 *
 * @noreturn
 */
native ezhttp_option_add_url_parameter(EzHttpOptions:options_id, const key[], const value[]);

/**
 * Adds a key-value pair to the form payload.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param key               Parameter key.
 * @param value             Parameter value.
 *
 * @noreturn
 */
native ezhttp_option_add_form_payload(EzHttpOptions:options_id, const key[], const value[]);

/**
 * Sets the request body.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param body              Request body.
 *
 * @noreturn
 */
native ezhttp_option_set_body(EzHttpOptions:options_id, const body[]);

/**
 * Copies serialized string to the requests body.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().  
 * @param json              EzJSON handle.
 * @param pretty            True to format pretty JSON string, false to not.
 *
 * @return                  True if serialization was successful, false otherwise.
 * @error                   If passed handle is not a valid value. If passed options_id is not exists.
 */
native bool:ezhttp_option_set_body_from_json(EzHttpOptions:options_id, EzJSON:json, bool:pretty = false);

/**
  * Appends a body to the HTTP request.
  *
  * @param options_id         Options identifier created via ezhttp_create_options().
  * @param body               The body to append.
  *
  * @noreturn
  */
 native ezhttp_option_append_body(EzHttpOptions:options_id, const body[]);

 /**
  * Sets a header for the HTTP request.
  *
  * @param options_id         Options identifier created via ezhttp_create_options().
  * @param key                The key of the header.
  * @param value              The value of the header.
  *
  * @noreturn
  */
 native ezhttp_option_set_header(EzHttpOptions:options_id, const key[], const value[]);

 /**
  * Sets a cookie for the HTTP request.
  *
  * @param options_id         Options identifier created via ezhttp_create_options().
  * @param key                The key of the cookie.
  * @param value              The value of the cookie.
  *
  * @noreturn
  */
 native ezhttp_option_set_cookie(EzHttpOptions:options_id, const key[], const value[]);

 /**
 * Sets a timeout for the HTTP request. 
 *
 * @note                     Specify 0 to disable timeout and make the request wait indefinitely.
 *                           Useful for long-polling.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param timeout_ms         The timeout to set in milliseconds.
 *
 * @noreturn
 */
native ezhttp_option_set_timeout(EzHttpOptions:options_id, timeout_ms);

/**
 * Sets a connect timeout for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param timeout_ms         The timeout to set in milliseconds.
 *
 * @noreturn
 */
native ezhttp_option_set_connect_timeout(EzHttpOptions:options_id, timeout_ms);

/**
 * Sets a proxy for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param proxy_url          The URL of the proxy.
 *
 * @noreturn
 */
native ezhttp_option_set_proxy(EzHttpOptions:options_id, const proxy_url[]);

/**
 * Sets a proxy authentication for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param user               The user name for the proxy authentication.
 * @param password           The password for the proxy authentication.
 *
 * @noreturn
 */
native ezhttp_option_set_proxy_auth(EzHttpOptions:options_id, const user[], const password[]);

/**
 * Sets an authentication for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param user               The user name for the authentication.
 * @param password           The password for the authentication.
 *
 * @noreturn
 */
native ezhttp_option_set_auth(EzHttpOptions:options_id, const user[], const password[]);

/**
 * Sets a custom request data for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param data               The user data to set.
 * @param len                The length of the user data.
 *
 * @noreturn
 */
native ezhttp_option_set_user_data(EzHttpOptions:options_id, const data[], len);

/**
 * Sets a plugin end behaviour for the HTTP request.
 *
 * @param options_id            Options identifier created via ezhttp_create_options().
 * @param plugin_end_behaviour  Member of EzHttpPluginEndBehaviour. The plugin end behaviour to set. 
 *                              Valid values are:
 *                              * EZH_CANCEL_REQUEST to cancel the request;
 *                              * EZH_FORGET_REQUEST to complete the request, but ignore its result
 *                              (callback will not be called).
 *
 * @noreturn
 */
native ezhttp_option_set_plugin_end_behaviour(
    EzHttpOptions:options_id,
    EzHttpPluginEndBehaviour:plugin_end_behaviour
);

/**
 * Sets a queue for the HTTP request.
 * Queues allow you to run all the requests in the queue sequentially (in the order they were called).
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param queue_id           The queue to for the request.
 *
 * @noreturn
 */
native ezhttp_option_set_queue(EzHttpOptions:options_id, EzHttpQueue:queue_id);

/**
 * Creates a new HTTP request queue. Requests in the queue are executed sequentially.
 *
 * @return                  EzHttpQueue queue handle.
 */
native EzHttpQueue:ezhttp_create_queue();

/**
 * Performs a GET request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Optional data buffer for binary payloads.
 * @param data_len          Length of the data buffer.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_get(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Performs a POST request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Optional data buffer for binary payloads.
 * @param data_len          Length of the data buffer.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_post(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Performs a PUT request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Optional data buffer for binary payloads.
 * @param data_len          Length of the data buffer.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_put(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Performs a PATCH request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Optional data buffer for binary payloads.
 * @param data_len          Length of the data buffer.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_patch(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Performs a DELETE request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Optional data buffer for binary payloads.
 * @param data_len          Length of the data buffer.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_delete(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Checks if a request exists.
 *
 * @param request_id        Request identifier.
 *
 * @return                  True if the request exists, false otherwise.
 */
native bool:ezhttp_is_request_exists(EzHttpRequest:request_id);

/**
 * Cancels a request.
 *
 * @param request_id        Request identifier.
 *
 * @noreturn
 */
native ezhttp_cancel_request(EzHttpRequest:request_id);

/**
 * Gets the request progress.
 *
 * @note                    It's not recommended to use this too frequently (every frame)
 *
 * @param request_id        Request identifier.
 * @param progress          Array to store the progress values in.
 *
 * @noreturn
 */
native ezhttp_request_progress(EzHttpRequest:request_id, progress[EzHttpProgress]);

/**
 * Gets the HTTP status code of the request.
 *
 * @note                    This native can only be used in the on_complete callback.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The HTTP status code.
 */
native ezhttp_get_http_code(EzHttpRequest:request_id);

/**
 * Gets the response data of the request.
 *
 * @note                    This native can only be used in the on_complete callback.
 *
 * @param request_id        The request identifier.
 * @param buffer            The buffer to store the data in.
 * @param max_len           The maximum length of the buffer.
 *
 * @noreturn
 */
native ezhttp_get_data(EzHttpRequest:request_id, buffer[], max_len);

/**
 * Parses http response body to JSON.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param request_id        request_id  
 * @param with_comments     True if parsing JSON includes comments (it will ignore them), false otherwise
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezhttp_parse_json_response(EzHttpRequest:request_id, bool:with_comments = false);

/**
 * Returns the URL of the request.
 *
 * @param request_id        The request identifier.
 * @param buffer            The buffer to store the URL.
 * @param max_len           The maximum length of the buffer.
 *
 * @noreturn
 */
native ezhttp_get_url(EzHttpRequest:request_id, buffer[], max_len);

/**
 * Saves the request data to a file.
 *
 * @param request_id        The request identifier.
 * @param file_path         The path to the file to save to. Must be relative to the mod directory.
 *
 * @return                  The number of bytes written to the file.
 */
native ezhttp_save_data_to_file(EzHttpRequest:request_id, const file_path[]);

/**
 * Saves the request data to a file.
 *
 * @param request_id        The request identifier.
 * @param file_handle       The file handle to write to. Must be opened via fopen with write permissions.
 *
 * @return                  The number of bytes written to the file.
 */
native ezhttp_save_data_to_file2(EzHttpRequest:request_id, file_handle);

/**
 * Returns the number of headers in the response.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of headers in the response.
 */
native ezhttp_get_headers_count(EzHttpRequest:request_id);

/**
 * Returns a header value by name.
 *
 * @param request_id        The request identifier.
 * @param key               The name of the header to retrieve.
 * @param value             The buffer to store the value.
 * @param max_len           The maximum length of the buffer.
 *
 * @return                  True if the header was found, false otherwise
 */
native bool:ezhttp_get_headers(EzHttpRequest:request_id, const key[], value[], max_len);

/**
 * Returns the elapsed time of the request in seconds.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The elapsed time in seconds.
 */
native Float:ezhttp_get_elapsed(EzHttpRequest:request_id);

/**
 * Returns the number of cookies in the response.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of cookies in the response.
 */
native ezhttp_get_cookies_count(EzHttpRequest:request_id);

/**
 * Returns a cookie value by name.
 *
 * @param request_id        The request identifier.
 * @param key               The name of the cookie to retrieve.
 * @param value             The buffer to store the value.
 * @param max_len           The maximum length of the buffer.
 *
 * @return                  True if the cookie was found, false otherwise
 */
native bool:ezhttp_get_cookies(EzHttpRequest:request_id, const key[], value[], max_len);

/**
 * Returns the error code of the connection failure of the request.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The error code of the request.
 */
native EzHttpErrorCode:ezhttp_get_error_code(EzHttpRequest:request_id);

/**
 * Returns the error message of the request.
 *
 * @param request_id        The request identifier.
 * @param buffer            The buffer to store the error message.
 * @param max_len           The maximum length of the buffer.
 *
 * @noreturn
 */
native ezhttp_get_error_message(EzHttpRequest:request_id, buffer[], max_len);

/**
 * Returns the number of redirects in the request.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of redirects in the request.
 */
native ezhttp_get_redirect_count(EzHttpRequest:request_id);

/**
 * Returns the number of bytes uploaded in the request.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of bytes uploaded in the request.
 */
native ezhttp_get_uploaded_bytes(EzHttpRequest:request_id);

/**
 * Returns the number of bytes downloaded in the request.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of bytes downloaded in the request.
 */
native ezhttp_get_downloaded_bytes(EzHttpRequest:request_id);

/**
 * Returns the custom data associated with the request set by ezhttp_option_set_user_data.
 *
 * @param request_id        The request identifier.
 * @param data              The buffer to store the user data.
 *
 * @noreturn
 */
native ezhttp_get_user_data(EzHttpRequest:request_id, data[]);

/**
 * Uploads a file to a remote server using FTP.
 *
 * @param user               The user name to use for the FTP connection.
 * @param password           The password to use for the FTP connection.
 * @param host               The host to connect to.
 * @param remote_file        The remote file to upload to.
 * @param local_file         The local file to upload.
 * @param on_complete        The function to call when the upload is complete.
 *                           Signature: public on_complete(EzHttpRequest:request_id)
 * @param security           Member of EzHttpFtpSecurity. The security strategy use for the FTP connection.
 * @param options_id         The options to use for the request.
 *
 * @return                   The request handle.
 */
native EzHttpRequest:ezhttp_ftp_upload(
    const user[] = "", 
    const password[] = "", 
    const host[] = "", 
    const remote_file[] = "", 
    const local_file[] = "", 
    const on_complete[] = "", 
    EzHttpFtpSecurity:security = EZH_UNSECURE, 
    EzHttpOptions:options_id = EzHttpOptions:0
);

/**
 * Uploads a file to a remote server using FTP by URI.
 *
 * @param uri                The URI to upload to.
 * @param local_file         The local file to upload.
 * @param on_complete        The function to call when the upload is complete.
 *                           Signature: public on_complete(EzHttpRequest:request_id)
 * @param security           Member of EzHttpFtpSecurity. The security strategy use for the FTP connection.
 * @param options_id         The options to use for the request.
 *
 * @return                   The request handle.
 */
native EzHttpRequest:ezhttp_ftp_upload2(
    const uri[] = "", 
    const local_file[] = "", 
    const on_complete[] = "", 
    EzHttpFtpSecurity:security = EZH_UNSECURE, 
    EzHttpOptions:options_id = EzHttpOptions:0
);

/**
 * Downloads a file from a remote server using FTP.
 *
 * @param user               The user name to use for the FTP connection.
 * @param password           The password to use for the FTP connection.
 * @param host               The host to connect to.
 * @param remote_file        The remote file to download.
 * @param local_file         The local file to save to.
 * @param on_complete        The function to call when the download is complete.
 *                           Signature: public on_complete(EzHttpRequest:request_id)
 * @param security           Member of EzHttpFtpSecurity. The security strategy use for the FTP connection.
 * @param options_id         The options to use for the request.
 *
 * @return                   The request handle.
 */
native EzHttpRequest:ezhttp_ftp_download(
    const user[] = "", 
    const password[] = "", 
    const host[] = "", 
    const remote_file[] = "", 
    const local_file[] = "", 
    const on_complete[] = "", 
    EzHttpFtpSecurity:security = EZH_UNSECURE, 
    EzHttpOptions:options_id = EzHttpOptions:0
);

/**
 * Downloads a file from a remote server using FTP by URI.
 *
 * @param uri                The URI to download from.
 * @param local_file         The local file to save to.
 * @param on_complete        The function to call when the download is complete.
 *                           Signature: public on_complete(EzHttpRequest:request_id)
 * @param security           Member of EzHttpFtpSecurity. The security strategy use for the FTP connection.
 * @param options_id         The options to use for the request.
 *
 * @return                   The request handle.
 */
native EzHttpRequest:ezhttp_ftp_download2(
    const uri[] = "", 
    const local_file[] = "", 
    const on_complete[] = "", 
    EzHttpFtpSecurity:security = EZH_UNSECURE, 
    EzHttpOptions:options_id = EzHttpOptions:0
);

/**
 * Downloads a file from a URL.
 *
 * @param url                URL to download from.
 * @param save_path          Local path to save the file (relative to mod directory).
 * @param on_complete        Function to call when the download is complete.
 *                           Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param data               Additional data to send with the request.
 * @param data_len           Length of the data.
 *
 * @return                   The request handle.
 */
native EzHttpRequest:ezhttp_download_url(
    const url[], 
    const save_path[], 
    const on_complete[], 
    EzHttpOptions:options_id = EzHttpOptions:0, 
    const data[] = {}, 
    data_len = 0
);

/**
 * Iterates over the response headers.
 *
 * @param request_id         The request handle.
 * @param callback           The function to call for each header.
 *                           Signature: public on_header(EzHttpRequest:request_id, const key[], const value[])
 *
 * @return                   True on success, false otherwise.
 */
native bool:ezhttp_iterate_headers(EzHttpRequest:request_id, const callback[]);

/**
 * Iterates over the response cookies.
 *
 * @param request_id         The request handle.
 * @param callback           The function to call for each cookie.
 *                           Signature: public on_cookie(EzHttpRequest:request_id, const name[], const value[])
 *
 * @return                   True on success, false otherwise.
 */
native bool:ezhttp_iterate_cookies(EzHttpRequest:request_id, const callback[]);

/**
 * Converts SteamID to SteamID64.
 *
 * @param steam_id          SteamID string.
 * @param buffer            Buffer to store SteamID64 string.
 * @param max_len           Maximum length of the buffer.
 *
 * @noreturn
 */
native ezhttp_steam_to_steam64(const steam_id[], buffer[], max_len);

/**
 * Parses JSON string or a file that contains JSON.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param string            String to parse
 * @param is_file           True to treat string param as filename, false otherwise
 * @param with_comments     True if parsing JSON includes comments (it will ignore them), false otherwise
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezjson_parse(const string[], bool:is_file = false, bool:with_comments = false);

/**
 * Checks if the first value is the same as the second one.
 *
 * @param value1            EzJSON handle
 * @param value2            EzJSON handle
 *
 * @return                  True if they are the same, false otherwise
 * @error                   If passed value is not a valid handle
 */
native bool:ezjson_equals(const EzJSON:value1, const EzJSON:value2);

/**
 * Validates json by checking if object have identically named
 * fields with matching types.
 *
 * @note                    Schema {"name":"", "age":0} will validate
 *                          {"name":"Joe", "age":25} and {"name":"Joe", "age":25, "gender":"m"},
 *                          but not {"name":"Joe"} or {"name":"Joe", "age":"Cucumber"}.
 *
 * @note                    In case of arrays, only first value in schema
 *                          is checked against all values in tested array.
 *
 * @note                    Empty objects ({}) validate all objects,
 *                          empty arrays ([]) validate all arrays,
 *                          null validates values of every type.
 *
 * @param schema            EzJSON handle
 * @param value             EzJSON handle
 *
 * @return                  True if passed value is valid, false otherwise
 * @error                   If a schema handle or value handle is invalid
 */
native bool:ezjson_validate(const EzJSON:schema, const EzJSON:value);

/**
 * Gets value's parent handle.
 *
 * @note                  Parent's handle needs to be freed using ezjson_free() native.
 *
 * @param value           EzJSON handle
 *
 * @return                Parent's handle
 */
native EzJSON:ezjson_get_parent(const EzJSON:value);

/**
 * Gets JSON type of passed value.
 *
 * @param value             EzJSON handle
 *
 * @return                  JSON type (EzJSONType constants)
 * @error                   If a value handle is invalid
 */
native EzJSONType:ezjson_get_type(const EzJSON:value);

/**
 * Inits an empty object.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezjson_init_object();

/**
 * Inits an empty array.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezjson_init_array();

/**
 * Inits string data.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param value             String that the handle will be initialized with
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezjson_init_string(const value[]);

/**
 * Inits a number.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param value             Integer number that the handle will be initialized with
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezjson_init_number(value);

/**
 * Inits a real number.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param value             Real number that the handle will be initialized with
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezjson_init_real(Float:value);

/**
 * Inits a boolean value.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param value             Boolean value that the handle will be initialized with
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezjson_init_bool(bool:value);

/**
 * Inits a null.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezjson_init_null();

/**
 * Creates deep copy of passed value.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param value             EzJSON handle to be copied
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 * @error                   If passed value is not a valid handle
 */
native EzJSON:ezjson_deep_copy(const EzJSON:value);

/**
 * Frees handle.
 *
 * @param handle            EzJSON handle to be freed
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid handle
 */
native bool:ezjson_free(&EzJSON:handle);

/**
 * Gets string data.
 *
 * @param value             EzJSON handle
 * @param buffer            Buffer to copy string to
 * @param maxlen            Maximum size of the buffer
 *
 * @return                  The number of cells written to the buffer
 * @error                   If passed value is not a valid handle
 */
native ezjson_get_string(const EzJSON:value, buffer[], maxlen);

/**
 * Gets a number.
 *
 * @param value             EzJSON handle
 *
 * @return                  Number
 * @error                   If passed value is not a valid handle
 */
native ezjson_get_number(const EzJSON:value);

/**
 * Gets a real number.
 *
 * @param value             EzJSON handle
 *
 * @return                  Real number
 * @error                   If passed value is not a valid handle
 */
native Float:ezjson_get_real(const EzJSON:value);

/**
 * Gets a boolean value.
 *
 * @param value             EzJSON handle
 *
 * @return                  Boolean value
 * @error                   If passed value is not a valid handle
 */
native bool:ezjson_get_bool(const EzJSON:value);

/**
 * Gets a value from the array.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param array             Array handle
 * @param index             Position in the array (starting from 0)
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 * @error                   If passed handle is not a valid array
 */
native EzJSON:ezjson_array_get_value(const EzJSON:array, index);

/**
 * Gets string data from the array.
 *
 * @param array             Array handle
 * @param index             Position in the array (starting from 0)
 * @param buffer            Buffer to copy string to
 * @param maxlen            Maximum size of the buffer
 *
 * @return                  The number of cells written to the buffer
 * @error                   If passed handle is not a valid array
 */
native ezjson_array_get_string(const EzJSON:array, index, buffer[], maxlen);

/**
 * Gets a number from the array.
 *
 * @param array             Array handle
 * @param index             Position in the array (starting from 0)
 *
 * @return                  The number as integer
 * @error                   If passed handle is not a valid array
 */
native ezjson_array_get_number(const EzJSON:array, index);

/**
 * Gets a real number from the array.
 *
 * @param array             Array handle
 * @param index             Position in the array (starting from 0)
 *
 * @return                  The number as float
 * @error                   If passed handle is not a valid array
 */
native Float:ezjson_array_get_real(const EzJSON:array, index);

/**
 * Gets a boolean value from the array.
 *
 * @param array             Array handle
 * @param index             Position in the array (starting from 0)
 *
 * @return                  Boolean value
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_get_bool(const EzJSON:array, index);

/**
 * Gets count of the elements in the array.
 *
 * @param array             Array handle
 *
 * @return                  Number of elements in the array
 * @error                   If passed handle is not a valid array
 */
native ezjson_array_get_count(const EzJSON:array);

/**
 * Replaces an element in the array with value.
 *
 * @param array             Array handle
 * @param index             Position in the array to be replaced
 * @param value             EzJSON handle to set
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_replace_value(EzJSON:array, index, const EzJSON:value);

/**
 * Replaces an element in the array with string data.
 *
 * @param array             Array handle
 * @param index             Position in the array to be replaced
 * @param string            String to copy
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_replace_string(EzJSON:array, index, const string[]);

/**
 * Replaces an element in the array with number.
 *
 * @param array             Array handle
 * @param index             Position in the array to be replaced
 * @param number            Number to set
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_replace_number(EzJSON:array, index, number);

/**
 * Replaces an element in the array with real number.
 *
 * @param array             Array handle
 * @param index             Position in the array to be replaced
 * @param number            Real number to set
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_replace_real(EzJSON:array, index, Float:number);

/**
 * Replaces an element in the array with boolean value.
 *
 * @param array             Array handle
 * @param index             Position in the array to be replaced
 * @param boolean           Boolean value to set
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_replace_bool(EzJSON:array, index, bool:boolean);

/**
 * Replaces an element in the array with null.
 *
 * @param array             Array handle
 * @param index             Position in the array to be replaced
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_replace_null(EzJSON:array, index);

/**
 * Appends a value in the array.
 *
 * @param array             Array handle
 * @param value             EzJSON handle to set
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_append_value(EzJSON:array, const EzJSON:value);

/**
 * Appends string data in the array.
 *
 * @param array             Array handle
 * @param string            String to copy
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_append_string(EzJSON:array, const string[]);

/**
 * Appends a number in the array.
 *
 * @param array             Array handle
 * @param number            Number to set
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_append_number(EzJSON:array, number);

/**
 * Appends a real number in the array.
 *
 * @param array             Array handle
 * @param number            Real number to set
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_append_real(EzJSON:array, Float:number);

/**
 * Appends a boolean value in the array.
 *
 * @param array             Array handle
 * @param boolean           Boolean value to set
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_append_bool(EzJSON:array, bool:boolean);

/**
 * Appends a null in the array.
 *
 * @param array             Array handle
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_append_null(EzJSON:array);

/**
 * Removes an element from the array.
 *
 * @note                    Order of values in array may change during execution.
 *
 * @param array             Array handle
 * @param index             Position in the array (starting from 0)
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_remove(EzJSON:array, index);

/**
 * Removes all elements from the array.
 *
 * @param array             Array handle
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid array
 */
native bool:ezjson_array_clear(EzJSON:array);

/**
 * Gets a value from the object.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 * @error                   If passed handle is not a valid object
 */
native EzJSON:ezjson_object_get_value(const EzJSON:object, const name[], bool:dot_not = false);

/**
 * Gets string data from the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param buffer            Buffer to copy string to
 * @param maxlen            Maximum size of the buffer
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  The number of cells written to the buffer
 * @error                   If passed handle is not a valid object
 */
native ezjson_object_get_string(const EzJSON:object, const name[], buffer[], maxlen, bool:dot_not = false);

/**
 * Gets a number from the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  Number
 * @error                   If passed handle is not a valid object
 */
native ezjson_object_get_number(const EzJSON:object, const name[], bool:dot_not = false);

/**
 * Gets a real number from the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  Real number
 * @error                   If passed handle is not a valid object
 */
native Float:ezjson_object_get_real(const EzJSON:object, const name[], bool:dot_not = false);

/**
 * Gets a boolean value from the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  Boolean value
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_get_bool(const EzJSON:object, const name[], bool:dot_not = false);

/**
 * Gets count of the keys in the object.
 *
 * @param object            Object handle
 *
 * @return                  Keys count
 * @error                   If passed handle is not a valid object
 */
native ezjson_object_get_count(const EzJSON:object);

/**
 * Gets name of the object's key.
 *
 * @param object            Object handle
 * @param index             Position from which get key name
 * @param buffer            Buffer to copy string to
 * @param maxlen            Maximum size of the buffer
 *
 * @return                  The number of cells written to the buffer
 * @error                   If passed handle is not a valid object
 */
native ezjson_object_get_name(const EzJSON:object, index, buffer[], maxlen);

/**
 * Gets a value at the specified position from the object.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param object            Object handle
 * @param index             Position from which get key name
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 * @error                   If passed handle is not a valid object
 */
native EzJSON:ezjson_object_get_value_at(const EzJSON:object, index);

/**
 * Checks if the object has a value with a specific name and type.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param type              Type of value, if EzJSONError type will not be checked
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  True if has, false if not
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_has_value(const EzJSON:object, const name[], EzJSONType:type = EzJSONError, bool:dot_not = false);

/**
 * Sets a value in the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 * @note                    It also removes the old value if any.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param value             EzJSON handle to set
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_set_value(EzJSON:object, const name[], const EzJSON:value, bool:dot_not = false);

/**
 * Sets string data in the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 * @note                    It also removes the old value if any.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param string            String to copy
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_set_string(EzJSON:object, const name[], const string[], bool:dot_not = false);

/**
 * Sets a number in the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 * @note                    It also removes the old value if any.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param number            Number to set
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_set_number(EzJSON:object, const name[], number, bool:dot_not = false);

/**
 * Sets a real number in the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 * @note                    It also removes the old value if any.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param number            Real number to set
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_set_real(EzJSON:object, const name[], Float:number, bool:dot_not = false);

/**
 * Sets a boolean value in the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 * @note                    It also removes the old value if any.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param boolean           Boolean value to set
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_set_bool(EzJSON:object, const name[], bool:boolean, bool:dot_not = false);

/**
 * Sets a null in the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 * @note                    It also removes the old value if any.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_set_null(EzJSON:object, const name[], bool:dot_not = false);

/**
 * Removes a key and its value in the object.
 *
 * @note                    If dot notation is used some values may be inaccessible
 *                          because valid names in JSON can contain dots.
 *
 * @param object            Object handle
 * @param name              Key name
 * @param dot_not           True to use dot notation, false to not
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_remove(EzJSON:object, const name[], bool:dot_not = false);

/**
 * Removes all keys and their values in the object.
 *
 * @param object            Object handle
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid object
 */
native bool:ezjson_object_clear(EzJSON:object);

/**
 * Gets size of serialization.
 *
 * @param value             EzJSON handle
 * @param pretty            True to count size for pretty format, false to not
 * @param null_byte         True to include null byte, false to not
 *
 * @return                  Size of serialized string
 * @error                   If passed handle is not a valid value
 */
native ezjson_serial_size(const EzJSON:value, bool:pretty = false, bool:null_byte = false);

/**
 * Copies serialized string to the buffer.
 *
 * @param value             EzJSON handle
 * @param buffer            Buffer to copy string to
 * @param maxlen            Maximum size of the buffer
 * @param pretty            True to format pretty JSON string, false to not
 *
 * @return                  The number of cells written to the buffer
 * @error                   If passed handle is not a valid value
 */
native ezjson_serial_to_string(const EzJSON:value, buffer[], maxlen, bool:pretty = false);

/**
 * Copies serialized string to the file.
 *
 * @param value             EzJSON handle
 * @param file              Path to the file
 * @param pretty            True to format pretty JSON string, false to not
 *
 * @return                  True if succeed, false otherwise
 * @error                   If passed handle is not a valid value
 */
native bool:ezjson_serial_to_file(const EzJSON:value, const file[], bool:pretty = false);
