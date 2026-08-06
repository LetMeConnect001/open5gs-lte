#include "nas-path.h"
#include "s1ap-path.h"
#include "sgsap-path.h"
#include "mme-path.h"

#include "mme-sm.h"
#include "mme-s13-handler.h"


static uint8_t emm_cause_from_diameter(
                const uint32_t *dia_err, const uint32_t *dia_exp_err);
static bool is_valid_equipment(
                uint32_t equipment_status_code, mme_eir_t eir_config);

uint8_t mme_s13_validate_message(ogs_diam_s13_message_t *s13_message)
{
    ogs_assert(s13_message);

    if (s13_message->result_code != ER_DIAMETER_SUCCESS) {
        ogs_warn("ME-Identity-Check failed [%d]",
                    s13_message->result_code);
        return emm_cause_from_diameter(
                s13_message->err, s13_message->exp_err);
    }

    return OGS_NAS_EMM_CAUSE_REQUEST_ACCEPTED;
}

uint8_t mme_s13_validate_eca(
        ogs_diam_s13_eca_message_t eca_message, mme_eir_t eir_config)
{
    if (is_valid_equipment(eca_message.equipment_status_code, eir_config)) {
        ogs_info("ME-Identity-Check accepted for "
                "equipment status code '%d'",
                eca_message.equipment_status_code);
        return OGS_NAS_EMM_CAUSE_REQUEST_ACCEPTED;
    } else {
        ogs_info("ME-Identity-Check rejected for "
                "equipment status code '%d'",
                eca_message.equipment_status_code);
        return OGS_NAS_EMM_CAUSE_ILLEGAL_ME;
    }
}

uint8_t mme_s13_handle_eca(
        mme_ue_t *mme_ue, ogs_diam_s13_message_t *s13_message)
{
    uint8_t rc;

    ogs_assert(mme_ue);
    ogs_assert(s13_message);

    rc = mme_s13_validate_message(s13_message);
    if (rc != OGS_NAS_EMM_CAUSE_REQUEST_ACCEPTED)
        return rc;

    return mme_s13_validate_eca(s13_message->eca_message, mme_self()->eir);
}

/* 3GPP TS 29.272 Annex A; Table A.1:
 * Mapping from S13 error codes to NAS Cause Codes */
static uint8_t emm_cause_from_diameter(
                const uint32_t *dia_err, const uint32_t *dia_exp_err)
{
    if (dia_exp_err) {
        switch (*dia_exp_err) {
        case OGS_DIAM_S13_ERROR_USER_UNKNOWN:                   /* 5001 */
            return OGS_NAS_EMM_CAUSE_PLMN_NOT_ALLOWED;
        case OGS_DIAM_S13_ERROR_UNKNOWN_EPS_SUBSCRIPTION:       /* 5420 */
            return OGS_NAS_EMM_CAUSE_NO_SUITABLE_CELLS_IN_TRACKING_AREA;
        case OGS_DIAM_S13_ERROR_RAT_NOT_ALLOWED:                /* 5421 */
            return OGS_NAS_EMM_CAUSE_ROAMING_NOT_ALLOWED_IN_THIS_TRACKING_AREA;
        case OGS_DIAM_S13_ERROR_ROAMING_NOT_ALLOWED:            /* 5004 */
            return OGS_NAS_EMM_CAUSE_PLMN_NOT_ALLOWED;
        case OGS_DIAM_S13_AUTHENTICATION_DATA_UNAVAILABLE:      /* 4181 */
            return OGS_NAS_EMM_CAUSE_NETWORK_FAILURE;
        }
    }
    if (dia_err) {
        switch (*dia_err) {
        case ER_DIAMETER_AUTHORIZATION_REJECTED:                /* 5003 */
        case ER_DIAMETER_UNABLE_TO_DELIVER:                     /* 3002 */
        case ER_DIAMETER_REALM_NOT_SERVED:                      /* 3003 */
            return OGS_NAS_EMM_CAUSE_NO_SUITABLE_CELLS_IN_TRACKING_AREA;
        case ER_DIAMETER_UNABLE_TO_COMPLY:                      /* 5012 */
        case ER_DIAMETER_INVALID_AVP_VALUE:                     /* 5004 */
        case ER_DIAMETER_AVP_UNSUPPORTED:                       /* 5001 */
        case ER_DIAMETER_MISSING_AVP:                           /* 5005 */
        case ER_DIAMETER_RESOURCES_EXCEEDED:                    /* 5006 */
        case ER_DIAMETER_AVP_OCCURS_TOO_MANY_TIMES:             /* 5009 */
            return OGS_NAS_EMM_CAUSE_NETWORK_FAILURE;
        }
    }

    ogs_error("Unexpected Diameter Result Code %d/%d, defaulting to severe "
              "network failure",
              dia_err ? *dia_err : -1, dia_exp_err ? *dia_exp_err : -1);
    return OGS_NAS_EMM_CAUSE_SEVERE_NETWORK_FAILURE;
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