// SPDX-License-Identifier: GPL-2.0-or-later
#include "HttpGetter.h"
#include <WiFiClientSecure.h>
#include "mbedtls/sha256.h"
#include <base64.h>
#include <ESPmDNS.h>

template<typename... Args>
void HttpGetter::logError(char const* format, Args... args) {
    snprintf(_errBuffer, sizeof(_errBuffer), format, args...);
}

bool HttpGetter::init()
{
    String url(_config.Url);

    int index = url.indexOf(':');
    if (index < 0) {
        logError("failed to parse URL protocol: no colon in URL");
        return false;
    }

    String protocol = url.substring(0, index);
    if (protocol != "http" && protocol != "https") {
        logError("failed to parse URL protocol: '%s' is neither 'http' nor 'https'", protocol.c_str());
        return false;
    }

    _useHttps = (protocol == "https");

    // initialize port to default values for http or https.
    // port will be overwritten below in case port is explicitly defined
    _port = _useHttps ? 443 : 80;

    String slashes = url.substring(index + 1, index + 3);
    if (slashes != "//") {
        logError("expected two forward slashes after first colon in URL");
        return false;
    }

    _uri = url.substring(index + 3); // without protocol identifier

    index = _uri.indexOf('/');
    if (index == -1) {
        index = _uri.length();
        _uri += '/';
    }
    _host = _uri.substring(0, index);
    _uri.remove(0, index); // remove host part

    index = _host.indexOf('@');
    if (index >= 0) {
        // basic authentication is only supported through setting username
        // and password using the respective inputs, not embedded into the URL.
        // to avoid regressions, we remove username and password from the host
        // part of the URL.
        _host.remove(0, index + 1); // remove auth part including @
    }

    // get port
    index = _host.indexOf(':');
    if (index >= 0) {
        _host = _host.substring(0, index); // up until colon
        _port = _host.substring(index + 1).toInt(); // after colon
    }

    if (_useHttps) {
        auto secureWifiClient = std::make_shared<WiFiClientSecure>();
        secureWifiClient->setInsecure();
        _spWiFiClient = std::move(secureWifiClient);
    } else {
        _spWiFiClient = std::make_shared<WiFiClient>();
    }

    return true;
}

