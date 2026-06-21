/*================================================================
 * Static QR UPI Soundbox - Audio Script Builder
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_audio_prompt_logic.h"
#include "sb_audio_script.h"

sb_status_t sb_audio_script_build_status(sb_audio_language_t language,
                                         sb_audio_prompt_id_t prompt,
                                         sb_audio_script_t *script)
{
    if (script == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    switch (prompt) {
    case SB_AUDIO_PROMPT_POWER_ON:
        return sb_audio_prompt_logic_build_common("start_tune.mp3", script);
    case SB_AUDIO_PROMPT_TRANSACTION_ERROR:
        return sb_audio_prompt_logic_build_common("transaction_error.mp3", script);
    case SB_AUDIO_PROMPT_READY:
        return sb_audio_prompt_logic_build_alert(language, "internet.mp3", script);
    case SB_AUDIO_PROMPT_SETUP:
        return sb_audio_prompt_logic_build_alert(language, "unregistered_device.mp3", script);
    case SB_AUDIO_PROMPT_NO_SIM:
        return sb_audio_prompt_logic_build_alert(language, "no_SIM.mp3", script);
    case SB_AUDIO_PROMPT_NO_NETWORK:
    case SB_AUDIO_PROMPT_NO_INTERNET:
        return sb_audio_prompt_logic_build_alert(language, "no_internet.mp3", script);
    case SB_AUDIO_PROMPT_NO_MQTT:
        return sb_audio_prompt_logic_build_alert(language, "no_mqtt.mp3", script);
    case SB_AUDIO_PROMPT_BATTERY_LOW:
        return sb_audio_prompt_logic_build_alert(language, "battery_low.mp3", script);
    case SB_AUDIO_PROMPT_PAYMENT_RECEIVED:
    default:
        return sb_audio_prompt_logic_build_alert(language, "no_transactions.mp3", script);
    }
}

sb_status_t sb_audio_script_build_amount_received(sb_audio_language_t language,
                                                  sb_audio_provider_t provider,
                                                  u64 amount_paise,
                                                  sb_audio_script_t *script)
{
    return sb_audio_prompt_logic_build_transaction(language, provider, amount_paise, 0u, script);
}
