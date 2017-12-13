/*
 * Copyright (c) 2013 William Pitcock <nenolod@dereferenced.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * ECDSA-NIST256P-CHALLENGE mechanism provider.
 */

#include "atheme.h"

#if defined(HAVE_OPENSSL) && defined(HAVE_OPENSSL_EC_H)

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#define CHALLENGE_LENGTH	SHA256_DIGEST_LENGTH
#define CURVE_IDENTIFIER	NID_X9_62_prime256v1

enum ecdsa_step
{
	ECDSA_ST_ACCNAME    = 0,
	ECDSA_ST_RESPONSE   = 1,
	ECDSA_ST_COUNT      = 2,
};

struct ecdsa_session
{
	EC_KEY          *pubkey;
	unsigned char    challenge[CHALLENGE_LENGTH];
	enum ecdsa_step  step;
};

static const struct sasl_core_functions *sasl_core_functions = NULL;

static int
mech_start(struct sasl_session *const restrict p, void __attribute__((unused)) **const restrict out,
           size_t __attribute__((unused)) *const restrict outlen)
{
	struct ecdsa_session *const s = smalloc(sizeof *s);

	p->mechdata = s;
	s->pubkey = EC_KEY_new_by_curve_name(CURVE_IDENTIFIER);
	s->step = ECDSA_ST_ACCNAME;

	(void) EC_KEY_set_conv_form(s->pubkey, POINT_CONVERSION_COMPRESSED);

	return ASASL_MORE;
}

static int
mech_step_accname(struct sasl_session *const restrict p, const void *const restrict in, const size_t inlen,
                  void **const restrict out, size_t *const restrict outlen)
{
	struct ecdsa_session *const s = p->mechdata;

	if (! (in && inlen))
		return ASASL_FAIL;

	char authcid[NICKLEN];
	(void) memset(authcid, 0x00, sizeof authcid);

	const char *const end = memchr(in, 0x00, inlen);
	if (! end)
	{
		if (! inlen || inlen >= sizeof authcid)
			return ASASL_FAIL;

		(void) memcpy(authcid, in, inlen);
	}
	else
	{
		char authzid[NICKLEN];
		(void) memset(authzid, 0x00, sizeof authzid);

		const char *const accnames = in;

		const size_t authcid_length = (size_t) (end - accnames);
		const size_t authzid_length = inlen - 1 - authcid_length;

		if (! authcid_length || authcid_length >= sizeof authcid)
			return ASASL_FAIL;

		if (! authzid_length || authzid_length >= sizeof authcid)
			return ASASL_FAIL;

		(void) memcpy(authcid, accnames, authcid_length);
		(void) memcpy(authzid, end + 1, authzid_length);

		if (! sasl_core_functions->authzid_can_login(p, authzid, NULL))
			return ASASL_FAIL;
	}

	myuser_t *mu = NULL;
	if (! sasl_core_functions->authcid_can_login(p, authcid, &mu))
		return ASASL_FAIL;

	metadata_t *md;
	if (! (md = metadata_find(mu, "private:pubkey")) && ! (md = metadata_find(mu, "pubkey")))
		return ASASL_FAIL;

	unsigned char pubkey_raw[0x1000];
	(void) memset(pubkey_raw, 0x00, sizeof pubkey_raw);

	const size_t ret = base64_decode(md->value, pubkey_raw, sizeof pubkey_raw);
	if (ret == (size_t) -1)
		return ASASL_FAIL;

	const unsigned char *pubkey_raw_p = pubkey_raw;
	if (! o2i_ECPublicKey(&s->pubkey, &pubkey_raw_p, (long) ret))
		return ASASL_FAIL;

	*out = smalloc(CHALLENGE_LENGTH);
	*outlen = CHALLENGE_LENGTH;

	(void) arc4random_buf(s->challenge, CHALLENGE_LENGTH);
	(void) memcpy(*out, s->challenge, CHALLENGE_LENGTH);

	s->step = ECDSA_ST_RESPONSE;
	return ASASL_MORE;
}

static int
mech_step_response(struct sasl_session *const restrict p, const void *const restrict in, const size_t inlen,
                   void __attribute__((unused)) **const restrict out,
                   size_t __attribute__((unused)) *const restrict outlen)
{
	struct ecdsa_session *const s = p->mechdata;

	if (! (in && inlen))
		return ASASL_FAIL;

	if (ECDSA_verify(0, s->challenge, CHALLENGE_LENGTH, in, (int) inlen, s->pubkey) != 1)
		return ASASL_FAIL;

	return ASASL_DONE;
}

typedef int (*mech_stepfn_t)(struct sasl_session *, const void *, size_t, void **, size_t *);

static int
mech_step(struct sasl_session *const restrict p, const void *const restrict in, const size_t inlen,
          void **const restrict out, size_t *const restrict outlen)
{
	static mech_stepfn_t mech_steps[ECDSA_ST_COUNT] = {
		[ECDSA_ST_ACCNAME] = &mech_step_accname,
		[ECDSA_ST_RESPONSE] = &mech_step_response,
	};

	struct ecdsa_session *const s = p->mechdata;

	if (mech_steps[s->step] != NULL)
		return mech_steps[s->step](p, in, inlen, out, outlen);

	return ASASL_FAIL;
}

static void
mech_finish(struct sasl_session *const restrict p)
{
	if (! (p && p->mechdata))
		return;

	struct ecdsa_session *const s = p->mechdata;

	if (s->pubkey)
		(void) EC_KEY_free(s->pubkey);

	(void) free(s);

	p->mechdata = NULL;
}

static struct sasl_mechanism mech = {

	.name           = "ECDSA-NIST256P-CHALLENGE",
	.mech_start     = &mech_start,
	.mech_step      = &mech_step,
	.mech_finish    = &mech_finish,
};

static void
mod_init(module_t *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_pubkey");
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions");

	(void) sasl_core_functions->mech_register(&mech);
}

static void
mod_deinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) sasl_core_functions->mech_unregister(&mech);
}

SIMPLE_DECLARE_MODULE_V1("saslserv/ecdsa-nist256p-challenge", MODULE_UNLOAD_CAPABILITY_OK)

#endif
