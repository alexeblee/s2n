/*
 * Copyright 2014 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <s2n.h>

#include "crypto/s2n_certificate.h"
#include "error/s2n_errno.h"
#include "tls/s2n_client_cert_preferences.h"
#include "tls/s2n_cipher_suites.h"
#include "tls/s2n_connection.h"
#include "tls/s2n_config.h"
#include "tls/s2n_signature_algorithms.h"
#include "tls/s2n_tls.h"
#include "stuffer/s2n_stuffer.h"
#include "utils/s2n_safety.h"


int s2n_client_cert_req_recv(struct s2n_connection *conn)
{
    struct s2n_stuffer *in = &conn->handshake.io;

    s2n_cert_type cert_type_out;    
    GUARD(s2n_recv_client_cert_preferences(in, &cert_type_out));

    if(conn->actual_protocol_version == S2N_TLS12){
        s2n_recv_supported_signature_algorithms(conn, in, &conn->handshake_params.server_sig_hash_algs);

        s2n_hash_algorithm chosen_hash_algorithm;
        s2n_signature_algorithm chosen_signature_algorithm;
        GUARD(s2n_set_signature_hash_pair_from_preference_list(conn, &conn->handshake_params.server_sig_hash_algs, &chosen_hash_algorithm, &chosen_signature_algorithm));
        conn->secure.client_cert_hash_algorithm = chosen_hash_algorithm;
        conn->secure.client_cert_sig_alg = chosen_signature_algorithm;
    }
    uint16_t cert_authorities_len = 0;
    GUARD(s2n_stuffer_read_uint16(in, &cert_authorities_len));

    if(cert_authorities_len != 0) {
        /* Avoid parsing X.501 encoded CA Distinguished Names */
        S2N_ERROR(S2N_ERR_UNIMPLEMENTED);
    }
    return 0;
}


int s2n_client_cert_req_send(struct s2n_connection *conn)
{
    struct s2n_stuffer *out = &conn->handshake.io;

    uint8_t client_cert_preference_list_size = sizeof(s2n_cert_type_preference_list);
    GUARD(s2n_stuffer_write_uint8(out, client_cert_preference_list_size));

    for(int i = 0; i < client_cert_preference_list_size; i++) {
        GUARD(s2n_stuffer_write_uint8(out, s2n_cert_type_preference_list[i]));
    }

    if (conn->actual_protocol_version == S2N_TLS12) {
        GUARD(s2n_send_supported_signature_algorithms(out));
    }

    /* RFC 5246 7.4.4 - If the certificate_authorities list is empty, then the
     * client MAY send any certificate of the appropriate ClientCertificateType */
    uint16_t acceptable_cert_authorities_len = 0;
    GUARD(s2n_stuffer_write_uint16(out, acceptable_cert_authorities_len));

    return 0;
}
