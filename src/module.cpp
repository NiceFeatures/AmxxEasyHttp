#include <algorithm>
#include <utility>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <memory>
#include <cstdlib>
#include <cerrno>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <sdk/amxxmodule.h>

#include "EasyHttpModule.h"
#include "json/JsonMngr.h"
#include "json/JsonNatives.h"
#include "utils/ftp_utils.h"
#include "utils/string_utils.h"
#include "utils/amxx_utils.h"

using namespace ezhttp;

bool ValidateOptionsId(AMX* amx, OptionsId options_id);
bool ValidateRequestId(AMX* amx, RequestId request_id);
bool ValidateQueueId(AMX* amx, QueueId queue_id);
template <class TMethod> void SetKeyValueOption(AMX* amx, cell* params, TMethod method);
template <class TMethod> void SetStringOption(AMX* amx, cell* params, TMethod method);

// Safe path validation with try/catch to prevent HLDS crashes from filesystem exceptions
bool ValidatePath(const char* native_name, const char* file_path, std::string& resolved_path_out)
{
    try
    {
        std::error_code ec;
        std::filesystem::path base_path = std::filesystem::weakly_canonical(MF_BuildPathname("."), ec);
        if (ec)
        {
            MF_Log("%s: Failed to resolve base path: %s", native_name, ec.message().c_str());
            return false;
        }

        std::filesystem::path target_path = std::filesystem::weakly_canonical(base_path / file_path, ec);
        if (ec)
        {
            MF_Log("%s: Failed to resolve target path '%s': %s", native_name, file_path, ec.message().c_str());
            return false;
        }

        std::string base_str = base_path.string();
        std::string target_str = target_path.string();

        if (target_str.find(base_str) != 0)
        {
            MF_Log("%s: Path traversal attempt blocked: %s", native_name, file_path);
            return false;
        }

        resolved_path_out = target_str;
        return true;
    }
    catch (const std::exception& e)
    {
        MF_Log("%s: Filesystem exception for path '%s': %s", native_name, file_path, e.what());
        return false;
    }
    catch (...)
    {
        MF_Log("%s: Unknown filesystem exception for path '%s'", native_name, file_path);
        return false;
    }
}

struct NativeRequestParams {
    std::string url;
    std::string callback;
    OptionsId options_id;
    std::shared_ptr<std::vector<cell>> user_data;
};

NativeRequestParams ExtractRequestParams(AMX* amx, cell* params) {
    enum { arg_count, arg_url, arg_callback, arg_option_id, arg_data, arg_data_len };
    
    // params[0] contains the number of bytes for the arguments (count * sizeof(cell))
    int arg_num = params[0] / sizeof(cell);

    int url_len = 0;
    char* url_ptr = MF_GetAmxString(amx, params[arg_url], 0, &url_len);
    if (!url_ptr) {
        throw std::runtime_error("ezhttp: URL pointer is null");
    }

    int callback_len = 0;
    char* callback_ptr = nullptr;
    if (arg_num >= 2) {
        callback_ptr = MF_GetAmxString(amx, params[arg_callback], 1, &callback_len);
    }
    std::string callback_str = (callback_ptr) ? std::string(callback_ptr, callback_len) : "";

    OptionsId options_id = OptionsId::Null;
    if (arg_num >= 3) {
        options_id = (OptionsId)params[arg_option_id];
    }

    std::shared_ptr<std::vector<cell>> user_data = nullptr;
    if (arg_num >= 5) {
        int data_len = params[arg_data_len];
        if (data_len > 0) {
            // Safety limit: 10MB (2.5 million cells). 
            // This prevents "vector too long" and OOM crashes from garbage data_len.
            if (data_len > 2500000) {
                throw std::runtime_error("ezhttp: data_len exceeds safety limit (10MB)");
            }

            user_data = std::make_shared<std::vector<cell>>(data_len);
            MF_CopyAmxMemory(user_data->data(), MF_GetAmxAddr(amx, params[arg_data]), data_len);
        }
    }
    return { std::string(url_ptr, url_len), callback_str, options_id, user_data };
}

RequestId SendRequest(AMX* amx, RequestMethod method, OptionsId options_id, const std::string& url, const std::string& callback, std::shared_ptr<std::vector<cell>> user_data = nullptr);

std::unique_ptr<EasyHttpModule> g_EasyHttpModule;
std::unique_ptr<JSONMngr> g_JsonManager;

void CreateModules()
{
    g_EasyHttpModule = std::make_unique<EasyHttpModule>(MF_BuildPathname("addons/amxmodx/data/amxx_easy_http_cacert.pem"));
    g_JsonManager = std::make_unique<JSONMngr>();
}

void DestroyModules()
{
    g_EasyHttpModule.reset();
    g_JsonManager.reset();
}

// native EzHttpOptions:ezhttp_create_options();
cell AMX_NATIVE_CALL ezhttp_create_options(AMX* amx, cell* params)
{
    OptionsId options_id = g_EasyHttpModule->CreateOptions();

    return (int)options_id;
}

// native ezhttp_destroy_options(EzHttpOptions:options_id);
cell AMX_NATIVE_CALL ezhttp_destroy_options(AMX* amx, cell* params)
{
    auto options_id = (OptionsId)params[1];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    return g_EasyHttpModule->DeleteOptions(options_id);
}

// native ezhttp_option_set_user_agent(EzHttpOptions:options_id, const user_agent[]);
cell AMX_NATIVE_CALL ezhttp_option_set_user_agent(AMX* amx, cell* params)
{
    SetStringOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetUserAgent);
    return 0;
}

// native ezhttp_option_add_url_parameter(EzHttpOptions:options_id, const key[], const value[]);
cell AMX_NATIVE_CALL ezhttp_option_add_url_parameter(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::AddUrlParameter);
    return 0;
}

// native ezhttp_option_add_form_payload(EzHttpOptions:options_id, const key[], const value[]);
cell AMX_NATIVE_CALL ezhttp_option_add_form_payload(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::AddFormPayload);
    return 0;
}

