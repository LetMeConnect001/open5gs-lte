#ifndef MME_S13_HANDLER_H
#define MME_S13_HANDLER_H

#include "mme-context.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t mme_s13_handle_eca(
        mme_ue_t *mme_ue, ogs_diam_s13_message_t *s13_message);

/* Exposed for testing purposes only; should not be called directly. */
uint8_t mme_s13_validate_message(ogs_diam_s13_message_t *s13_message);
uint8_t mme_s13_validate_eca(
        ogs_diam_s13_eca_message_t eca_message, mme_eir_t eir_config);

#ifdef __cplusplus
}
#endif

#endif /* MME_S13_HANDLER_H */