// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_GSSAPI_POSIX_H_
#define NET_HTTP_HTTP_AUTH_GSSAPI_POSIX_H_

#include <string>

#include "base/gtest_prod_util.h"
#include "base/native_library.h"
#include "net/http/http_auth.h"

#define GSS_USE_FUNCTION_POINTERS
#include "net/third_party/gssapi/gssapi.h"

class GURL;

namespace net {

class HttpRequestInfo;
class ProxyInfo;

extern gss_OID CHROME_GSS_C_NT_HOSTBASED_SERVICE_X;
extern gss_OID CHROME_GSS_C_NT_HOSTBASED_SERVICE;

// GSSAPILibrary is introduced so unit tests can mock the calls to the GSSAPI
// library. The default implementation attempts to load one of the standard
// GSSAPI library implementations, then simply passes the arguments on to
// that implementation.
class GSSAPILibrary {
 public:
  virtual ~GSSAPILibrary() {}

  // Initializes the library, including any necessary dynamic libraries.
  virtual bool Init() = 0;

  // These methods match the ones in the GSSAPI library.
  virtual OM_uint32 import_name(
      OM_uint32* minor_status,
      const gss_buffer_t input_name_buffer,
      const gss_OID input_name_type,
      gss_name_t* output_name) = 0;
  virtual OM_uint32 release_name(
      OM_uint32* minor_status,
      gss_name_t* input_name) = 0;
  virtual OM_uint32 release_buffer(
      OM_uint32* minor_status,
      gss_buffer_t buffer) = 0;
  virtual OM_uint32 display_status(
      OM_uint32* minor_status,
      OM_uint32 status_value,
      int status_type,
      const gss_OID mech_type,
      OM_uint32* message_contex,
      gss_buffer_t status_string) = 0;
  virtual OM_uint32 init_sec_context(
      OM_uint32* minor_status,
      const gss_cred_id_t initiator_cred_handle,
      gss_ctx_id_t* context_handle,
      const gss_name_t target_name,
      const gss_OID mech_type,
      OM_uint32 req_flags,
      OM_uint32 time_req,
      const gss_channel_bindings_t input_chan_bindings,
      const gss_buffer_t input_token,
      gss_OID* actual_mech_type,
      gss_buffer_t output_token,
      OM_uint32* ret_flags,
      OM_uint32* time_rec) = 0;
  virtual OM_uint32 wrap_size_limit(
      OM_uint32* minor_status,
      const gss_ctx_id_t context_handle,
      int conf_req_flag,
      gss_qop_t qop_req,
      OM_uint32 req_output_size,
      OM_uint32* max_input_size) = 0;

  // Get the default GSSPILibrary instance. The object returned is a singleton
  // instance, and the caller should not delete it.
  static GSSAPILibrary* GetDefault();
};

// GSSAPISharedLibrary class is defined here so that unit tests can access it.
class GSSAPISharedLibrary : public GSSAPILibrary {
 public:
  GSSAPISharedLibrary();
  virtual ~GSSAPISharedLibrary();

  // GSSAPILibrary methods:
  virtual bool Init();
  virtual OM_uint32 import_name(
      OM_uint32* minor_status,
      const gss_buffer_t input_name_buffer,
      const gss_OID input_name_type,
      gss_name_t* output_name);
  virtual OM_uint32 release_name(
      OM_uint32* minor_status,
      gss_name_t* input_name);
  virtual OM_uint32 release_buffer(
      OM_uint32* minor_status,
      gss_buffer_t buffer);
  virtual OM_uint32 display_status(
      OM_uint32* minor_status,
      OM_uint32 status_value,
      int status_type,
      const gss_OID mech_type,
      OM_uint32* message_contex,
      gss_buffer_t status_string);
  virtual OM_uint32 init_sec_context(
      OM_uint32* minor_status,
      const gss_cred_id_t initiator_cred_handle,
      gss_ctx_id_t* context_handle,
      const gss_name_t target_name,
      const gss_OID mech_type,
      OM_uint32 req_flags,
      OM_uint32 time_req,
      const gss_channel_bindings_t input_chan_bindings,
      const gss_buffer_t input_token,
      gss_OID* actual_mech_type,
      gss_buffer_t output_token,
      OM_uint32* ret_flags,
      OM_uint32* time_rec);
  virtual OM_uint32 wrap_size_limit(
      OM_uint32* minor_status,
      const gss_ctx_id_t context_handle,
      int conf_req_flag,
      gss_qop_t qop_req,
      OM_uint32 req_output_size,
      OM_uint32* max_input_size);

 private:
  FRIEND_TEST_ALL_PREFIXES(HttpAuthGSSAPIPOSIXTest, GSSAPIStartup);

  bool InitImpl();
  static base::NativeLibrary LoadSharedObject();
  bool BindMethods();

  bool initialized_;

  // Need some way to invalidate the library.
  base::NativeLibrary gssapi_library_;

  // Function pointers
  gss_import_name_type import_name_;
  gss_release_name_type release_name_;
  gss_release_buffer_type release_buffer_;
  gss_display_status_type display_status_;
  gss_init_sec_context_type init_sec_context_;
  gss_wrap_size_limit_type wrap_size_limit_;
};

// TODO(cbentzel): Share code with HttpAuthSSPI.
class HttpAuthGSSAPI {
 public:
  HttpAuthGSSAPI(GSSAPILibrary* library,
                 const std::string& scheme,
                 const gss_OID gss_oid);
  ~HttpAuthGSSAPI();

  bool NeedsIdentity() const;
  bool IsFinalRound() const;

  bool ParseChallenge(HttpAuth::ChallengeTokenizer* tok);

  // Generates an authentication token.
  // The return value is an error code. If it's not |OK|, the value of
  // |*auth_token| is unspecified.
  // |spn| is the Service Principal Name of the server that the token is
  // being generated for.
  // If this is the first round of a multiple round scheme, credentials are
  // obtained using |*username| and |*password|. If |username| and |password|
  // are NULL, the default credentials are used instead.
  int GenerateAuthToken(const std::wstring* username,
                        const std::wstring* password,
                        const std::wstring& spn,
                        const HttpRequestInfo* request,
                        const ProxyInfo* proxy,
                        std::string* out_credentials);

 private:
  int OnFirstRound(const std::wstring* username,
                   const std::wstring* password);
  int GetNextSecurityToken(const std::wstring& spn,
                           gss_buffer_t in_token,
                           gss_buffer_t out_token);

  std::string scheme_;
  std::wstring username_;
  std::wstring password_;
  gss_OID gss_oid_;
  GSSAPILibrary* library_;
  std::string decoded_server_auth_token_;
  gss_ctx_id_t sec_context_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_GSSAPI_POSIX_H_