HttpRequestResult HttpGetter::performGetRequest()
{
    // hostByName in WiFiGeneric fails to resolve local names. issue described at
    // https://github.com/espressif/arduino-esp32/issues/3822 and in analyzed in
    // depth at https://github.com/espressif/esp-idf/issues/2507#issuecomment-761836300
    // in conclusion: we cannot rely on _upHttpClient->begin(*wifiClient, url) to resolve
    // IP adresses. have to do it manually.
    IPAddress ipaddr((uint32_t)0);

    if (!ipaddr.fromString(_host)) {
        // host is not an IP address, so try to resolve the name to an address.
        // first try locally via mDNS, then via DNS. WiFiGeneric::hostByName()
        // will spam the console if done the other way around.
        ipaddr = INADDR_NONE;

        if (Configuration.get().Mdns.Enabled) {
            ipaddr = MDNS.queryHost(_host); // INADDR_NONE if failed
        }

        if (ipaddr == INADDR_NONE && !WiFiGenericClass::hostByName(_host.c_str(), ipaddr)) {
            logError("failed to resolve host '%s' via DNS", _host.c_str());
            return { false };
        }
    }

    auto upTmpHttpClient = std::make_unique<HTTPClient>();

    // use HTTP1.0 to avoid problems with chunked transfer encoding when the
    // stream is later used to read the server's response.
    upTmpHttpClient->useHTTP10(true);

    if (!upTmpHttpClient->begin(*_spWiFiClient, ipaddr.toString(), _port, _uri, _useHttps)) {
        logError("HTTP client begin() failed for %s://%s",
                (_useHttps ? "https" : "http"), _host.c_str());
        return { false };
    }

    upTmpHttpClient->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    upTmpHttpClient->setUserAgent("OpenDTU-OnBattery");
    upTmpHttpClient->setConnectTimeout(_config.Timeout);
    upTmpHttpClient->setTimeout(_config.Timeout);
    for (auto const& h : _additionalHeaders) {
        upTmpHttpClient->addHeader(h.first.c_str(), h.second.c_str());
    }

    if (strlen(_config.HeaderKey) > 0) {
        upTmpHttpClient->addHeader(_config.HeaderKey, _config.HeaderValue);
    }

    using Auth_t = HttpRequestConfig::Auth;
    switch (_config.AuthType) {
        case Auth_t::None:
            break;
        case Auth_t::Basic: {
            String credentials = String(_config.Username) + ":" + _config.Password;
            String authorization = "Basic " + base64::encode(credentials);
            upTmpHttpClient->addHeader("Authorization", authorization);
            break;
        }
        case Auth_t::Digest: {
            const char *headers[1] = {"WWW-Authenticate"};
            upTmpHttpClient->collectHeaders(headers, 1);
            break;
        }
    }

    int httpCode = upTmpHttpClient->GET();

    if (httpCode == HTTP_CODE_UNAUTHORIZED && _config.AuthType == Auth_t::Digest) {
        if (!upTmpHttpClient->hasHeader("WWW-Authenticate")) {
            logError("Cannot perform digest authentication as server did "
                        "not send a WWW-Authenticate header");
            return { false };
        }
        String authReq = upTmpHttpClient->header("WWW-Authenticate");
        String authorization = getAuthDigest(authReq, 1);
        upTmpHttpClient->addHeader("Authorization", authorization);
        httpCode = upTmpHttpClient->GET();
    }

    if (httpCode <= 0) {
        logError("HTTP Error: %s", upTmpHttpClient->errorToString(httpCode).c_str());
        return { false };
    }

    if (httpCode != HTTP_CODE_OK) {
        logError("Bad HTTP code: %d", httpCode);
        return { false };
    }

    return { true, std::move(upTmpHttpClient), _spWiFiClient };
}

static String sha256(const String& data) {
    uint8_t hash[32];

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // select SHA256
    mbedtls_sha256_update(&ctx, reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    char res[sizeof(hash) * 2 + 1];
    for (int i = 0; i < sizeof(hash); i++) {
        snprintf(res + (i*2), sizeof(res) - (i*2), "%02x", hash[i]);
    }

    return res;
}

static String extractParam(String const& authReq, String const& param, char delimiter) {
    auto begin = authReq.indexOf(param);
    if (begin == -1) { return ""; }
    auto end = authReq.indexOf(delimiter, begin + param.length());
    return authReq.substring(begin + param.length(), end);
}

static String getcNonce(int len) {
    static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";
    String s = "";

    for (int i = 0; i < len; ++i) { s += alphanum[rand() % (sizeof(alphanum) - 1)]; }

    return s;
}

String HttpGetter::getAuthDigest(String const& authReq, unsigned int counter) {
    // extracting required parameters for RFC 2617 Digest
    String realm = extractParam(authReq, "realm=\"", '"');
    String nonce = extractParam(authReq, "nonce=\"", '"');
    String cNonce = getcNonce(8);

    char nc[9];
    snprintf(nc, sizeof(nc), "%08x", counter);

    // sha256 of the user:realm:password
    String ha1 = sha256(String(_config.Username) + ":" + realm + ":" + _config.Password);

    // sha256 of method:uri
    String ha2 = sha256("GET:" + _uri);

    // sha256 of h1:nonce:nc:cNonce:auth:h2
    String response = sha256(ha1 + ":" + nonce + ":" + String(nc) +
            ":" + cNonce + ":" + "auth" + ":" + ha2);

    // Final authorization String
    return String("Digest username=\"") + _config.Username +
        "\", realm=\"" + realm + "\", nonce=\"" + nonce + "\", uri=\"" +
        _uri + "\", cnonce=\"" + cNonce + "\", nc=" + nc +
        ", qop=auth, response=\"" + response  + "\", algorithm=SHA-256";
}

void HttpGetter::addHeader(char const* key, char const* value)
{
    _additionalHeaders.push_back({ key, value });
}
