/*
 * Copyright 2019-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "mongocrypt-private.h"
#include "mongocrypt-binary-private.h"
#include "mongocrypt-buffer-private.h"
#include "mongocrypt-ctx-private.h"
#include "mongocrypt-kms-ctx-private.h"
#include "mongocrypt-opts-private.h"
#include "mongocrypt-status-private.h"
#include <kms_message/kms_b64.h>
#include <kms_message/kms_azure_request.h>
#include <kms_message/kms_gcp_request.h>
#include "mongocrypt.h"

typedef struct {
   mongocrypt_status_t *status;
   void *ctx;
} ctx_with_status_t;

/* Before we've read the Content-Length header in an HTTP response,
 * we don't know how many bytes we'll need. So return this value
 * in kms_ctx_bytes_needed until we are fed the Content-Length.
 */
#define DEFAULT_MAX_KMS_BYTE_REQUEST 1024
#define SHA256_LEN 32

static bool
_sha256 (void *ctx, const char *input, size_t len, unsigned char *hash_out)
{
   bool ret;
   ctx_with_status_t *ctx_with_status = (ctx_with_status_t *) ctx;
   _mongocrypt_crypto_t *crypto = (_mongocrypt_crypto_t *) ctx_with_status->ctx;
   mongocrypt_binary_t *plaintext, *out;

   plaintext =
      mongocrypt_binary_new_from_data ((uint8_t *) input, (uint32_t) len);
   out = mongocrypt_binary_new ();

   out->data = hash_out;
   out->len = SHA256_LEN;

   ret = crypto->sha_256 (crypto->ctx, plaintext, out, ctx_with_status->status);

   mongocrypt_binary_destroy (plaintext);
   mongocrypt_binary_destroy (out);
   return ret;
}

static bool
_sha256_hmac (void *ctx,
              const char *key_input,
              size_t key_len,
              const char *input,
              size_t len,
              unsigned char *hash_out)
{
   ctx_with_status_t *ctx_with_status = (ctx_with_status_t *) ctx;
   _mongocrypt_crypto_t *crypto = (_mongocrypt_crypto_t *) ctx_with_status->ctx;
   mongocrypt_binary_t *key, *plaintext, *out;
   bool ret;

   (void) crypto;

   key = mongocrypt_binary_new_from_data ((uint8_t *) key_input,
                                          (uint32_t) key_len);
   plaintext =
      mongocrypt_binary_new_from_data ((uint8_t *) input, (uint32_t) len);
   out = mongocrypt_binary_new ();

   out->data = hash_out;
   out->len = SHA256_LEN;

   ret = crypto->hmac_sha_256 (
      crypto->ctx, key, plaintext, out, ctx_with_status->status);

   mongocrypt_binary_destroy (key);
   mongocrypt_binary_destroy (plaintext);
   mongocrypt_binary_destroy (out);
   return ret;
}

static void
_set_kms_crypto_hooks (_mongocrypt_crypto_t *crypto,
                       ctx_with_status_t *ctx_with_status,
                       kms_request_opt_t *opts)
{
   if (crypto->hooks_enabled) {
      kms_request_opt_set_crypto_hooks (
         opts, _sha256, _sha256_hmac, ctx_with_status);
   }
}

static void
_init_common (mongocrypt_kms_ctx_t *kms,
              _mongocrypt_log_t *log,
              _kms_request_type_t kms_type)
{
   kms->parser = kms_response_parser_new ();
   kms->log = log;
   kms->status = mongocrypt_status_new ();
   kms->req_type = kms_type;
   _mongocrypt_buffer_init (&kms->result);
}

bool
_mongocrypt_kms_ctx_init_aws_decrypt (mongocrypt_kms_ctx_t *kms,
                                      _mongocrypt_opts_t *crypt_opts,
                                      _mongocrypt_key_doc_t *key,
                                      _mongocrypt_log_t *log,
                                      _mongocrypt_crypto_t *crypto)
{
   kms_request_opt_t *opt;
   mongocrypt_status_t *status;
   ctx_with_status_t ctx_with_status;
   bool ret = false;

   _init_common (kms, log, MONGOCRYPT_KMS_AWS_DECRYPT);
   status = kms->status;
   ctx_with_status.ctx = crypto;
   ctx_with_status.status = mongocrypt_status_new ();

   if (!key->kek.kms_provider) {
      CLIENT_ERR ("no kms provider specified on key");
      goto done;
   }

   if (MONGOCRYPT_KMS_PROVIDER_AWS != key->kek.kms_provider) {
      CLIENT_ERR ("expected aws kms provider");
      goto done;
   }

   if (!key->kek.provider.aws.region) {
      CLIENT_ERR ("no key region provided");
      goto done;
   }

   if (0 == (crypt_opts->kms_providers & MONGOCRYPT_KMS_PROVIDER_AWS)) {
      CLIENT_ERR ("aws kms not configured");
      goto done;
   }

   if (!crypt_opts->kms_provider_aws.access_key_id) {
      CLIENT_ERR ("aws access key id not provided");
      goto done;
   }

   if (!crypt_opts->kms_provider_aws.secret_access_key) {
      CLIENT_ERR ("aws secret access key not provided");
      goto done;
   }

   /* create the KMS request. */
   opt = kms_request_opt_new ();
   BSON_ASSERT (opt);

   _set_kms_crypto_hooks (crypto, &ctx_with_status, opt);
   kms_request_opt_set_connection_close (opt, true);

   kms->req = kms_decrypt_request_new (
      key->key_material.data, key->key_material.len, opt);

   kms_request_opt_destroy (opt);
   kms_request_set_service (kms->req, "kms");

   if (crypt_opts->kms_provider_aws.session_token) {
      kms_request_add_header_field (kms->req,
                                    "X-Amz-Security-Token",
                                    crypt_opts->kms_provider_aws.session_token);
   }

   if (kms_request_get_error (kms->req)) {
      CLIENT_ERR ("error constructing KMS message: %s",
                  kms_request_get_error (kms->req));
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }

   /* If an endpoint was set, override the default Host header. */
   if (key->kek.provider.aws.endpoint) {
      if (!kms_request_add_header_field (
             kms->req, "Host", key->kek.provider.aws.endpoint->host_and_port)) {
         CLIENT_ERR ("error constructing KMS message: %s",
                     kms_request_get_error (kms->req));
         _mongocrypt_status_append (status, ctx_with_status.status);
         goto done;
      }
   }

   if (!kms_request_set_region (kms->req, key->kek.provider.aws.region)) {
      CLIENT_ERR ("failed to set region");
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }

   if (!kms_request_set_access_key_id (
          kms->req, crypt_opts->kms_provider_aws.access_key_id)) {
      CLIENT_ERR ("failed to set aws access key id");
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }
   if (!kms_request_set_secret_key (
          kms->req, crypt_opts->kms_provider_aws.secret_access_key)) {
      CLIENT_ERR ("failed to set aws secret access key");
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }

   _mongocrypt_buffer_init (&kms->msg);
   kms->msg.data = (uint8_t *) kms_request_get_signed (kms->req);
   if (!kms->msg.data) {
      CLIENT_ERR ("failed to create KMS message");
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }
   kms->msg.len = (uint32_t) strlen ((char *) kms->msg.data);
   kms->msg.owned = true;

   if (key->kek.provider.aws.endpoint) {
      kms->endpoint =
         bson_strdup (key->kek.provider.aws.endpoint->host_and_port);
   } else {
      /* construct the endpoint from AWS region. */
      kms->endpoint = bson_strdup_printf ("kms.%s.amazonaws.com",
                                          key->kek.provider.aws.region);
   }

   ret = true;
done:
   mongocrypt_status_destroy (ctx_with_status.status);

   return ret;
}