// native ezhttp_option_set_body(EzHttpOptions:options_id, const body[]);
cell AMX_NATIVE_CALL ezhttp_option_set_body(AMX* amx, cell* params)
{
    SetStringOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetBody);
    return 0;
}

// native bool:ezhttp_option_set_body_from_json(EzHttpOptions:options_id, EzJSON:json, bool:pretty = false);
cell AMX_NATIVE_CALL ezhttp_option_set_body_from_json(AMX* amx, cell* params)
{
    auto options_id = (OptionsId)params[1];
    auto json_handle = (JS_Handle)params[2];
    auto pretty = (bool)params[3];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    if (!g_JsonManager->IsValidHandle(json_handle))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid JSON handle! %d", json_handle);
        return 0;
    }

    char* json_str = g_JsonManager->SerialToString(json_handle, pretty);
    if (json_str == nullptr)
        return 0;

    g_EasyHttpModule->GetOptions(options_id).options_builder.SetBody(json_str);
    g_JsonManager->FreeString(json_str);

    return 1;
}

// native ezhttp_option_append_body(EzHttpOptions:options_id, const body[]);
cell AMX_NATIVE_CALL ezhttp_option_append_body(AMX* amx, cell* params)
{
    SetStringOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::AppendBody);
    return 0;
}

// native ezhttp_option_set_header(EzHttpOptions:options_id, const key[], const value[]);
cell AMX_NATIVE_CALL ezhttp_option_set_header(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetHeader);
    return 0;
}

// native ezhttp_option_set_cookie(EzHttpOptions:options_id, const key[], const value[]);
cell AMX_NATIVE_CALL ezhttp_option_set_cookie(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetCookie);
    return 0;
}

// native ezhttp_option_set_timeout(EzHttpOptions:options_id, timeout_ms);
cell AMX_NATIVE_CALL ezhttp_option_set_timeout(AMX* amx, cell* params)
{
    auto options_id = (OptionsId)params[1];
    cell timeout_ms = params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    g_EasyHttpModule->GetOptions(options_id).options_builder.SetTimeout(timeout_ms);
    return 0;
}

// native ezhttp_option_set_connect_timeout(EzHttpOptions:options_id, timeout_ms);
cell AMX_NATIVE_CALL ezhttp_option_set_connect_timeout(AMX* amx, cell* params)
{
    auto options_id = (OptionsId)params[1];
    cell timeout_ms = params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    g_EasyHttpModule->GetOptions(options_id).options_builder.SetConnectTimeout(timeout_ms);
    return 0;
}

// native ezhttp_option_set_proxy(EzHttpOptions:options_id, const proxy_url[]);
cell AMX_NATIVE_CALL ezhttp_option_set_proxy(AMX* amx, cell* params)
{
    SetStringOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetProxy);
    return 0;
}

// native ezhttp_option_set_proxy_auth(EzHttpOptions:options_id, const user[], const password[]);
cell AMX_NATIVE_CALL ezhttp_option_set_proxy_auth(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetProxyAuth);
    return 0;
}

// native ezhttp_option_set_auth(EzHttpOptions:options_id, const user[], const password[]);
cell AMX_NATIVE_CALL ezhttp_option_set_auth(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetAuth);
    return 0;
}

// native ezhttp_option_set_user_data(EzHttpOptions:options_id, const data[], len);
cell AMX_NATIVE_CALL ezhttp_option_set_user_data(AMX* amx, cell* params)
{
    try {
        auto options_id = (OptionsId)params[1];
        cell* data_addr = MF_GetAmxAddr(amx, params[2]);
        int data_len = params[3];

        if (!ValidateOptionsId(amx, options_id))
            return 0;

        if (!data_addr) {
            MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_option_set_user_data: data pointer is null");
            return 0;
        }

        if (data_len > 2500000) {
            throw std::runtime_error("ezhttp: data_len exceeds safety limit (10MB)");
        }

        auto user_data = std::make_shared<std::vector<cell>>(data_len);
        MF_CopyAmxMemory(user_data->data(), data_addr, data_len);

        g_EasyHttpModule->GetOptions(options_id).user_data = user_data;
        return 0;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_option_set_user_data: %s", e.what());
        return 0;
    }
}


// native ezhttp_option_set_plugin_end_behaviour(EzHttpOptions:options_id, EzHttpPluginEndBehaviour:plugin_end_behaviour);
cell AMX_NATIVE_CALL ezhttp_option_set_plugin_end_behaviour(AMX* amx, cell* params)
{
    auto options_id = (OptionsId)params[1];
    auto plugin_end_behaviour = (PluginEndBehaviour)params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    g_EasyHttpModule->GetOptions(options_id).plugin_end_behaviour = plugin_end_behaviour;
    return 0;
}

// native ezhttp_option_set_queue(EzHttpOptions:options_id, EzHttpQueue:end_map_behaviour);
cell AMX_NATIVE_CALL ezhttp_option_set_queue(AMX* amx, cell* params)
{
    auto options_id = (OptionsId)params[1];
    auto queue_id = (QueueId)params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    if (!ValidateQueueId(amx, queue_id))
        return 0;

    g_EasyHttpModule->GetOptions(options_id).queue_id = queue_id;
    return 0;
}

// native EzHttpRequest:ezhttp_get(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_get(AMX* amx, cell* params)
{
    try {
        auto req = ExtractRequestParams(amx, params);
        return (cell)SendRequest(amx, RequestMethod::HttpGet, req.options_id, req.url, req.callback, req.user_data);
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_get: %s", e.what());
        return 0;
    }
}

// native EzHttpRequest:ezhttp_post(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_post(AMX* amx, cell* params)
{
    try {
        auto req = ExtractRequestParams(amx, params);
        return (cell)SendRequest(amx, RequestMethod::HttpPost, req.options_id, req.url, req.callback, req.user_data);
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_post: %s", e.what());
        return 0;
    }
}

