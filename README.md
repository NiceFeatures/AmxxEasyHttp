AmxxEasyHttp v2.0.0 by Polarhigh & iceeedR
=======
AmxxEasyHttp is a high-performance module for AMX Mod X that provides an easy-to-use asynchronous API for HTTP, HTTPS, and FTP requests.

Example
-------
Here is a short example which shows some features of AmxxEasyHttp:

```c
#include <amxmodx>
#include <easy_http>

public http_get()
{
    // Simple GET request
    ezhttp_get("https://httpbin.org/get", "http_complete")
}

public http_get_with_data()
{
    new dat[1];
    dat[0] = 1337;

    // GET with binary user data (Up to 10MB)
    ezhttp_get(
        .url = "https://httpbin.org/get", 
        .on_complete = "http_complete_with_data", 
        .data = dat, 
        .data_len = sizeof(dat)
    );

    // OR using options (prefer this for complex requests)
    new EzHttpOptions:options = ezhttp_create_options()
    ezhttp_option_set_user_data(options, dat, sizeof(dat))

    ezhttp_get(
        .url = "https://httpbin.org/get", 
        .on_complete = "http_complete_with_data", 
        .options_id = options
    );
    
    // Always destroy options if not used in a request or if you want to reuse handles
    // ezhttp_destroy_options(options) 
}

public http_complete_with_data(EzHttpRequest:request_id, const data[])
{
    server_print("data[0]: %d", data[0]);
}

public http_post()
{
    new EzHttpOptions:options_id = ezhttp_create_options()
    ezhttp_option_set_header(options_id, "Content-Type", "text/plain")
    ezhttp_option_set_body(options_id, "Hello!")

    ezhttp_post("https://httpbin.org/post", "http_complete", options_id)
}

public http_complete(EzHttpRequest:request_id)
{
    if (ezhttp_get_error_code(request_id) != EZH_OK)
    {
        new error[64]
        ezhttp_get_error_message(request_id, error, charsmax(error))
        server_print("Response error: %s", error);
        return
    }

    new data[512]
    ezhttp_get_data(request_id, data, charsmax(data))
    server_print("Response data: %s", data)

    // Save response to file (Path-safe validation included)
    ezhttp_save_data_to_file(request_id, "addons/amxmodx/response.json")
}
```

## Features

* **Consolidated API**: Single include `easy_http.inc` for both HTTP and JSON.
* **Non-blocking/Asynchronous**: Requests run in separate threads (up to 6 concurrent).
* **Security & Stability**:
    * **HLDS Protection**: Global exception handling prevents server crashes.
    * **Path Validation**: Prevents path traversal vulnerabilities.
    * **Memory Safety**: Automatic caps on user data size (10MB).
* **Full Protocol Support**: HTTP, HTTPS, FTP, and FTPES.
* **Modern Auth**: Basic authentication, Proxy support, and SSL (OpenSSL/WinSSL).
* **JSON support**: Built-in Parson integration for easy JSON manipulation.

## Security Scaling
Starting from **v2.0.0**, the module implements "Zero Trust" parameter validation. Every native call verifies argument counts and pointer validity before processing, ensuring that even buggy plugins cannot crash the host process.

## Advanced features

### Behavior at the end of the map
By default, when you change map, all requests are interrupted. The callback functions are called with error code EZH_REQUEST_CANCELLED.
An alternative is to specify the option ```ezhttp_option_set_plugin_end_behaviour(options_id, EZH_FORGET_REQUEST)```, in which case the request will not be interrupted at the end of the map (but the callback will not be called).

### Request queue
If you need to execute requests sequentially, you can create a queue with ```new EzHttpQueue:queue_id = ezhttp_create_queue()``` and then set the ```ezhttp_option_set_queue(options_id, queue_id)``` option.

## Building

Building AmxxEasyHttp requires CMake 3.20+ and a C++17 compiler.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target easy_http
```

### Building with Docker
```bash
docker build -t amxx-ezhttp-builder .
docker run --rm -v .:/ezhttp amxx-ezhttp-builder
```