bool
_mongocrypt_kms_ctx_init_aws_encrypt (
   mongocrypt_kms_ctx_t *kms,
   _mongocrypt_opts_t *crypt_opts,
   _mongocrypt_ctx_opts_t *ctx_opts,
   _mongocrypt_buffer_t *plaintext_key_material,
   _mongocrypt_log_t *log,
   _mongocrypt_crypto_t *crypto)
{
   kms_request_opt_t *opt;
   mongocrypt_status_t *status;
   ctx_with_status_t ctx_with_status;
   bool ret = false;

   _init_common (kms, log, MONGOCRYPT_KMS_AWS_ENCRYPT);
   status = kms->status;
   ctx_with_status.ctx = crypto;
   ctx_with_status.status = mongocrypt_status_new ();

   if (MONGOCRYPT_KMS_PROVIDER_AWS != ctx_opts->kek.kms_provider) {
      CLIENT_ERR ("expected aws kms provider");
      goto done;
   }

   if (!ctx_opts->kek.provider.aws.region) {
      CLIENT_ERR ("no key region provided");
      goto done;
   }

   if (!ctx_opts->kek.provider.aws.cmk) {
      CLIENT_ERR ("no aws cmk provided");
      goto done;
   }

   if (0 == (crypt_opts->kms_providers & MONGOCRYPT_KMS_PROVIDER_AWS)) {
      CLIENT_ERR ("aws kms not configured");
      goto done;
   }

   if (!crypt_opts->kms_provider_aws.access_key_id) {
      CLIENT_ERR ("aws access key id not provided");
      goto done;
   }

   if (!crypt_opts->kms_provider_aws.secret_access_key) {
      CLIENT_ERR ("aws secret access key not provided");
      goto done;
   }

   /* create the KMS request. */
   opt = kms_request_opt_new ();
   BSON_ASSERT (opt);


   _set_kms_crypto_hooks (crypto, &ctx_with_status, opt);
   kms_request_opt_set_connection_close (opt, true);

   kms->req = kms_encrypt_request_new (plaintext_key_material->data,
                                       plaintext_key_material->len,
                                       ctx_opts->kek.provider.aws.cmk,
                                       opt);

   kms_request_opt_destroy (opt);
   kms_request_set_service (kms->req, "kms");

   if (crypt_opts->kms_provider_aws.session_token) {
      kms_request_add_header_field (kms->req,
                                    "X-Amz-Security-Token",
                                    crypt_opts->kms_provider_aws.session_token);
   }

   if (kms_request_get_error (kms->req)) {
      CLIENT_ERR ("error constructing KMS message: %s",
                  kms_request_get_error (kms->req));
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }

   /* If an endpoint was set, override the default Host header. */
   if (ctx_opts->kek.provider.aws.endpoint) {
      if (!kms_request_add_header_field (
             kms->req, "Host", ctx_opts->kek.provider.aws.endpoint->host)) {
         CLIENT_ERR ("error constructing KMS message: %s",
                     kms_request_get_error (kms->req));
         _mongocrypt_status_append (status, ctx_with_status.status);
         goto done;
      }
   }

   if (!kms_request_set_region (kms->req, ctx_opts->kek.provider.aws.region)) {
      CLIENT_ERR ("failed to set region");
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }

   if (!kms_request_set_access_key_id (
          kms->req, crypt_opts->kms_provider_aws.access_key_id)) {
      CLIENT_ERR ("failed to set aws access key id");
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }
   if (!kms_request_set_secret_key (
          kms->req, crypt_opts->kms_provider_aws.secret_access_key)) {
      CLIENT_ERR ("failed to set aws secret access key");
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }

   _mongocrypt_buffer_init (&kms->msg);
   kms->msg.data = (uint8_t *) kms_request_get_signed (kms->req);
   if (!kms->msg.data) {
      CLIENT_ERR ("failed to create KMS message");
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto done;
   }
   kms->msg.len = (uint32_t) strlen ((char *) kms->msg.data);
   kms->msg.owned = true;

   /* construct the endpoint */
   if (ctx_opts->kek.provider.aws.endpoint) {
      kms->endpoint =
         bson_strdup (ctx_opts->kek.provider.aws.endpoint->host_and_port);
   } else {
      kms->endpoint = bson_strdup_printf ("kms.%s.amazonaws.com",
                                          ctx_opts->kek.provider.aws.region);
   }

   ret = true;
done:
   mongocrypt_status_destroy (ctx_with_status.status);
   return ret;
}