// native EzHttpRequest:ezhttp_put(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_put(AMX* amx, cell* params)
{
    try {
        auto req = ExtractRequestParams(amx, params);
        return (cell)SendRequest(amx, RequestMethod::HttpPut, req.options_id, req.url, req.callback, req.user_data);
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_put: %s", e.what());
        return 0;
    }
}

// native EzHttpRequest:ezhttp_patch(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_patch(AMX* amx, cell* params)
{
    try {
        auto req = ExtractRequestParams(amx, params);
        return (cell)SendRequest(amx, RequestMethod::HttpPatch, req.options_id, req.url, req.callback, req.user_data);
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_patch: %s", e.what());
        return 0;
    }
}

// native EzHttpRequest:ezhttp_delete(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_delete(AMX* amx, cell* params)
{
    try {
        auto req = ExtractRequestParams(amx, params);
        return (cell)SendRequest(amx, RequestMethod::HttpDelete, req.options_id, req.url, req.callback, req.user_data);
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_delete: %s", e.what());
        return 0;
    }
}

// native EzHttpRequest:ezhttp_download_url(const url[], const save_path[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0, const data[] = "", data_len = 0);
cell AMX_NATIVE_CALL ezhttp_download_url(AMX* amx, cell* params)
{
    try {
        enum { arg_count, arg_url, arg_save_path, arg_callback, arg_option_id, arg_data, arg_data_len };

        int arg_num = params[0] / sizeof(cell);

        int url_len;
        char* url = MF_GetAmxString(amx, params[arg_url], 0, &url_len);
        if (!url) throw std::runtime_error("URL pointer is null");

        int save_path_len;
        char* save_path = MF_GetAmxString(amx, params[arg_save_path], 1, &save_path_len);
        if (!save_path) throw std::runtime_error("Save path pointer is null");

        int callback_len = 0;
        char* callback = nullptr;
        if (arg_num >= 3) {
            callback = MF_GetAmxString(amx, params[arg_callback], 2, &callback_len);
        }

        std::shared_ptr<std::vector<cell>> user_data = nullptr;
        if (arg_num >= 6)
        {
            int data_len = params[arg_data_len];
            if (data_len > 0)
            {
                // Safety limit: 10MB (2.5 million cells). 
                if (data_len > 2500000) {
                    throw std::runtime_error("ezhttp: data_len exceeds safety limit (10MB)");
                }
                user_data = std::make_shared<std::vector<cell>>(data_len);
                MF_CopyAmxMemory(user_data->data(), MF_GetAmxAddr(amx, params[arg_data]), data_len);
            }
        }

        OptionsId options_id = OptionsId::Null;
        if (arg_num >= 4) {
            options_id = (OptionsId)params[arg_option_id];
        }

        // Path Traversal Protection
        std::string resolved_save_path;
        if (!ValidatePath("ezhttp_download_url", save_path, resolved_save_path))
            return 0;

        if (!ValidateOptionsId(amx, options_id))
            return 0;

        g_EasyHttpModule->GetOptions(options_id).options_builder.SetFilePath(resolved_save_path);

        return (cell)SendRequest(amx, RequestMethod::HttpDownload, options_id, std::string(url, url_len), (callback) ? std::string(callback, callback_len) : "", user_data);
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_download_url: %s", e.what());
        return 0;
    }
}

// native ezhttp_is_request_exists(EzHttpRequest:request_id);
cell AMX_NATIVE_CALL ezhttp_is_request_exists(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    return g_EasyHttpModule->IsRequestExists(request_id);
}

// native ezhttp_cancel_request(EzHttpRequest:request_id);
cell AMX_NATIVE_CALL ezhttp_cancel_request(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    RequestData& request_data = g_EasyHttpModule->GetRequest(request_id);

    request_data.request_control->canceled.store(true);

    return 0;
}

