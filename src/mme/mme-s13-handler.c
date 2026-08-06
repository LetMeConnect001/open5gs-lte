#include "mme-sm.h"
#include "mme-s13-handler.h"

static mme_s13_result_e result_from_diameter(
                const uint32_t *dia_err, const uint32_t *dia_exp_err);
static bool is_valid_equipment(
                uint32_t equipment_status_code, mme_eir_t eir_config);

mme_s13_result_e mme_s13_validate_message(ogs_diam_s13_message_t *s13_message)
{
    ogs_assert(s13_message);

    if (s13_message->result_code != ER_DIAMETER_SUCCESS) {
        ogs_warn("ME-Identity-Check failed [%d]",
                    s13_message->result_code);
        return result_from_diameter(
                s13_message->err, s13_message->exp_err);
    }

    return MME_S13_RESULT_ALLOWED;
}

mme_s13_result_e mme_s13_validate_eca(
        ogs_diam_s13_eca_message_t eca_message, mme_eir_t eir_config)
{
    if (is_valid_equipment(eca_message.equipment_status_code, eir_config)) {
        ogs_info("ME-Identity-Check accepted for "
                "equipment status code '%d'",
                eca_message.equipment_status_code);
        return MME_S13_RESULT_ALLOWED;
    } else {
        ogs_info("ME-Identity-Check rejected for "
                "equipment status code '%d'",
                eca_message.equipment_status_code);
        return MME_S13_RESULT_DENIED;
    }
}

mme_s13_result_e mme_s13_handle_eca(
        mme_ue_t *mme_ue, ogs_diam_s13_message_t *s13_message)
{
    mme_s13_result_e rc;

    ogs_assert(mme_ue);
    ogs_assert(s13_message);

    rc = mme_s13_validate_message(s13_message);
    if (rc != MME_S13_RESULT_ALLOWED)
        return rc;

    return mme_s13_validate_eca(s13_message->eca_message, mme_self()->eir);
}

/*
 * 3GPP TS 29.272 clause 7.4:
 * DIAMETER_ERROR_EQUIPMENT_UNKNOWN is the only application error the EIR
 * can return over S13. Every other failure means the EIR did not give a
 * verdict at all, so the decision is left to the operator policy rather
 * than being turned into a rejection here.
 */
static mme_s13_result_e result_from_diameter(
                const uint32_t *dia_err, const uint32_t *dia_exp_err)
{
    if (dia_exp_err && *dia_exp_err == OGS_DIAM_S13_ERROR_EQUIPMENT_UNKNOWN)
        return MME_S13_RESULT_DENIED;

    ogs_warn("No usable ME-Identity-Check verdict "
             "[Result-Code:%d Experimental-Result-Code:%d]",
             dia_err ? (int)*dia_err : -1,
             dia_exp_err ? (int)*dia_exp_err : -1);
    return MME_S13_RESULT_UNAVAILABLE;
}

static bool is_valid_equipment(
        uint32_t equipment_status_code, mme_eir_t eir_config)
{
    switch (equipment_status_code) {
    case OGS_DIAM_S13_EQUIPMENT_WHITELIST:
        return eir_config.allow_whitelist;
    case OGS_DIAM_S13_EQUIPMENT_GREYLIST:
        return eir_config.allow_greylist;
    case OGS_DIAM_S13_EQUIPMENT_BLACKLIST:
        return eir_config.allow_blacklist;
    default:
        return false;
    }
}
