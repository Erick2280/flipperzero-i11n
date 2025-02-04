#include "subghz_txrx_i.h"
#include "subghz_txrx_create_protocol_key.h"
#include <lib/subghz/transmitter.h>
#include <lib/subghz/protocols/protocol_items.h>
#include <lib/subghz/protocols/protocol_items.h>
#include <lib/subghz/protocols/keeloq.h>
#include <lib/subghz/protocols/secplus_v1.h>
#include <lib/subghz/protocols/secplus_v2.h>

#include <flipper_format/flipper_format_i.h>
#include <lib/toolbox/stream/stream.h>
#include <lib/subghz/protocols/raw.h>

#define TAG "SubGhzCreateProtocolKey"

bool subghz_txrx_gen_data_protocol(
    void* context,
    const char* preset_name,
    uint32_t frequency,
    const char* protocol_name,
    uint64_t key,
    uint32_t bit) {
    furi_assert(context);
    SubGhzTxRx* instance = context;

    bool res = false;

    subghz_txrx_set_preset(instance, preset_name, frequency, NULL, 0);
    instance->decoder_result =
        subghz_receiver_search_decoder_base_by_name(instance->receiver, protocol_name);

    if(instance->decoder_result == NULL) {
        //TODO FL-3502: Error
        // furi_string_set(error_str, "Protocol not\nfound!");
        // scene_manager_next_scene(scene_manager, SubGhzSceneShowErrorSub);
        FURI_LOG_E(TAG, "Protocol not found!");
        return false;
    }

    do {
        Stream* fff_data_stream = flipper_format_get_raw_stream(instance->fff_data);
        stream_clean(fff_data_stream);
        if(subghz_protocol_decoder_base_serialize(
               instance->decoder_result, instance->fff_data, instance->preset) !=
           SubGhzProtocolStatusOk) {
            FURI_LOG_E(TAG, "Unable to serialize");
            break;
        }
        if(!flipper_format_update_uint32(instance->fff_data, "Bit", &bit, 1)) {
            FURI_LOG_E(TAG, "Unable to update Bit");
            break;
        }

        uint8_t key_data[sizeof(uint64_t)] = {0};
        for(size_t i = 0; i < sizeof(uint64_t); i++) {
            key_data[sizeof(uint64_t) - i - 1] = (key >> (i * 8)) & 0xFF;
        }
        if(!flipper_format_update_hex(instance->fff_data, "Key", key_data, sizeof(uint64_t))) {
            FURI_LOG_E(TAG, "Unable to update Key");
            break;
        }
        res = true;
    } while(false);
    return res;
}

bool subghz_txrx_gen_data_protocol_and_te(
    SubGhzTxRx* instance,
    const char* preset_name,
    uint32_t frequency,
    const char* protocol_name,
    uint64_t key,
    uint32_t bit,
    uint32_t te) {
    furi_assert(instance);
    bool ret = false;
    if(subghz_txrx_gen_data_protocol(instance, preset_name, frequency, protocol_name, key, bit)) {
        if(!flipper_format_update_uint32(instance->fff_data, "TE", (uint32_t*)&te, 1)) {
            FURI_LOG_E(TAG, "Unable to update Te");
        } else {
            ret = true;
        }
    }
    return ret;
}

bool subghz_txrx_gen_keeloq_protocol(
    SubGhzTxRx* instance,
    const char* name_preset,
    uint32_t frequency,
    const char* name_sysmem,
    uint32_t serial,
    uint8_t btn,
    uint16_t cnt) {
    furi_assert(instance);

    bool ret = false;
    serial &= 0x0FFFFFFF;
    instance->transmitter =
        subghz_transmitter_alloc_init(instance->environment, SUBGHZ_PROTOCOL_KEELOQ_NAME);
    subghz_txrx_set_preset(instance, name_preset, frequency, NULL, 0);
    if(instance->transmitter) {
        subghz_protocol_keeloq_create_data(
            subghz_transmitter_get_protocol_instance(instance->transmitter),
            instance->fff_data,
            serial,
            btn,
            cnt,
            name_sysmem,
            instance->preset);
        ret = true;
    }
    subghz_transmitter_free(instance->transmitter);
    return ret;
}

bool subghz_txrx_gen_secplus_v2_protocol(
    SubGhzTxRx* instance,
    const char* name_preset,
    uint32_t frequency,
    uint32_t serial,
    uint8_t btn,
    uint32_t cnt) {
    furi_assert(instance);

    bool ret = false;
    instance->transmitter =
        subghz_transmitter_alloc_init(instance->environment, SUBGHZ_PROTOCOL_SECPLUS_V2_NAME);
    subghz_txrx_set_preset(instance, name_preset, frequency, NULL, 0);
    if(instance->transmitter) {
        subghz_protocol_secplus_v2_create_data(
            subghz_transmitter_get_protocol_instance(instance->transmitter),
            instance->fff_data,
            serial,
            btn,
            cnt,
            instance->preset);
        ret = true;
    }
    return ret;
}

bool subghz_txrx_gen_secplus_v1_protocol(
    SubGhzTxRx* instance,
    const char* name_preset,
    uint32_t frequency) {
    furi_assert(instance);

    bool ret = false;
    uint32_t serial = (uint32_t)rand();
    while(!subghz_protocol_secplus_v1_check_fixed(serial)) {
        serial = (uint32_t)rand();
    }
    if(subghz_txrx_gen_data_protocol(
           instance,
           name_preset,
           frequency,
           SUBGHZ_PROTOCOL_SECPLUS_V1_NAME,
           (uint64_t)serial << 32 | 0xE6000000,
           42)) {
        ret = true;
    }
    return ret;
}