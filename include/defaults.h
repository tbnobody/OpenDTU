// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#define SERIAL_BAUDRATE 115200

#define APP_HOSTNAME "OpenDTU-%06X"

#define HTTP_PORT 80

#define ACCESS_POINT_NAME "OpenDTU-"
#define ACCESS_POINT_PASSWORD "openDTU42"
#define ACCESS_POINT_TIMEOUT 3;
#define AUTH_USERNAME "admin"
#define SECURITY_ALLOW_READONLY true

#define WIFI_RECONNECT_TIMEOUT 15
#define WIFI_RECONNECT_REDO_TIMEOUT 600

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define WIFI_DHCP true

#define MDNS_ENABLED false

#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_TIMEZONEDESCR "Europe/Berlin"
#define NTP_LONGITUDE 10.4515f
#define NTP_LATITUDE 51.1657f
#define NTP_SUNSETTYPE 1U

#define MQTT_ENABLED false
#define MQTT_HOST ""
#define MQTT_PORT 1883U
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_TOPIC "solar/"
#define MQTT_RETAIN true
#define MQTT_TLS false
// ISRG_Root_X1.crt -- Root CA for Letsencrypt
#define MQTT_ROOT_CA_CERT "-----BEGIN CERTIFICATE-----\n"                                      \
                          "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
                          "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
                          "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
                          "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
                          "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
                          "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
                          "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n" \
                          "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n" \
                          "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n" \
                          "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n" \
                          "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n" \
                          "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n" \
                          "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n" \
                          "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n" \
                          "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n" \
                          "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n" \
                          "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n" \
                          "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n" \
                          "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n" \
                          "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n" \
                          "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n" \
                          "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n" \
                          "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n" \
                          "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n" \
                          "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n" \
                          "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n" \
                          "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n" \
                          "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n" \
                          "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
                          "-----END CERTIFICATE-----\n"
#define MQTT_TLSCERTLOGIN false
#define MQTT_TLSCLIENTCERT ""
#define MQTT_TLSCLIENTKEY ""
#define MQTT_LWT_TOPIC "dtu/status"
#define MQTT_LWT_ONLINE "online"
#define MQTT_LWT_OFFLINE "offline"
#define MQTT_PUBLISH_INTERVAL 5U
#define MQTT_CLEAN_SESSION true

#define DTU_SERIAL 0x99978563412U
#define DTU_POLL_INTERVAL 5U
#define DTU_NRF_PA_LEVEL 0U
#define DTU_CMT_PA_LEVEL 0
#define DTU_CMT_FREQUENCY 865000U

#define MQTT_HASS_ENABLED false
#define MQTT_HASS_EXPIRE true
#define MQTT_HASS_RETAIN true
#define MQTT_HASS_TOPIC "homeassistant/"
#define MQTT_HASS_INDIVIDUALPANELS false

#define DEV_PINMAPPING ""

#define DISPLAY_POWERSAFE true
#define DISPLAY_SCREENSAVER true
#define DISPLAY_ROTATION 2U
#define DISPLAY_CONTRAST 60U
#define DISPLAY_LANGUAGE 0U

#define REACHABLE_THRESHOLD 2U