// native ezhttp_request_progress(EzHttpRequest:request_id, progress[EzHttpProgress]);
cell AMX_NATIVE_CALL ezhttp_request_progress(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    auto& request_control = g_EasyHttpModule->GetRequest(request_id).request_control;
    auto progress = request_control->GetProgress();

    cell* p = MF_GetAmxAddr(amx, params[2]);
    p[0] = progress.download_now;
    p[1] = progress.download_total;
    p[2] = progress.upload_now;
    p[3] = progress.upload_total;

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_http_code(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.status_code;
}

cell AMX_NATIVE_CALL ezhttp_get_data(AMX* amx, cell* params)
{
    try {
        int arg_num = params[0] / sizeof(cell);
        auto request_id = (RequestId)params[1];
        cell max_len = 0;
        if (arg_num >= 3) {
            max_len = params[3];
        }

        if (!ValidateRequestId(amx, request_id) || max_len <= 0)
            return 0;

        const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

        utils::SetAmxStringUTF8CharSafe(amx, params[2], response.text.c_str(), response.text.length(), max_len);

        return 1;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_get_data: %s", e.what());
        return 0;
    }
}

// native EzJSON:ezhttp_parse_json_response(EzHttpRequest:request_id, bool:with_comments = false);
cell AMX_NATIVE_CALL ezhttp_parse_json_response(AMX* amx, cell* params)
{
    try {
        int arg_num = params[0] / sizeof(cell);
        auto request_id = (RequestId)params[1];
        bool with_comments = false;
        if (arg_num >= 2) {
            with_comments = (bool)params[2];
        }

        if (!ValidateRequestId(amx, request_id))
            return 0;

        const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

        JS_Handle json_handle;
        bool result = g_JsonManager->Parse(response.text.c_str(), &json_handle, false, with_comments);

        return result ? json_handle : -1;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_parse_json_response: %s", e.what());
        return -1;
    }
}

cell AMX_NATIVE_CALL ezhttp_get_url(AMX* amx, cell* params)
{
    try {
        int arg_num = params[0] / sizeof(cell);
        auto request_id = (RequestId)params[1];
        cell max_len = 0;
        if (arg_num >= 3) {
            max_len = params[3];
        }

        if (!ValidateRequestId(amx, request_id) || max_len <= 0)
            return 0;

        const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

        utils::SetAmxStringUTF8CharSafe(amx, params[2], response.url.c_str(), response.url.str().length(), max_len);

        return 1;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_get_url: %s", e.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL ezhttp_save_data_to_file(AMX* amx, cell* params)
{
    try {
        auto request_id = (RequestId)params[1];

        int file_path_len;
        char* file_path = MF_GetAmxString(amx, params[2], 0, &file_path_len);
        if (!file_path) throw std::runtime_error("File path pointer is null");

        if (!ValidateRequestId(amx, request_id))
            return 0;

        const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

        if (response.text.empty())
            return 0;

        std::string resolved_path;
        if (!ValidatePath("ezhttp_save_data_to_file", file_path, resolved_path))
            return 0;

        std::ofstream file(resolved_path, std::ofstream::out | std::ofstream::binary);
        if (!file.is_open()) {
            MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_save_data_to_file: Could not open file for writing: %s", resolved_path.c_str());
            return 0;
        }

        file.write(response.text.data(), response.text.length());
        file.close();

        return (cell)response.text.length();
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_save_data_to_file: %s", e.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL ezhttp_save_data_to_file2(AMX* amx, cell* params)
{
    try {
        auto request_id = (RequestId)params[1];
        cell file_cell = params[2];

        if (!ValidateRequestId(amx, request_id))
            return 0;

        if (file_cell == 0)
        {
            MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_save_data_to_file2: Invalid file handle (null)");
            return 0;
        }

        // Safety: for HLDS (32-bit), cell is 32-bit and usually matches FILE*.
        // However, this is inherently unsafe if the handle is not a valid FILE*.
        FILE* file_handle = reinterpret_cast<FILE*>(static_cast<uintptr_t>(static_cast<ucell>(file_cell)));
        if (!file_handle) {
            MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_save_data_to_file2: Resolved file handle is null");
            return 0;
        }

        const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

        if (response.text.empty())
            return 0;

        size_t written = std::fwrite(response.text.data(), sizeof(char), response.text.length(), file_handle);
        return (cell)written;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_save_data_to_file2: %s", e.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL ezhttp_get_headers_count(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.header.size();
}

cell AMX_NATIVE_CALL ezhttp_get_headers(AMX* amx, cell* params)
{
    try {
        auto request_id = (RequestId)params[1];
        int key_len;
        char* key = MF_GetAmxString(amx, params[2], 0, &key_len);
        cell value_max_len = params[4];

        if (!ValidateRequestId(amx, request_id))
            return 0;

        const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

        if (key && response.header.count(key))
        {
            utils::SetAmxStringUTF8CharSafe(amx, params[3], response.header.at(key).c_str(), response.header.at(key).length(), value_max_len);
            return 1;
        }

        return 0;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_get_headers: %s", e.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL ezhttp_iterate_headers(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];
    int callback_len;
    char* callback_name = MF_GetAmxString(amx, params[2], 0, &callback_len);

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const RequestData& request = g_EasyHttpModule->GetRequest(request_id);
    int func_index = MF_RegisterSPForwardByName(amx, callback_name, FP_ARRAY, FP_STRING, FP_STRING, FP_DONE);

    if (func_index == -1)
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Function \"%s\" not found", callback_name);
        return 0;
    }

    for (const auto& it : request.response.header)
    {
        MF_ExecuteForward(func_index, (cell)request_id, it.first.c_str(), it.second.c_str());
    }

    MF_UnregisterSPForward(func_index);
    return 1;
}

cell AMX_NATIVE_CALL ezhttp_get_elapsed(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    return amx_ftoc(response.elapsed);
}

cell AMX_NATIVE_CALL ezhttp_get_cookies_count(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    size_t size = response.cookies.end() - response.cookies.begin();
    return (cell)size;
}

cell AMX_NATIVE_CALL ezhttp_get_cookies(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];
    int key_len;
    char* key = MF_GetAmxString(amx, params[2], 0, &key_len);
    cell value_max_len = params[4];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    const std::string cookie_key(key, key_len);

    auto it = std::find_if(response.cookies.begin(), response.cookies.end(), [&cookie_key](const auto& item) { return item.GetName() == cookie_key; });
    if (it != response.cookies.end())
    {
        const std::string& cookie_value = it->GetValue();

        utils::SetAmxStringUTF8CharSafe(amx, params[3], cookie_value.c_str(), cookie_value.length(), value_max_len);

        return 1;
    }

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_iterate_cookies(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];
    int callback_len;
    char* callback_name = MF_GetAmxString(amx, params[2], 0, &callback_len);

    if (!ValidateRequestId(amx, request_id))
        return 0;

    Response& response = g_EasyHttpModule->GetRequest(request_id).response;
    int func_index = MF_RegisterSPForwardByName(amx, callback_name, FP_ARRAY, FP_STRING, FP_STRING, FP_DONE);

    if (func_index == -1)
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Function \"%s\" not found", callback_name);
        return 0;
    }

    for (auto it = response.cookies.begin(); it != response.cookies.end(); ++it)
    {
        MF_ExecuteForward(func_index, (cell)request_id, it->GetName().c_str(), it->GetValue().c_str());
    }

    MF_UnregisterSPForward(func_index);
    return 1;
}

cell AMX_NATIVE_CALL ezhttp_get_error_code(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    return (cell)response.error.code;
}

cell AMX_NATIVE_CALL ezhttp_get_error_message(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];
    cell max_len = params[3];

    if (!ValidateRequestId(amx, request_id) || max_len == 0)
        return 0;

    const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    utils::SetAmxStringUTF8CharSafe(amx, params[2], response.error.message.c_str(), response.error.message.length(), max_len);

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_redirect_count(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.redirect_count;
}

cell AMX_NATIVE_CALL ezhttp_get_uploaded_bytes(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.uploaded_bytes;
}

cell AMX_NATIVE_CALL ezhttp_get_downloaded_bytes(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response& response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.downloaded_bytes;
}

cell AMX_NATIVE_CALL ezhttp_get_user_data(AMX* amx, cell* params)
{
    auto request_id = (RequestId)params[1];
    cell* data_addr = MF_GetAmxAddr(amx, params[2]);

    if (!ValidateRequestId(amx, request_id))
        return 0;

    std::shared_ptr<std::vector<cell>> user_data = g_EasyHttpModule->GetRequest(request_id).user_data;

    if (!user_data)
        return 0;

    MF_CopyAmxMemory(data_addr, user_data->data(), user_data->size());

    return 0;
}


cell AMX_NATIVE_CALL ezhttp_ftp_upload(AMX* amx, cell* params)
{
    try {
        int arg_num = params[0] / sizeof(cell);
        int len;
        char* user_ptr = MF_GetAmxString(amx, params[1], 0, &len);
        std::string user(user_ptr ? user_ptr : "");
        char* pass_ptr = MF_GetAmxString(amx, params[2], 0, &len);
        std::string password (pass_ptr ? pass_ptr : "");
        char* host_ptr = MF_GetAmxString(amx, params[3], 0, &len);
        if (!host_ptr) throw std::runtime_error("Host pointer is null");
        std::string host(host_ptr);
        char* remote_ptr = MF_GetAmxString(amx, params[4], 0, &len);
        if (!remote_ptr) throw std::runtime_error("Remote file pointer is null");
        std::string remote_file(remote_ptr);
        
        int local_file_len;
        char* local_file = MF_GetAmxString(amx, params[5], 1, &local_file_len);
        if (!local_file) throw std::runtime_error("Local file pointer is null");

        char* callback_ptr = nullptr;
        if (arg_num >= 6) {
            callback_ptr = MF_GetAmxString(amx, params[6], 0, &len);
        }
        std::string callback(callback_ptr ? callback_ptr : "");

        bool secure = false;
        if (arg_num >= 7) {
            secure = (bool)params[7];
        }

        auto options_id = OptionsId::Null;
        if (arg_num >= 8) {
            options_id = (OptionsId)params[8];
        }

        std::string url = utils::ConstructFtpUrl(user, password, host, remote_file);

        if (options_id == OptionsId::Null)
            options_id = g_EasyHttpModule->CreateOptions();
        else if (!ValidateOptionsId(amx, options_id))
            return 0;

        std::string resolved_path;
        if (!ValidatePath("ezhttp_ftp_upload", local_file, resolved_path))
            return 0;

        auto& builder = g_EasyHttpModule->GetOptionsBuilder(options_id);
        builder.SetFilePath(resolved_path);
        builder.SetSecure(secure);

        SendRequest(amx, RequestMethod::FtpUpload, options_id, url, callback);

        return 0;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_ftp_upload: %s", e.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL ezhttp_ftp_upload2(AMX* amx, cell* params)
{
    try {
        int arg_num = params[0] / sizeof(cell);
        int url_str_len;
        char* url_str = MF_GetAmxString(amx, params[1], 0, &url_str_len);

        int local_file_len;
        char* local_file = MF_GetAmxString(amx, params[2], 1, &local_file_len);

        int callback_len = 0;
        char* callback = nullptr;
        if (arg_num >= 3) {
            callback = MF_GetAmxString(amx, params[3], 2, &callback_len);
        }

        bool secure = false;
        if (arg_num >= 4) {
            secure = (bool)params[4];
        }

        auto options_id = OptionsId::Null;
        if (arg_num >= 5) {
            options_id = (OptionsId)params[5];
        }

        if (options_id == OptionsId::Null)
            options_id = g_EasyHttpModule->CreateOptions();
        else if (!ValidateOptionsId(amx, options_id))
            return 0;

        std::string resolved_path;
        if (!ValidatePath("ezhttp_ftp_upload2", local_file, resolved_path))
            return 0;

        auto& builder = g_EasyHttpModule->GetOptionsBuilder(options_id);
        builder.SetFilePath(resolved_path);
        builder.SetSecure(secure);

        SendRequest(amx, RequestMethod::FtpUpload, options_id, std::string(url_str, url_str_len), (callback) ? std::string(callback, callback_len) : "");

        return 0;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_ftp_upload2: %s", e.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL ezhttp_ftp_download(AMX* amx, cell* params)
{
    try {
        int arg_num = params[0] / sizeof(cell);
        int len;
        char* user_ptr = MF_GetAmxString(amx, params[1], 0, &len);
        std::string user(user_ptr ? user_ptr : "");
        char* pass_ptr = MF_GetAmxString(amx, params[2], 0, &len);
        std::string password (pass_ptr ? pass_ptr : "");
        char* host_ptr = MF_GetAmxString(amx, params[3], 0, &len);
        if (!host_ptr) throw std::runtime_error("Host pointer is null");
        std::string host(host_ptr);
        char* remote_ptr = MF_GetAmxString(amx, params[4], 0, &len);
        if (!remote_ptr) throw std::runtime_error("Remote file pointer is null");
        std::string remote_file(remote_ptr);

        int local_file_len;
        char* local_file = MF_GetAmxString(amx, params[5], 1, &local_file_len);
        if (!local_file) throw std::runtime_error("Local file pointer is null");

        char* callback_ptr = nullptr;
        if (arg_num >= 6) {
            callback_ptr = MF_GetAmxString(amx, params[6], 0, &len);
        }
        std::string callback(callback_ptr ? callback_ptr : "");

        bool secure = false;
        if (arg_num >= 7) {
            secure = (bool)params[7];
        }

        auto options_id = OptionsId::Null;
        if (arg_num >= 8) {
            options_id = (OptionsId)params[8];
        }

        std::string url = utils::ConstructFtpUrl(user, password, host, remote_file);

        if (options_id == OptionsId::Null)
            options_id = g_EasyHttpModule->CreateOptions();
        else if (!ValidateOptionsId(amx, options_id))
            return 0;

        std::string resolved_path;
        if (!ValidatePath("ezhttp_ftp_download", local_file, resolved_path))
            return 0;

        auto& builder = g_EasyHttpModule->GetOptionsBuilder(options_id);
        builder.SetFilePath(resolved_path);
        builder.SetSecure(secure);

        SendRequest(amx, RequestMethod::FtpDownload, options_id, url, callback);

        return 0;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_ftp_download: %s", e.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL ezhttp_ftp_download2(AMX* amx, cell* params)
{
    try {
        int arg_num = params[0] / sizeof(cell);
        int url_str_len;
        char* url_str = MF_GetAmxString(amx, params[1], 0, &url_str_len);

        int local_file_len;
        char* local_file = MF_GetAmxString(amx, params[2], 1, &local_file_len);

        int callback_len = 0;
        char* callback = nullptr;
        if (arg_num >= 3) {
            callback = MF_GetAmxString(amx, params[3], 2, &callback_len);
        }

        bool secure = false;
        if (arg_num >= 4) {
            secure = (bool)params[4];
        }

        auto options_id = OptionsId::Null;
        if (arg_num >= 5) {
            options_id = (OptionsId)params[5];
        }

        if (options_id == OptionsId::Null)
            options_id = g_EasyHttpModule->CreateOptions();
        else if (!ValidateOptionsId(amx, options_id))
            return 0;

        std::string resolved_path;
        if (!ValidatePath("ezhttp_ftp_download2", local_file, resolved_path))
            return 0;

        auto& builder = g_EasyHttpModule->GetOptionsBuilder(options_id);
        builder.SetFilePath(resolved_path);
        builder.SetSecure(secure);

        SendRequest(amx, RequestMethod::FtpDownload, options_id, std::string(url_str, url_str_len), (callback) ? std::string(callback, callback_len) : "");

        return 0;
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "ezhttp_ftp_download2: %s", e.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL ezhttp_create_queue(AMX* amx, cell* params)
{
    return (cell)g_EasyHttpModule->CreateQueue();
}

cell AMX_NATIVE_CALL ezhttp_get_active_requests_count(AMX* amx, cell* params)
{
    return (cell)g_EasyHttpModule->GetActiveRequestCount();
}

cell AMX_NATIVE_CALL ezhttp_steam_to_steam64(AMX* amx, cell* params)
{
    // doc https://developer.valvesoftware.com/wiki/SteamID

    int steam_len;
    char* steam_str = MF_GetAmxString(amx, params[1], 0, &steam_len);
    cell steam64_maxlen = params[3];

    std::string steam(steam_str, steam_len);

    if (steam.find("STEAM_") != 0)
        return 0;

    std::vector<std::string> tokens;
    utils::split(std::string_view(steam.c_str() + sizeof("STEAM_") - 1), ":", tokens);

    if (tokens.size() != 3)
        return 0;

    // Validate Y bit (universe bit) must be 0 or 1
    if (tokens[1] != "0" && tokens[1] != "1")
        return 0;

    char* endptr = nullptr;
    errno = 0;
    long y_bit = std::strtol(tokens[1].c_str(), &endptr, 10);
    if (errno != 0 || *endptr != '\0')
        return 0;

    errno = 0;
    long account_id_value = std::strtol(tokens[2].c_str(), &endptr, 10);
    if (errno != 0 || *endptr != '\0' || account_id_value < 0)
        return 0;

    uint32_t account_id = static_cast<uint32_t>(account_id_value); // 32 bit
    account_id = account_id << 1 | static_cast<uint32_t>(y_bit);
    uint32_t account_instance = 1;                         // 20 bit, 1 for individual account
    uint8_t account_type = 1;                              // 4 bit,  1 is individual account
    uint8_t universe = 1;                                  // 8 bit, using token[0] produces incorrect results, so use always 1

    uint64_t steam64 = (uint64_t)universe << 56 | (uint64_t)account_type << 52 | (uint64_t)account_instance << 32 | account_id;

    steam.assign(std::to_string(steam64));
    utils::SetAmxStringUTF8CharSafe(amx, params[2], steam.c_str(), steam.length(), steam64_maxlen);

    return 1;
}

RequestId SendRequest(AMX* amx, RequestMethod method, OptionsId options_id, const std::string& url, const std::string& callback, std::shared_ptr<std::vector<cell>> user_data)
{
    int callback_id = -1;
    if (!callback.empty())
    {
        if (!user_data)
        {
            callback_id = MF_RegisterSPForwardByName(amx, callback.c_str(), FP_CELL, FP_DONE);
        } else {
            callback_id = MF_RegisterSPForwardByName(amx, callback.c_str(), FP_CELL, FP_ARRAY, FP_DONE);
        }

        if (callback_id == -1)
        {
            MF_LogError(amx, AMX_ERR_NATIVE, "Callback function \"%s\" is not exists", callback.c_str());
            return RequestId::Null;
        }
    }

    auto on_complete = [callback_id, user_data](RequestId request_id) {
        if (callback_id == -1)
        {
            g_EasyHttpModule->DeleteRequest(request_id, true);
            return;
        }

        try {
            if (!user_data)
            {
                MF_ExecuteForward(callback_id, request_id);
            } else {
                MF_ExecuteForward(callback_id, request_id, MF_PrepareCellArray(user_data->data(), user_data->size()));
            }
        } catch (const std::exception& e) {
            MF_LogError(nullptr, AMX_ERR_NATIVE, "ezhttp: Exception in callback execution: %s", e.what());
        } catch (...) {
            MF_LogError(nullptr, AMX_ERR_NATIVE, "ezhttp: Unknown exception in callback execution");
        }

        MF_UnregisterSPForward(callback_id);
        g_EasyHttpModule->DeleteRequest(request_id, true);
    };

    if (!user_data)
    {
        if (options_id == OptionsId::Null)
            options_id = g_EasyHttpModule->CreateOptions();

        user_data = g_EasyHttpModule->GetOptions(options_id).user_data;
    }

    RequestId request_id = g_EasyHttpModule->SendRequest(method, url, options_id, on_complete, user_data);

    return request_id;
}


bool ValidateOptionsId(AMX* amx, OptionsId options_id)
{
    if (!g_EasyHttpModule->IsOptionsExists(options_id))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Options id %d not exists", options_id);
        return false;
    }

    return true;
}

bool ValidateRequestId(AMX* amx, RequestId request_id)
{
    if (!g_EasyHttpModule->IsRequestExists(request_id))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Request id %d not exists", request_id);
        return false;
    }

    return true;
}

bool ValidateQueueId(AMX* amx, QueueId queue_id)
{
    if (!g_EasyHttpModule->IsQueueExists(queue_id))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Queue id %d not exists", queue_id);
        return false;
    }

    return true;
}

template <class TMethod>
void SetKeyValueOption(AMX* amx, cell* params, TMethod method)
{
    try {
        auto options_id = (OptionsId)params[1];
        int key_len;
        char* key = MF_GetAmxString(amx, params[2], 0, &key_len);
        int value_len;
        char* value = MF_GetAmxString(amx, params[3], 1, &value_len);

        if (!ValidateOptionsId(amx, options_id))
            return;

        if (!key || !value) {
            MF_LogError(amx, AMX_ERR_NATIVE, "Key or value pointer is null");
            return;
        }

        (g_EasyHttpModule->GetOptions(options_id).options_builder.*method)(std::string(key, key_len), std::string(value, value_len));
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Exception in SetKeyValueOption: %s", e.what());
    }
}

template <class TMethod>
void SetStringOption(AMX* amx, cell* params, TMethod method)
{
    try {
        auto options_id = (OptionsId)params[1];
        int value_len;
        char* value = MF_GetAmxString(amx, params[2], 0, &value_len);

        if (!ValidateOptionsId(amx, options_id))
            return;

        if (!value) {
            MF_LogError(amx, AMX_ERR_NATIVE, "Value pointer is null");
            return;
        }

        (g_EasyHttpModule->GetOptions(options_id).options_builder.*method)(std::string(value, value_len));
    } catch (const std::exception& e) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Exception in SetStringOption: %s", e.what());
    }
}

AMX_NATIVE_INFO g_Natives[] =
{
    // options
    { "ezhttp_create_options",              ezhttp_create_options },
    { "ezhttp_destroy_options",             ezhttp_destroy_options },
    { "ezhttp_option_set_user_agent",       ezhttp_option_set_user_agent },
    { "ezhttp_option_add_url_parameter",    ezhttp_option_add_url_parameter },
    { "ezhttp_option_add_form_payload",     ezhttp_option_add_form_payload },
    { "ezhttp_option_set_body",             ezhttp_option_set_body },
    { "ezhttp_option_set_body_from_json",   ezhttp_option_set_body_from_json },
    { "ezhttp_option_append_body",          ezhttp_option_append_body },
    { "ezhttp_option_set_header",           ezhttp_option_set_header },
    { "ezhttp_option_set_cookie",           ezhttp_option_set_cookie },
    { "ezhttp_option_set_timeout",          ezhttp_option_set_timeout },
    { "ezhttp_option_set_connect_timeout",  ezhttp_option_set_connect_timeout },
    { "ezhttp_option_set_proxy",            ezhttp_option_set_proxy },
    { "ezhttp_option_set_proxy_auth",       ezhttp_option_set_proxy_auth },
    { "ezhttp_option_set_auth",             ezhttp_option_set_auth },
    { "ezhttp_option_set_user_data",        ezhttp_option_set_user_data },
    { "ezhttp_option_set_plugin_end_behaviour", ezhttp_option_set_plugin_end_behaviour },
    { "ezhttp_option_set_queue",            ezhttp_option_set_queue },

    // requests
    { "ezhttp_get",                         ezhttp_get },
    { "ezhttp_post",                        ezhttp_post },
    { "ezhttp_put",                         ezhttp_put },
    { "ezhttp_patch",                       ezhttp_patch },
    { "ezhttp_delete",                      ezhttp_delete },
    { "ezhttp_is_request_exists",           ezhttp_is_request_exists },
    { "ezhttp_cancel_request",              ezhttp_cancel_request },
    { "ezhttp_request_progress",            ezhttp_request_progress },

    // response
    { "ezhttp_get_http_code",               ezhttp_get_http_code },
    { "ezhttp_get_data",                    ezhttp_get_data },
    { "ezhttp_parse_json_response",         ezhttp_parse_json_response },
    { "ezhttp_get_url",                     ezhttp_get_url },
    { "ezhttp_download_url",                ezhttp_download_url },
    { "ezhttp_save_data_to_file",           ezhttp_save_data_to_file },
    { "ezhttp_save_data_to_file2",          ezhttp_save_data_to_file2 },
    { "ezhttp_get_headers_count",           ezhttp_get_headers_count },
    { "ezhttp_get_headers",                 ezhttp_get_headers },
    { "ezhttp_iterate_headers",             ezhttp_iterate_headers },
    { "ezhttp_get_elapsed",                 ezhttp_get_elapsed },
    { "ezhttp_get_cookies_count",           ezhttp_get_cookies_count },
    { "ezhttp_get_cookies",                 ezhttp_get_cookies },
    { "ezhttp_iterate_cookies",             ezhttp_iterate_cookies },
    { "ezhttp_get_error_code",              ezhttp_get_error_code },
    { "ezhttp_get_error_message",           ezhttp_get_error_message },
    { "ezhttp_get_redirect_count",          ezhttp_get_redirect_count },
    { "ezhttp_get_uploaded_bytes",          ezhttp_get_uploaded_bytes },
    { "ezhttp_get_downloaded_bytes",        ezhttp_get_downloaded_bytes },
    { "ezhttp_get_user_data",               ezhttp_get_user_data },

    // ftp
    { "ezhttp_ftp_upload",                  ezhttp_ftp_upload },
    { "ezhttp_ftp_upload2",                 ezhttp_ftp_upload2 },
    { "ezhttp_ftp_download",                ezhttp_ftp_download },
    { "ezhttp_ftp_download2",               ezhttp_ftp_download2 },

    // queue
    { "ezhttp_create_queue",                ezhttp_create_queue },

    // info
    { "ezhttp_get_active_requests_count",   ezhttp_get_active_requests_count },

    // special
    { "ezhttp_steam_to_steam64",            ezhttp_steam_to_steam64 },
    { nullptr,                              nullptr },
};

cvar_t cvar_ezhttp_version = { "ezhttp_version", MODULE_VERSION, FCVAR_SERVER | FCVAR_SPONLY };

void HandleAutoUpdate()
{
    std::error_code ec;

#ifdef _WIN32
    std::filesystem::path modulePath = std::filesystem::weakly_canonical(MF_BuildPathname("modules/easy_http_amxx.dll"), ec);
#else
    std::filesystem::path modulePath = std::filesystem::weakly_canonical(MF_BuildPathname("modules/easy_http_amxx_i386.so"), ec);
#endif

    if (ec) {
        MF_PrintSrvConsole("[AmxxEasyHttp] AutoUpdater: Failed to resolve module path. Error: %s\n", ec.message().c_str());
        return;
    }

    std::filesystem::path newPath = modulePath.string() + ".new";
    std::filesystem::path oldPath = modulePath.string() + ".old";

    bool swapOccurred = false;

    if (std::filesystem::exists(newPath, ec)) {
        
        // --- 1. DRY RUN NATIVE LOAD TEST ---
        // Dynamically load the .new file to verify it is absolutely healthy before overwriting anything.
#ifdef _WIN32
        HMODULE testLoad = LoadLibraryA(newPath.string().c_str());
        if (!testLoad) {
            MF_PrintSrvConsole("[AmxxEasyHttp] AutoUpdater: CRITICAL - Downloaded .new DLL is CORRUPT or missing dependencies! Aborting swap.\n");
            std::filesystem::remove(newPath, ec);
            return;
        }
        FreeLibrary(testLoad);
#else
        void* testLoad = dlopen(newPath.string().c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!testLoad) {
            MF_PrintSrvConsole("[AmxxEasyHttp] AutoUpdater: CRITICAL - Downloaded .new SO is CORRUPT or missing dependencies! %s\n", dlerror());
            std::filesystem::remove(newPath, ec);
            return;
        }
        dlclose(testLoad);
#endif

        // --- 2. THE SWAP ---
        if (std::filesystem::exists(oldPath, ec)) {
            std::filesystem::remove(oldPath, ec);
        }

        std::filesystem::rename(modulePath, oldPath, ec);
        if (!ec) {
            std::filesystem::rename(newPath, modulePath, ec);
            if (!ec) {
                swapOccurred = true;
                MF_PrintSrvConsole("[AmxxEasyHttp] AutoUpdater: UPDATE SUCCESSFUL! Module swapped. Triggering reboot safely...\n");
            } else {
                std::error_code recover_ec;
                std::filesystem::rename(oldPath, modulePath, recover_ec);
                MF_PrintSrvConsole("[AmxxEasyHttp] AutoUpdater: Error renaming .new to .dll/.so. Recovery attempted.\n");
            }
        } else {
            MF_PrintSrvConsole("[AmxxEasyHttp] AutoUpdater: Error renaming current module to .old: %s\n", ec.message().c_str());
        }
    } else {
        if (std::filesystem::exists(oldPath, ec)) {
            std::filesystem::remove(oldPath, ec);
            MF_PrintSrvConsole("[AmxxEasyHttp] AutoUpdater: Old module version pruned successfully.\n");
        }
    }

    if (swapOccurred) {
        g_engfuncs.pfnServerCommand((char*)"quit\n");
    }
}

void OnAmxxAttach()
{
    try {
        HandleAutoUpdate();
        
        MF_AddNatives(g_Natives);
        MF_AddNatives(g_JsonNatives);

        CreateModules();

        CVAR_REGISTER(&cvar_ezhttp_version);
    } catch (const std::exception& e) {
        MF_LogError(nullptr, AMX_ERR_NATIVE, "ezhttp: Fatal exception during attach: %s", e.what());
    }
}

void OnAmxxDetach()
{
    DestroyModules();
}

void StartFrame()
{
    try {
        if (g_EasyHttpModule)
            g_EasyHttpModule->RunFrame();
    } catch (const std::exception& e) {
        static bool error_logged = false;
        if (!error_logged) {
            MF_LogError(nullptr, AMX_ERR_NATIVE, "ezhttp: Exception during StartFrame: %s", e.what());
            error_logged = true;
        }
    } catch (...) {
        static bool error_logged = false;
        if (!error_logged) {
            MF_LogError(nullptr, AMX_ERR_NATIVE, "ezhttp: Unknown exception during StartFrame");
            error_logged = true;
        }
    }

    SET_META_RESULT(MRES_IGNORED);
}

void ServerDeactivate()
{
    if (g_EasyHttpModule) {
        // Drain callbacks from already-completed requests so they don't
        // fire into unloaded AMX plugins during map change
        for (int i = 0; i < 20; i++) {
            try {
                g_EasyHttpModule->RunFrame();
            } catch (...) {
                break;
            }
        }

        g_EasyHttpModule->ServerDeactivate();
    }

    if (g_JsonManager)
        g_JsonManager->FreeAllHandles();

    SET_META_RESULT(MRES_IGNORED);
}

void GameShutdown()
{
    DestroyModules();
}