uint32_t
mongocrypt_kms_ctx_bytes_needed (mongocrypt_kms_ctx_t *kms)
{
   if (!kms) {
      return 0;
   }
   /* TODO: an oddity of kms-message. After retrieving the JSON result, it
    * resets the parser. */
   if (!mongocrypt_status_ok (kms->status) ||
       !_mongocrypt_buffer_empty (&kms->result)) {
      return 0;
   }
   return kms_response_parser_wants_bytes (kms->parser,
                                           DEFAULT_MAX_KMS_BYTE_REQUEST);
}

/* An AWS KMS context has received full response. Parse out the result or error.
 */
static bool
_ctx_done_aws (mongocrypt_kms_ctx_t *kms, const char *json_field)
{
   kms_response_t *response = NULL;
   const char *body;
   bson_t body_bson = BSON_INITIALIZER;
   bool ret;
   bson_error_t bson_error;
   bson_iter_t iter;
   uint32_t b64_strlen;
   char *b64_str;
   int http_status;
   size_t body_len;
   mongocrypt_status_t *status;

   status = kms->status;
   ret = false;
   /* Parse out the {en|de}crypted result. */
   http_status = kms_response_parser_status (kms->parser);
   response = kms_response_parser_get_response (kms->parser);
   body = kms_response_get_body (response, &body_len);

   if (http_status != 200) {
      /* 1xx, 2xx, and 3xx HTTP status codes are not errors, but we only
       * support handling 200 response. */
      if (http_status < 400) {
         CLIENT_ERR ("Unsupported HTTP code in KMS response. HTTP status=%d",
                     http_status);
         goto fail;
      }

      /* Either empty body or body containing JSON with error message. */
      if (body_len == 0) {
         CLIENT_ERR ("Error in KMS response. HTTP status=%d", http_status);
         goto fail;
      }
      /* AWS error responses include a JSON message, like { "message":
       * "error" } */
      bson_destroy (&body_bson);
      if (!bson_init_from_json (&body_bson, body, body_len, &bson_error)) {
         bson_init (&body_bson);
      } else if (bson_iter_init_find (&iter, &body_bson, "message") &&
                 BSON_ITER_HOLDS_UTF8 (&iter)) {
         CLIENT_ERR ("Error in KMS response '%s'. "
                     "HTTP status=%d",
                     bson_iter_utf8 (&iter, NULL),
                     http_status);
         goto fail;
      }

      /* If we couldn't parse JSON, return the body unchanged as an error. */
      CLIENT_ERR ("Error parsing JSON in KMS response '%s'. HTTP status=%d",
                  body,
                  http_status);
      goto fail;
   }

   /* If HTTP response succeeded (status 200) then body should contain JSON.
    */
   bson_destroy (&body_bson);
   if (!bson_init_from_json (&body_bson, body, body_len, &bson_error)) {
      CLIENT_ERR ("Error parsing JSON in KMS response '%s'. "
                  "HTTP status=%d",
                  bson_error.message,
                  http_status);
      bson_init (&body_bson);
      goto fail;
   }

   if (!bson_iter_init_find (&iter, &body_bson, json_field) ||
       !BSON_ITER_HOLDS_UTF8 (&iter)) {
      CLIENT_ERR (
         "KMS JSON response does not include string '%s'. HTTP status=%d",
         json_field,
         http_status);
      goto fail;
   }

   b64_str = (char *) bson_iter_utf8 (&iter, &b64_strlen);
   BSON_ASSERT (b64_str);
   kms->result.data = bson_malloc (b64_strlen + 1);
   BSON_ASSERT (kms->result.data);

   kms->result.len =
      kms_message_b64_pton (b64_str, kms->result.data, b64_strlen);
   kms->result.owned = true;
   ret = true;
fail:
   bson_destroy (&body_bson);
   kms_response_destroy (response);
   return ret;
}

/* A Azure/GCP oauth KMS context has received full response. Parse out the
 * bearer token or error. */
static bool
_ctx_done_oauth (mongocrypt_kms_ctx_t *kms)
{
   kms_response_t *response = NULL;
   const char *body;
   bson_t *bson_body = NULL;
   bool ret;
   bson_error_t bson_error;
   bson_iter_t iter;
   int http_status;
   size_t body_len;
   mongocrypt_status_t *status;

   status = kms->status;
   ret = false;
   /* Parse out the oauth token result (or error). */
   http_status = kms_response_parser_status (kms->parser);
   response = kms_response_parser_get_response (kms->parser);
   body = kms_response_get_body (response, &body_len);

   if (body_len == 0) {
      CLIENT_ERR ("Empty KMS response. HTTP status=%d", http_status);
      goto fail;
   }

   bson_body =
      bson_new_from_json ((const uint8_t *) body, body_len, &bson_error);
   if (!bson_body) {
      CLIENT_ERR ("Invalid JSON in KMS response. HTTP status=%d. Error: %s",
                  http_status,
                  bson_error.message);
      goto fail;
   }

   if (http_status != 200) {
      const char *error = "";
      const char *error_description = "";
      /* Check for oauth errors.
       * https://docs.microsoft.com/en-us/azure/active-directory/develop/reference-aadsts-error-codes#handling-error-codes-in-your-application.
       * 'error' provides a short error classification
       * 'error_description' is a long error description
       */
      if (bson_iter_init_find (&iter, bson_body, "error") &&
          BSON_ITER_HOLDS_UTF8 (&iter)) {
         error = bson_iter_utf8 (&iter, NULL);
      }
      if (bson_iter_init_find (&iter, bson_body, "error_description") &&
          BSON_ITER_HOLDS_UTF8 (&iter)) {
         error_description = bson_iter_utf8 (&iter, NULL);
      }
      CLIENT_ERR ("Error in KMS response: '%s', '%s'. HTTP status=%d",
                  error,
                  error_description,
                  http_status);
      goto fail;
   }

   if (!bson_iter_init_find (&iter, bson_body, "access_token") ||
       !BSON_ITER_HOLDS_UTF8 (&iter)) {
      CLIENT_ERR (
         "Invalid KMS response, no access_token returned. HTTP status=%d",
         http_status);
      goto fail;
   }

   /* Store the full response, to include the expiration time. */
   _mongocrypt_buffer_steal_from_bson (&kms->result, bson_body);
   bson_body = NULL;

   ret = true;
fail:
   bson_destroy (bson_body);
   kms_response_destroy (response);
   return ret;
}

