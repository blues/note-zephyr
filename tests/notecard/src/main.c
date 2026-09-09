/*
 * Copyright (c) 2026 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

/*
 * Hardware-in-the-loop tests for the note-zephyr transport layer.
 *
 * These run on a real Swan with a real Notecard attached, so they deliberately
 * avoid anything that depends on cellular or Notehub connectivity: a test that
 * waits for a sync would be flaky in CI for reasons that have nothing to do
 * with this module. Everything here talks to the Notecard over the local
 * transport (I2C or UART, whichever the devicetree selects) and asserts on the
 * response it gets back.
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

#include <note.h>

LOG_MODULE_REGISTER(notecard_hil, LOG_LEVEL_INF);

/* Notefile used by the note.add cases. Queued outbound (.qo) rather than
 * synced, so nothing leaves the device.
 */
#define TEST_NOTEFILE "hil.qo"

/*
 * Issue a request and return the response. Fails the current test if the
 * request could not be allocated or the Notecard did not answer at all --
 * both of which mean the transport is broken rather than the request being
 * rejected. The caller owns the response.
 */
static J *hil_request(const char *request)
{
	J *req = NoteNewRequest(request);

	zassert_not_null(req, "failed to allocate '%s' request", request);

	J *rsp = NoteRequestResponse(req);

	zassert_not_null(rsp, "no response to '%s' -- is the Notecard wired up?",
			 request);

	return rsp;
}

/*
 * Drop the test notefile so note.add case totals start from a known point.
 * A missing file is not an error worth failing on, so the response is only
 * logged.
 */
static void *notecard_hil_setup(void)
{
	J *req = NoteNewRequest("file.delete");

	if (req != NULL) {
		J *files = JAddArrayToObject(req, "files");

		if (files != NULL) {
			JAddItemToArray(files, JCreateString(TEST_NOTEFILE));
		}

		J *rsp = NoteRequestResponse(req);

		if (rsp != NULL) {
			if (NoteResponseError(rsp)) {
				LOG_INF("file.delete of %s: %s (expected if absent)",
					TEST_NOTEFILE, JGetString(rsp, "err"));
			}
			NoteDeleteResponse(rsp);
		}
	}

	return NULL;
}

ZTEST_SUITE(notecard_hil, NULL, notecard_hil_setup, NULL, NULL, NULL);

/*
 * The transport works at all: the Notecard answers card.version and the
 * response carries the fields note-c expects to parse. This is the case to
 * read first when the suite goes red -- if it fails, the rest will too.
 */
ZTEST(notecard_hil, test_card_version)
{
	J *rsp = hil_request("card.version");

	zassert_false(NoteResponseError(rsp), "card.version returned: %s",
		      JGetString(rsp, "err"));

	const char *version = JGetString(rsp, "version");

	zassert_not_null(version, "card.version response had no 'version'");
	zassert_true(strlen(version) > 0, "card.version 'version' was empty");

	zassert_true(JGetInt(rsp, "api") > 0,
		     "card.version response had no usable 'api' field");

	LOG_INF("Notecard version: %s", version);

	NoteDeleteResponse(rsp);
}

/*
 * Consecutive exchanges. I2C framing bugs tend to show up on the request
 * *after* the first rather than on the first itself, so issue the same
 * request repeatedly and require an identical answer each time -- that
 * catches a truncated or offset read, which a bare "did it error" check
 * would let through.
 */
ZTEST(notecard_hil, test_consecutive_requests_are_stable)
{
	char first[64] = {0};

	for (int i = 0; i < 3; i++) {
		J *rsp = hil_request("card.version");

		zassert_false(NoteResponseError(rsp),
			      "card.version #%d returned: %s", i,
			      JGetString(rsp, "err"));

		const char *version = JGetString(rsp, "version");

		zassert_not_null(version, "card.version #%d had no 'version'", i);

		if (i == 0) {
			strncpy(first, version, sizeof(first) - 1);
		} else {
			zassert_str_equal(first, version,
					  "card.version #%d returned '%s', "
					  "expected '%s' -- transport is "
					  "corrupting or misaligning reads",
					  i, version, first);
		}

		NoteDeleteResponse(rsp);
	}
}

/*
 * A request that writes: hub.set is what every example issues at startup, so
 * a regression here breaks all of them.
 */
ZTEST(notecard_hil, test_hub_set)
{
	J *req = NoteNewRequest("hub.set");

	zassert_not_null(req, "failed to allocate hub.set request");

	JAddStringToObject(req, "product", CONFIG_BLUES_NOTEHUB_PRODUCT_UID);
	JAddStringToObject(req, "mode", "continuous");
	JAddStringToObject(req, "sn", "note-zephyr-hil");

	J *rsp = NoteRequestResponse(req);

	zassert_not_null(rsp, "no response to hub.set");
	zassert_false(NoteResponseError(rsp), "hub.set returned: %s",
		      JGetString(rsp, "err"));

	NoteDeleteResponse(rsp);
}

/*
 * Queue a Note locally. No "sync" is requested, so this exercises the
 * request/response path with a nested body and does not depend on the
 * Notecard having a connection.
 */
ZTEST(notecard_hil, test_note_add_queues_locally)
{
	J *req = NoteNewRequest("note.add");

	zassert_not_null(req, "failed to allocate note.add request");

	JAddStringToObject(req, "file", TEST_NOTEFILE);

	J *body = JAddObjectToObject(req, "body");

	zassert_not_null(body, "failed to add body to note.add request");
	JAddNumberToObject(body, "count", 1);
	JAddStringToObject(body, "source", "note-zephyr-hil");

	J *rsp = NoteRequestResponse(req);

	zassert_not_null(rsp, "no response to note.add");
	zassert_false(NoteResponseError(rsp), "note.add returned: %s",
		      JGetString(rsp, "err"));

	zassert_true(JGetInt(rsp, "total") >= 1,
		     "note.add reported total %lld, expected at least 1",
		     (long long)JGetInt(rsp, "total"));

	NoteDeleteResponse(rsp);
}

/*
 * Errors have to come back as errors. Without this, every other case in the
 * suite could pass against a transport that silently swallowed failures.
 */
ZTEST(notecard_hil, test_unknown_request_returns_error)
{
	J *rsp = hil_request("note.zephyr.no.such.request");

	zassert_true(NoteResponseError(rsp),
		     "an unknown request should have returned an 'err'");

	LOG_INF("unknown request rejected with: %s", JGetString(rsp, "err"));

	NoteDeleteResponse(rsp);
}
