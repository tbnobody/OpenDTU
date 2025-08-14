// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Thomas Basler and others
 */
#include "MqttSubscribeParser.h"

void MqttSubscribeParser::register_callback(const std::string& topic, uint8_t qos, const OnMessageCallback& cb)
{
    cb_filter_t cbf;
    cbf.topic = topic;
    cbf.qos = qos;
    cbf.cb = cb;
    _callbacks.push_back(cbf);
}

void MqttSubscribeParser::unregister_callback(const std::string& topic)
{
    for (auto it = _callbacks.begin(); it != _callbacks.end();) {
        if ((*it).topic == topic) {
            it = _callbacks.erase(it);
        } else {
            ++it;
        }
    }
}

void MqttSubscribeParser::handle_message(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len)
{
    bool result = false;
    for (const auto& cb : _callbacks) {
        if (mosquitto_topic_matches_sub(cb.topic.c_str(), topic, &result) == MOSQ_ERR_SUCCESS) {
            if (result) {
                cb.cb(properties, topic, payload, len);
            }
        }
    }
}

std::vector<cb_filter_t> MqttSubscribeParser::get_callbacks()
{
    return _callbacks;
}

/* Does a topic match a subscription? */
int MqttSubscribeParser::mosquitto_topic_matches_sub(const char* sub, const char* topic, bool* result)
{
    size_t spos;

    if (!result)
        return MOSQ_ERR_INVAL;
    *result = false;

    if (!sub || !topic || sub[0] == 0 || topic[0] == 0) {
        return MOSQ_ERR_INVAL;
    }

    if ((sub[0] == '$' && topic[0] != '$')
        || (topic[0] == '$' && sub[0] != '$')) {

        return MOSQ_ERR_SUCCESS;
    }

    spos = 0;

    while (sub[0] != 0) {
        if (topic[0] == '+' || topic[0] == '#') {
            return MOSQ_ERR_INVAL;
        }
        if (sub[0] != topic[0] || topic[0] == 0) { /* Check for wildcard matches */
            if (sub[0] == '+') {
                /* Check for bad "+foo" or "a/+foo" subscription */
                if (spos > 0 && sub[-1] != '/') {
                    return MOSQ_ERR_INVAL;
                }
                /* Check for bad "foo+" or "foo+/a" subscription */
                if (sub[1] != 0 && sub[1] != '/') {
                    return MOSQ_ERR_INVAL;
                }
                spos++;
                sub++;
                while (topic[0] != 0 && topic[0] != '/') {
                    if (topic[0] == '+' || topic[0] == '#') {
                        return MOSQ_ERR_INVAL;
                    }
                    topic++;
                }
                if (topic[0] == 0 && sub[0] == 0) {
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }
            } else if (sub[0] == '#') {
                /* Check for bad "foo#" subscription */
                if (spos > 0 && sub[-1] != '/') {
                    return MOSQ_ERR_INVAL;
                }
                /* Check for # not the final character of the sub, e.g. "#foo" */
                if (sub[1] != 0) {
                    return MOSQ_ERR_INVAL;
                } else {
                    while (topic[0] != 0) {
                        if (topic[0] == '+' || topic[0] == '#') {
                            return MOSQ_ERR_INVAL;
                        }
                        topic++;
                    }
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }
            } else {
                /* Check for e.g. foo/bar matching foo/+/# */
                if (topic[0] == 0
                    && spos > 0
                    && sub[-1] == '+'
                    && sub[0] == '/'
                    && sub[1] == '#') {
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }

                /* There is no match at this point, but is the sub invalid? */
                while (sub[0] != 0) {
                    if (sub[0] == '#' && sub[1] != 0) {
                        return MOSQ_ERR_INVAL;
                    }
                    spos++;
                    sub++;
                }

                /* Valid input, but no match */
                return MOSQ_ERR_SUCCESS;
            }
        } else {
            /* sub[spos] == topic[tpos] */
            if (topic[1] == 0) {
                /* Check for e.g. foo matching foo/# */
                if (sub[1] == '/'
                    && sub[2] == '#'
                    && sub[3] == 0) {
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }
            }
            spos++;
            sub++;
            topic++;
            if (sub[0] == 0 && topic[0] == 0) {
                *result = true;
                return MOSQ_ERR_SUCCESS;
            } else if (topic[0] == 0 && sub[0] == '+' && sub[1] == 0) {
                if (spos > 0 && sub[-1] != '/') {
                    return MOSQ_ERR_INVAL;
                }
                spos++;
                sub++;
                *result = true;
                return MOSQ_ERR_SUCCESS;
            }
        }
    }
    if ((topic[0] != 0 || sub[0] != 0)) {
        *result = false;
    }
    while (topic[0] != 0) {
        if (topic[0] == '+' || topic[0] == '#') {
            return MOSQ_ERR_INVAL;
        }
        topic++;
    }

    return MOSQ_ERR_SUCCESS;
}