/* An Azure oauth KMS context has received full response. Parse out the bearer
 * token or error. */
static bool
_ctx_done_azure_wrapkey_unwrapkey (mongocrypt_kms_ctx_t *kms)
{
   kms_response_t *response = NULL;
   const char *body;
   bson_t *bson_body = NULL;
   bool ret;
   bson_error_t bson_error;
   bson_iter_t iter;
   int http_status;
   size_t body_len;
   mongocrypt_status_t *status;
   const char *b64url_data = NULL;
   uint32_t b64url_len;
   char *b64_data = NULL;
   uint32_t b64_len;

   status = kms->status;
   ret = false;
   /* Parse out the oauth token result (or error). */
   http_status = kms_response_parser_status (kms->parser);
   response = kms_response_parser_get_response (kms->parser);
   body = kms_response_get_body (response, &body_len);

   if (body_len == 0) {
      CLIENT_ERR ("Empty KMS response. HTTP status=%d", http_status);
      goto fail;
   }

   bson_body =
      bson_new_from_json ((const uint8_t *) body, body_len, &bson_error);
   if (!bson_body) {
      CLIENT_ERR ("Invalid JSON in KMS response. HTTP status=%d", http_status);
      goto fail;
   }

   if (http_status != 200) {
      const char *message = "";
      /* Check for errors.
       * https://docs.microsoft.com/en-us/rest/api/keyvault/wrapkey/wrapkey#error
       */
      if (bson_iter_init (&iter, bson_body) &&
          bson_iter_find_descendant (&iter, "error.message", &iter) &&
          BSON_ITER_HOLDS_UTF8 (&iter)) {
         message = bson_iter_utf8 (&iter, NULL);
      }
      CLIENT_ERR (
         "Error in KMS response: '%s'. HTTP status=%d", message, http_status);
      goto fail;
   }

   if (!bson_iter_init_find (&iter, bson_body, "value") ||
       !BSON_ITER_HOLDS_UTF8 (&iter)) {
      CLIENT_ERR ("Invalid KMS response, no value returned. HTTP status=%d",
                  http_status);
      goto fail;
   }

   b64url_data = bson_iter_utf8 (&iter, &b64url_len);
   /* add four for padding. */
   b64_len = b64url_len + 4;
   b64_data = bson_malloc0 (b64_len);
   if (kms_message_b64url_to_b64 (b64url_data, b64url_len, b64_data, b64_len) ==
       -1) {
      CLIENT_ERR ("Error converting base64url to base64");
      goto fail;
   }
   kms->result.data = bson_malloc0 (b64_len);
   kms->result.len = kms_message_b64_pton (b64_data, kms->result.data, b64_len);
   kms->result.owned = true;

   ret = true;
fail:
   bson_destroy (bson_body);
   kms_response_destroy (response);
   bson_free (b64_data);
   return ret;
}

/* A GCP KMS context has received full response. Parse out the result or error.
 */
