#ifndef NEW_TH_TEST_PLATFORM_H
#define NEW_TH_TEST_PLATFORM_H

#include "../src/server/sendbufr.h"
#include <errno.h>
#include <string.h>

#ifdef DOS_DIVIDER
#define DIVIDER "\\"
#else
#define DIVIDER "/"
#endif

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>

#pragma endregion

static void PathCombineString(void **state) {
    const char *path1 = "foo", *path2 = "bar";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "foo" DIVIDER "bar");
}

static void PathCombineStringNoDivider(void **state) {
    const char *path1 = DIVIDER "foo", *path2 = "bar" DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER "foo" DIVIDER "bar" DIVIDER);
}

static void PathCombineStringTrailingDivider(void **state) {
    const char *path1 = DIVIDER "foo" DIVIDER, *path2 = "bar" DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER "foo" DIVIDER "bar" DIVIDER);
}

static void PathCombineStringLeadingDivider(void **state) {
    const char *path1 = DIVIDER "foo", *path2 = DIVIDER "bar" DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER "foo" DIVIDER "bar" DIVIDER);
}

static void PathCombineStringBothDividers(void **state) {
    const char *path1 = DIVIDER "foo" DIVIDER, *path2 = DIVIDER "bar" DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER "foo" DIVIDER "bar" DIVIDER);
}

static void PathCombineStringJustDividers(void **state) {
    const char *path1 = DIVIDER DIVIDER DIVIDER DIVIDER, *path2 = path1;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER);
}

static void PathCombineStringDumbInput(void **state) {
    const char *path1, *path2 = path1 = DIVIDER;
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, DIVIDER);
}

static void PathCombineStringRelativeInput(void **state) {
    const char *path1 = ".", *path2 = "foo";
    char output[FILENAME_MAX];
    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "./foo");
}

static void HaystackAndNeedle(void **state) {
    const char *msg1 = "foo", *msg2 = "bar", *msg3 = "foobar", *msg4 = "barfoo", *msg5 = "";
    const char *MSG1 = "FOO", *MSG2 = "BAR";

    /* Empty string shouldn't match, what is there to match? */
    assert_null(platformStringFindNeedle(msg5, msg5));

    /* 'barfoo' starts to match 'foobar' but doesn't */
    assert_null(platformStringFindNeedle(msg3, msg4));

    /* 'foo' and 'bar' both match inside 'foobar' */
    assert_ptr_equal(platformStringFindNeedle(msg3, msg1), msg3);
    assert_ptr_equal(platformStringFindNeedle(msg3, msg2), &msg3[3]);

    /* 'FOO' and 'BAR' both match inside 'foobar' since the function is case insensitive */
    assert_ptr_equal(platformStringFindNeedle(msg3, MSG1), msg3);
    assert_ptr_equal(platformStringFindNeedle(msg3, MSG2), &msg3[3]);
}

#ifdef DOS_DIVIDER

static void PathCombineStringUnixDividers(void **state) {
    const char *path1 = "/foo/", *path2 = "/bar/";
    char output[FILENAME_MAX];

    platformPathCombine(output, path1, path2);
    assert_string_equal(output, "/foo" DIVIDER "bar/");
}

#endif

#pragma clang diagnostic pop

const struct CMUnitTest platformTest[] = {cmocka_unit_test(HaystackAndNeedle), cmocka_unit_test(PathCombineString),
                                          cmocka_unit_test(PathCombineStringBothDividers),
                                          cmocka_unit_test(PathCombineStringDumbInput),
                                          cmocka_unit_test(PathCombineStringJustDividers),
                                          cmocka_unit_test(PathCombineStringLeadingDivider),
                                          cmocka_unit_test(PathCombineStringNoDivider),
                                          cmocka_unit_test(PathCombineStringRelativeInput),
                                          cmocka_unit_test(PathCombineStringTrailingDivider)
#ifdef BACKSLASH_PATH_DIVIDER
        ,cmocka_unit_test(PathCombineStringUnixDividers)
#endif
};

#endif /* NEW_TH_TEST_PLATFORM_H */