static bool
_ctx_done_gcp (mongocrypt_kms_ctx_t *kms, const char *json_field)
{
   kms_response_t *response = NULL;
   const char *body;
   bson_t body_bson = BSON_INITIALIZER;
   bool ret;
   bson_error_t bson_error;
   bson_iter_t iter;
   size_t outlen;
   char *b64_str;
   int http_status;
   size_t body_len;
   mongocrypt_status_t *status;

   status = kms->status;
   ret = false;
   /* Parse out the {en|de}crypted result. */
   http_status = kms_response_parser_status (kms->parser);
   response = kms_response_parser_get_response (kms->parser);
   body = kms_response_get_body (response, &body_len);

   if (http_status != 200) {
      /* 1xx, 2xx, and 3xx HTTP status codes are not errors, but we only
       * support handling 200 response. */
      if (http_status < 400) {
         CLIENT_ERR ("Unsupported HTTP code in KMS response. HTTP status=%d",
                     http_status);
         goto fail;
      }

      /* Either empty body or body containing JSON with error message. */
      if (body_len == 0) {
         CLIENT_ERR ("Error in KMS response. HTTP status=%d", http_status);
         goto fail;
      }
      /* GCP error responses include a JSON message, like { "message":
       * "error", "code": <num> } */
      bson_destroy (&body_bson);
      if (!bson_init_from_json (&body_bson, body, body_len, &bson_error)) {
         bson_init (&body_bson);
      } else {
         const char *msg = "";
         int32_t code = 0;

         if (bson_iter_init_find (&iter, &body_bson, "message") &&
             BSON_ITER_HOLDS_UTF8 (&iter)) {
            msg = bson_iter_utf8 (&iter, NULL);
         }

         if (bson_iter_init_find (&iter, &body_bson, "code") &&
             BSON_ITER_HOLDS_INT32 (&iter)) {
            code = bson_iter_int32 (&iter);
         }

         CLIENT_ERR ("Error in KMS response '%s', code: '%d'. "
                     "HTTP status=%d",
                     msg,
                     code,
                     http_status);
         goto fail;
      }

      /* If we couldn't parse JSON, return the body unchanged as an error. */
      CLIENT_ERR ("Error parsing JSON in KMS response '%s'. HTTP status=%d",
                  body,
                  http_status);
      goto fail;
   }

   /* If HTTP response succeeded (status 200) then body should contain JSON.
    */
   bson_destroy (&body_bson);
   if (!bson_init_from_json (&body_bson, body, body_len, &bson_error)) {
      CLIENT_ERR ("Error parsing JSON in KMS response '%s'. "
                  "HTTP status=%d",
                  bson_error.message,
                  http_status);
      bson_init (&body_bson);
      goto fail;
   }

   if (!bson_iter_init_find (&iter, &body_bson, json_field) ||
       !BSON_ITER_HOLDS_UTF8 (&iter)) {
      CLIENT_ERR (
         "KMS JSON response does not include string '%s'. HTTP status=%d",
         json_field,
         http_status);
      goto fail;
   }


   b64_str = (char *) bson_iter_utf8 (&iter, NULL);
   BSON_ASSERT (b64_str);
   kms->result.data = kms_message_b64_to_raw (b64_str, &outlen);
   kms->result.len = (uint32_t) outlen;
   kms->result.owned = true;
   ret = true;
fail:
   bson_destroy (&body_bson);
   kms_response_destroy (response);
   return ret;
}

bool
mongocrypt_kms_ctx_feed (mongocrypt_kms_ctx_t *kms, mongocrypt_binary_t *bytes)
{
   mongocrypt_status_t *status;

   if (!kms) {
      return false;
   }

   status = kms->status;
   if (!mongocrypt_status_ok (status)) {
      return false;
   }

   if (!bytes) {
      CLIENT_ERR ("argument 'bytes' is required");
      return false;
   }

   if (bytes->len > mongocrypt_kms_ctx_bytes_needed (kms)) {
      CLIENT_ERR ("KMS response fed too much data");
      return false;
   }

   if (kms->log->trace_enabled) {
      _mongocrypt_log (kms->log,
                       MONGOCRYPT_LOG_LEVEL_TRACE,
                       "%s (%s=\"%.*s\")",
                       BSON_FUNC,
                       "bytes",
                       mongocrypt_binary_len (bytes),
                       mongocrypt_binary_data (bytes));
   }

   if (!kms_response_parser_feed (kms->parser, bytes->data, bytes->len)) {
      CLIENT_ERR ("KMS response parser error with status %d, error: '%s'",
                  kms_response_parser_status (kms->parser),
                  kms_response_parser_error (kms->parser));
      return false;
   }

   if (0 == mongocrypt_kms_ctx_bytes_needed (kms)) {
      if (kms->req_type == MONGOCRYPT_KMS_AWS_ENCRYPT) {
         return _ctx_done_aws (kms, "CiphertextBlob");
      } else if (kms->req_type == MONGOCRYPT_KMS_AWS_DECRYPT) {
         return _ctx_done_aws (kms, "Plaintext");
      } else if (kms->req_type == MONGOCRYPT_KMS_AZURE_OAUTH) {
         return _ctx_done_oauth (kms);
      } else if (kms->req_type == MONGOCRYPT_KMS_AZURE_WRAPKEY) {
         return _ctx_done_azure_wrapkey_unwrapkey (kms);
      } else if (kms->req_type == MONGOCRYPT_KMS_AZURE_UNWRAPKEY) {
         return _ctx_done_azure_wrapkey_unwrapkey (kms);
      } else if (kms->req_type == MONGOCRYPT_KMS_GCP_OAUTH) {
         return _ctx_done_oauth (kms);
      } else if (kms->req_type == MONGOCRYPT_KMS_GCP_ENCRYPT) {
         return _ctx_done_gcp (kms, "ciphertext");
      } else if (kms->req_type == MONGOCRYPT_KMS_GCP_DECRYPT) {
         return _ctx_done_gcp (kms, "plaintext");
      } else {
         CLIENT_ERR ("Unknown request type");
         return false;
      }
   }
   return true;
}


bool
_mongocrypt_kms_ctx_result (mongocrypt_kms_ctx_t *kms,
                            _mongocrypt_buffer_t *out)
{
   mongocrypt_status_t *status;

   status = kms->status;

   /* If we have no status, we were never initialized */
   if (!status) {
      return false;
   }

   if (!mongocrypt_status_ok (status)) {
      return false;
   }

   if (mongocrypt_kms_ctx_bytes_needed (kms) > 0) {
      CLIENT_ERR ("KMS response unfinished");
      return false;
   }

   _mongocrypt_buffer_init (out);
   out->data = kms->result.data;
   out->len = kms->result.len;
   return true;
}


bool
mongocrypt_kms_ctx_status (mongocrypt_kms_ctx_t *kms,
                           mongocrypt_status_t *status_out)
{
   if (!kms) {
      return false;
   }

   if (!status_out) {
      mongocrypt_status_t *status = kms->status;
      CLIENT_ERR ("argument 'status' is required");
      return false;
   }
   _mongocrypt_status_copy_to (kms->status, status_out);
   return mongocrypt_status_ok (status_out);
}


void
_mongocrypt_kms_ctx_cleanup (mongocrypt_kms_ctx_t *kms)
{
   if (!kms) {
      return;
   }
   if (kms->req) {
      kms_request_destroy (kms->req);
   }
   if (kms->parser) {
      kms_response_parser_destroy (kms->parser);
   }
   mongocrypt_status_destroy (kms->status);
   _mongocrypt_buffer_cleanup (&kms->msg);
   _mongocrypt_buffer_cleanup (&kms->result);
   bson_free (kms->endpoint);
}


bool
mongocrypt_kms_ctx_message (mongocrypt_kms_ctx_t *kms, mongocrypt_binary_t *msg)
{
   if (!kms) {
      return false;
   }

   if (!msg) {
      mongocrypt_status_t *status = kms->status;
      CLIENT_ERR ("argument 'msg' is required");
      return false;
   }
   msg->data = kms->msg.data;
   msg->len = kms->msg.len;
   return true;
}


bool
mongocrypt_kms_ctx_endpoint (mongocrypt_kms_ctx_t *kms, const char **endpoint)
{
   if (!kms) {
      return false;
   }
   if (!endpoint) {
      mongocrypt_status_t *status = kms->status;
      CLIENT_ERR ("argument 'endpoint' is required");
      return false;
   }
   *endpoint = kms->endpoint;
   return true;
}

bool
_mongocrypt_kms_ctx_init_azure_auth (mongocrypt_kms_ctx_t *kms,
                                     _mongocrypt_log_t *log,
                                     _mongocrypt_opts_t *crypt_opts,
                                     _mongocrypt_endpoint_t *key_vault_endpoint)
{
   kms_request_opt_t *opt = NULL;
   mongocrypt_status_t *status;
   _mongocrypt_endpoint_t *identity_platform_endpoint;
   char *scope = NULL;
   const char *host;
   char *request_string;
   bool ret = false;

   _init_common (kms, log, MONGOCRYPT_KMS_AZURE_OAUTH);
   status = kms->status;
   identity_platform_endpoint =
      crypt_opts->kms_provider_azure.identity_platform_endpoint;

   if (identity_platform_endpoint) {
      kms->endpoint = bson_strdup (identity_platform_endpoint->host_and_port);
      host = identity_platform_endpoint->host;
   } else {
      kms->endpoint = bson_strdup ("login.microsoftonline.com");
      host = kms->endpoint;
   }

   if (key_vault_endpoint) {
      /* Request a custom scope. It is URL encoded, like
       * https%3A%2F%2Fvault.azure.net%2F.default */
      scope = bson_strdup_printf (
         "%s%s%s", "https%3A%2F%2F", key_vault_endpoint->domain, "%2F.default");
   } else {
      /* Default to commercial Azure endpoint. */
      scope = bson_strdup ("https%3A%2F%2Fvault.azure.net%2F.default");
   }

   opt = kms_request_opt_new ();
   BSON_ASSERT (opt);
   kms_request_opt_set_connection_close (opt, true);
   kms_request_opt_set_provider (opt, KMS_REQUEST_PROVIDER_AZURE);
   kms->req =
      kms_azure_request_oauth_new (host,
                                   scope,
                                   crypt_opts->kms_provider_azure.tenant_id,
                                   crypt_opts->kms_provider_azure.client_id,
                                   crypt_opts->kms_provider_azure.client_secret,
                                   opt);
   if (kms_request_get_error (kms->req)) {
      CLIENT_ERR ("error constructing KMS message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }

   request_string = kms_request_to_string (kms->req);
   if (!request_string) {
      CLIENT_ERR ("error getting Azure OAuth KMS message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }
   _mongocrypt_buffer_init (&kms->msg);
   kms->msg.data = (uint8_t *) request_string;
   kms->msg.len = (uint32_t) strlen (request_string);
   kms->msg.owned = true;

   ret = true;
fail:
   bson_free (scope);
   kms_request_opt_destroy (opt);
   return ret;
}

bool
_mongocrypt_kms_ctx_init_azure_wrapkey (
   mongocrypt_kms_ctx_t *kms,
   _mongocrypt_log_t *log,
   _mongocrypt_opts_t *crypt_opts,
   struct __mongocrypt_ctx_opts_t *ctx_opts,
   const char *access_token,
   _mongocrypt_buffer_t *plaintext_key_material)
{
   kms_request_opt_t *opt = NULL;
   mongocrypt_status_t *status;
   char *path_and_query = NULL;
   char *payload = NULL;
   const char *host;
   char *request_string;
   bool ret = false;
   char *bearer_token_value = NULL;

   _init_common (kms, log, MONGOCRYPT_KMS_AZURE_WRAPKEY);
   status = kms->status;

   kms->endpoint = bson_strdup (
      ctx_opts->kek.provider.azure.key_vault_endpoint->host_and_port);
   host = ctx_opts->kek.provider.azure.key_vault_endpoint->host;

   opt = kms_request_opt_new ();
   BSON_ASSERT (opt);
   kms_request_opt_set_connection_close (opt, true);
   kms_request_opt_set_provider (opt, KMS_REQUEST_PROVIDER_AZURE);
   kms->req =
      kms_azure_request_wrapkey_new (host,
                                     access_token,
                                     ctx_opts->kek.provider.azure.key_name,
                                     ctx_opts->kek.provider.azure.key_version,
                                     plaintext_key_material->data,
                                     plaintext_key_material->len,
                                     opt);

   if (kms_request_get_error (kms->req)) {
      CLIENT_ERR ("error constructing KMS wrapkey message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }

   request_string = kms_request_to_string (kms->req);
   if (!request_string) {
      CLIENT_ERR ("error getting Azure wrapkey KMS message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }
   _mongocrypt_buffer_init (&kms->msg);
   kms->msg.data = (uint8_t *) request_string;
   kms->msg.len = (uint32_t) strlen (request_string);
   kms->msg.owned = true;

   ret = true;
fail:
   kms_request_opt_destroy (opt);
   bson_free (path_and_query);
   bson_free (payload);
   bson_free (bearer_token_value);
   return ret;
}

bool
_mongocrypt_kms_ctx_init_azure_unwrapkey (mongocrypt_kms_ctx_t *kms,
                                          _mongocrypt_opts_t *crypt_opts,
                                          const char *access_token,
                                          _mongocrypt_key_doc_t *key,
                                          _mongocrypt_log_t *log)
{
   kms_request_opt_t *opt = NULL;
   mongocrypt_status_t *status;
   char *path_and_query = NULL;
   char *payload = NULL;
   const char *host;
   char *request_string;
   bool ret = false;
   char *bearer_token_value = NULL;

   _init_common (kms, log, MONGOCRYPT_KMS_AZURE_UNWRAPKEY);
   status = kms->status;

   kms->endpoint =
      bson_strdup (key->kek.provider.azure.key_vault_endpoint->host_and_port);
   host = key->kek.provider.azure.key_vault_endpoint->host;

   opt = kms_request_opt_new ();
   BSON_ASSERT (opt);
   kms_request_opt_set_connection_close (opt, true);
   kms_request_opt_set_provider (opt, KMS_REQUEST_PROVIDER_AZURE);
   kms->req =
      kms_azure_request_unwrapkey_new (host,
                                       access_token,
                                       key->kek.provider.azure.key_name,
                                       key->kek.provider.azure.key_version,
                                       key->key_material.data,
                                       key->key_material.len,
                                       opt);

   if (kms_request_get_error (kms->req)) {
      CLIENT_ERR ("error constructing KMS unwrapkey message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }

   request_string = kms_request_to_string (kms->req);
   if (!request_string) {
      CLIENT_ERR ("error getting Azure unwrapkey KMS message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }
   _mongocrypt_buffer_init (&kms->msg);
   kms->msg.data = (uint8_t *) request_string;
   kms->msg.len = (uint32_t) strlen (request_string);
   kms->msg.owned = true;

   ret = true;
fail:
   kms_request_opt_destroy (opt);
   bson_free (path_and_query);
   bson_free (payload);
   bson_free (bearer_token_value);
   return ret;
}

#define RSAES_PKCS1_V1_5_SIGNATURE_LEN 256

/* This is the form of the callback that KMS message calls. */
bool
_sign_rsaes_pkcs1_v1_5_trampoline (void *ctx,
                                   const char *private_key,
                                   size_t private_key_len,
                                   const char *input,
                                   size_t input_len,
                                   unsigned char *signature_out)
{
   ctx_with_status_t *ctx_with_status;
   _mongocrypt_opts_t *crypt_opts;
   mongocrypt_binary_t private_key_bin;
   mongocrypt_binary_t input_bin;
   mongocrypt_binary_t output_bin;
   bool ret;

   ctx_with_status = (ctx_with_status_t *) ctx;
   crypt_opts = (_mongocrypt_opts_t *) ctx_with_status->ctx;
   private_key_bin.data = (uint8_t *) private_key;
   private_key_bin.len = (uint32_t) private_key_len;
   input_bin.data = (uint8_t *) input;
   input_bin.len = (uint32_t) input_len;
   output_bin.data = (uint8_t *) signature_out;
   output_bin.len = RSAES_PKCS1_V1_5_SIGNATURE_LEN;

   ret = crypt_opts->sign_rsaes_pkcs1_v1_5 (crypt_opts->sign_ctx,
                                            &private_key_bin,
                                            &input_bin,
                                            &output_bin,
                                            ctx_with_status->status);
   return ret;
}

bool
_mongocrypt_kms_ctx_init_gcp_auth (mongocrypt_kms_ctx_t *kms,
                                   _mongocrypt_log_t *log,
                                   _mongocrypt_opts_t *crypt_opts,
                                   _mongocrypt_endpoint_t *kms_endpoint)
{
   kms_request_opt_t *opt = NULL;
   mongocrypt_status_t *status;
   _mongocrypt_endpoint_t *auth_endpoint;
   char *scope = NULL;
   char *audience = NULL;
   const char *host;
   char *request_string;
   bool ret = false;
   ctx_with_status_t ctx_with_status;

   _init_common (kms, log, MONGOCRYPT_KMS_GCP_OAUTH);
   status = kms->status;
   auth_endpoint = crypt_opts->kms_provider_gcp.endpoint;
   ctx_with_status.ctx = crypt_opts;
   ctx_with_status.status = mongocrypt_status_new ();

   if (auth_endpoint) {
      kms->endpoint = bson_strdup (auth_endpoint->host_and_port);
      host = auth_endpoint->host;
      audience = bson_strdup_printf ("https://%s/token", auth_endpoint->host);
   } else {
      kms->endpoint = bson_strdup ("oauth2.googleapis.com");
      host = kms->endpoint;
      audience = bson_strdup_printf ("https://oauth2.googleapis.com/token");
   }

   if (kms_endpoint) {
      /* Request a custom scope. */
      scope = bson_strdup_printf ("https://www.%s/auth/cloudkms",
                                  kms_endpoint->domain);
   } else {
      scope = bson_strdup ("https://www.googleapis.com/auth/cloudkms");
   }

   opt = kms_request_opt_new ();
   BSON_ASSERT (opt);
   kms_request_opt_set_connection_close (opt, true);
   kms_request_opt_set_provider (opt, KMS_REQUEST_PROVIDER_GCP);
   if (crypt_opts->sign_rsaes_pkcs1_v1_5) {
      kms_request_opt_set_crypto_hook_sign_rsaes_pkcs1_v1_5 (
         opt, _sign_rsaes_pkcs1_v1_5_trampoline, &ctx_with_status);
   }
   kms->req = kms_gcp_request_oauth_new (
      host,
      crypt_opts->kms_provider_gcp.email,
      audience,
      scope,
      (const char *) crypt_opts->kms_provider_gcp.private_key.data,
      crypt_opts->kms_provider_gcp.private_key.len,
      opt);
   if (kms_request_get_error (kms->req)) {
      CLIENT_ERR ("error constructing KMS message: %s",
                  kms_request_get_error (kms->req));
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto fail;
   }

   request_string = kms_request_to_string (kms->req);
   if (!request_string) {
      CLIENT_ERR ("error getting GCP OAuth KMS message: %s",
                  kms_request_get_error (kms->req));
      _mongocrypt_status_append (status, ctx_with_status.status);
      goto fail;
   }
   _mongocrypt_buffer_init (&kms->msg);
   kms->msg.data = (uint8_t *) request_string;
   kms->msg.len = (uint32_t) strlen (request_string);
   kms->msg.owned = true;

   ret = true;
fail:
   bson_free (scope);
   bson_free (audience);
   kms_request_opt_destroy (opt);
   mongocrypt_status_destroy (ctx_with_status.status);
   return ret;
}

bool
_mongocrypt_kms_ctx_init_gcp_encrypt (
   mongocrypt_kms_ctx_t *kms,
   _mongocrypt_log_t *log,
   _mongocrypt_opts_t *crypt_opts,
   struct __mongocrypt_ctx_opts_t *ctx_opts,
   const char *access_token,
   _mongocrypt_buffer_t *plaintext_key_material)
{
   kms_request_opt_t *opt = NULL;
   mongocrypt_status_t *status;
   char *path_and_query = NULL;
   char *payload = NULL;
   const char *host;
   char *request_string;
   bool ret = false;
   char *bearer_token_value = NULL;

   _init_common (kms, log, MONGOCRYPT_KMS_GCP_ENCRYPT);
   status = kms->status;

   if (ctx_opts->kek.provider.gcp.endpoint) {
      kms->endpoint =
         bson_strdup (ctx_opts->kek.provider.gcp.endpoint->host_and_port);
      host = ctx_opts->kek.provider.gcp.endpoint->host;
   } else {
      kms->endpoint = bson_strdup ("cloudkms.googleapis.com");
      host = kms->endpoint;
   }

   opt = kms_request_opt_new ();
   BSON_ASSERT (opt);
   kms_request_opt_set_connection_close (opt, true);
   kms_request_opt_set_provider (opt, KMS_REQUEST_PROVIDER_GCP);
   kms->req =
      kms_gcp_request_encrypt_new (host,
                                   access_token,
                                   ctx_opts->kek.provider.gcp.project_id,
                                   ctx_opts->kek.provider.gcp.location,
                                   ctx_opts->kek.provider.gcp.key_ring,
                                   ctx_opts->kek.provider.gcp.key_name,
                                   ctx_opts->kek.provider.gcp.key_version,
                                   plaintext_key_material->data,
                                   plaintext_key_material->len,
                                   opt);

   if (kms_request_get_error (kms->req)) {
      CLIENT_ERR ("error constructing GCP KMS encrypt message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }

   request_string = kms_request_to_string (kms->req);
   if (!request_string) {
      CLIENT_ERR ("error getting GCP KMS encrypt KMS message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }
   _mongocrypt_buffer_init (&kms->msg);
   kms->msg.data = (uint8_t *) request_string;
   kms->msg.len = (uint32_t) strlen (request_string);
   kms->msg.owned = true;

   ret = true;
fail:
   kms_request_opt_destroy (opt);
   bson_free (path_and_query);
   bson_free (payload);
   bson_free (bearer_token_value);
   return ret;
}

bool
_mongocrypt_kms_ctx_init_gcp_decrypt (mongocrypt_kms_ctx_t *kms,
                                      _mongocrypt_opts_t *crypt_opts,
                                      const char *access_token,
                                      _mongocrypt_key_doc_t *key,
                                      _mongocrypt_log_t *log)
{
   kms_request_opt_t *opt = NULL;
   mongocrypt_status_t *status;
   char *path_and_query = NULL;
   char *payload = NULL;
   const char *host;
   char *request_string;
   bool ret = false;
   char *bearer_token_value = NULL;

   _init_common (kms, log, MONGOCRYPT_KMS_GCP_DECRYPT);
   status = kms->status;

   if (key->kek.provider.gcp.endpoint) {
      kms->endpoint =
         bson_strdup (key->kek.provider.gcp.endpoint->host_and_port);
      host = key->kek.provider.gcp.endpoint->host;
   } else {
      kms->endpoint = bson_strdup ("cloudkms.googleapis.com");
      host = kms->endpoint;
   }

   opt = kms_request_opt_new ();
   BSON_ASSERT (opt);
   kms_request_opt_set_connection_close (opt, true);
   kms_request_opt_set_provider (opt, KMS_REQUEST_PROVIDER_GCP);
   kms->req = kms_gcp_request_decrypt_new (host,
                                           access_token,
                                           key->kek.provider.gcp.project_id,
                                           key->kek.provider.gcp.location,
                                           key->kek.provider.gcp.key_ring,
                                           key->kek.provider.gcp.key_name,
                                           key->key_material.data,
                                           key->key_material.len,
                                           opt);

   if (kms_request_get_error (kms->req)) {
      CLIENT_ERR ("error constructing GCP KMS decrypt message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }

   request_string = kms_request_to_string (kms->req);
   if (!request_string) {
      CLIENT_ERR ("error getting GCP KMS decrypt KMS message: %s",
                  kms_request_get_error (kms->req));
      goto fail;
   }
   _mongocrypt_buffer_init (&kms->msg);
   kms->msg.data = (uint8_t *) request_string;
   kms->msg.len = (uint32_t) strlen (request_string);
   kms->msg.owned = true;

   ret = true;
fail:
   kms_request_opt_destroy (opt);
   bson_free (path_and_query);
   bson_free (payload);
   bson_free (bearer_token_value);
   return ret;
}