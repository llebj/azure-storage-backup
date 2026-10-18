#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

#include "../src/lib.h"
#include "../src/parser.h"

void setUp(void)
{
}

void tearDown(void)
{
}

struct slice slice_from(char *s)
{
	return (struct slice) {
		.start = s,
		.size = strlen(s)
	};
}

struct attribute attribute(char *k, char *v)
{
	return (struct attribute) {
		.key = slice_from(k),
		.value = slice_from(v)
	};
}

// -------------
// -- parsing --
// -------------

void test_it_parses_valid_profiles(void)
{
	// Arrange
	struct section sections[2];
	struct attribute attributes[4];

	struct slice header = (struct slice) {
		.start = "Profile",
		.size = 7
	};

	sections[0] = (struct section) {
		.header = header,
		.tag = slice_from("root"),
		.attribs_offset = 0,
		.attribs_size = 2
	};
	attributes[0] = attribute("Source", "/");
	attributes[1] = attribute("Type", "preinstall");

	sections[1] = (struct section) {
		.header = header,
		.tag = slice_from("home"),
		.attribs_offset = 2,
		.attribs_size = 2
	};
	attributes[2] = attribute("Source", "/home");
	attributes[3] = attribute("Type", "preinstall");

	struct ini ini = {
		.sections = sections,
		.size_section = sizeof(sections) / sizeof(*sections),
		.attributes = attributes,
	};

	// Act
	struct file_profile profiles[ini.size_section];
	enum InstallerCode result = parse_profiles(&ini, profiles);

	// Assert
	TEST_ASSERT_EQUAL_INT(PI_OK, result);

	struct file_profile root = profiles[0];
	TEST_ASSERT_EQUAL_STRING("root", root.name);
	TEST_ASSERT_EQUAL_STRING("/", root.source);
	TEST_ASSERT_EQUAL_INT(PreInstall, root.type);

	struct file_profile home = profiles[1];
	TEST_ASSERT_EQUAL_STRING("home", home.name);
	TEST_ASSERT_EQUAL_STRING("/home", home.source);
	TEST_ASSERT_EQUAL_INT(PreInstall, home.type);
}

void test_it_requires_a_correct_header(void)
{
	// Arrange
	struct section sections[1];
	struct attribute attributes[2];

	struct slice header = (struct slice) {
		.start = "Wrong",
		.size = 5
	};

	sections[0] = (struct section) {
		.header = header,
		.tag = slice_from("root"),
		.attribs_offset = 0,
		.attribs_size = 2
	};
	attributes[0] = attribute("Source", "/");
	attributes[1] = attribute("Type", "preinstall");

	struct ini ini = {
		.sections = sections,
		.size_section = sizeof(sections) / sizeof(*sections),
		.attributes = attributes,
	};

	// Act
	struct file_profile profiles[ini.size_section];
	enum InstallerCode result = parse_profiles(&ini, profiles);

	// Assert
	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, result);
}

void test_it_requires_a_name(void)
{
	// Arrange
	struct section sections[1];
	struct attribute attributes[2];

	struct slice header = (struct slice) {
		.start = "Profile",
		.size = 5
	};

	sections[0] = (struct section) {
		.header = header,
		.tag = {0},
		.attribs_offset = 0,
		.attribs_size = 2
	};
	attributes[0] = attribute("Source", "/");
	attributes[1] = attribute("Type", "preinstall");

	struct ini ini = {
		.sections = sections,
		.size_section = sizeof(sections) / sizeof(*sections),
		.attributes = attributes,
	};

	// Act
	struct file_profile profiles[ini.size_section];
	enum InstallerCode result = parse_profiles(&ini, profiles);

	// Assert
	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, result);
}

void test_it_requires_a_source(void)
{
	// Arrange
	struct section sections[1];
	struct attribute attributes[1];

	struct slice header = (struct slice) {
		.start = "Profile",
		.size = 5
	};

	sections[0] = (struct section) {
		.header = header,
		.tag = slice_from("root"),
		.attribs_offset = 0,
		.attribs_size = 2
	};
	attributes[0] = attribute("Type", "preinstall");

	struct ini ini = {
		.sections = sections,
		.size_section = sizeof(sections) / sizeof(*sections),
		.attributes = attributes,
	};

	// Act
	struct file_profile profiles[ini.size_section];
	enum InstallerCode result = parse_profiles(&ini, profiles);

	// Assert
	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, result);
}

void test_it_parses_multi_part_types(void)
{
	// Arrange
	struct section sections[1];
	struct attribute attributes[2];

	struct slice header = (struct slice) {
		.start = "Profile",
		.size = 5
	};

	sections[0] = (struct section) {
		.header = header,
		.tag = slice_from("root"),
		.attribs_offset = 0,
		.attribs_size = 2
	};
	attributes[0] = attribute("Source", "/");
	attributes[1] = attribute("Type", "preinstall,postinstall");

	struct ini ini = {
		.sections = sections,
		.size_section = sizeof(sections) / sizeof(*sections),
		.attributes = attributes,
	};

	// Act
	struct file_profile profiles[ini.size_section];
	enum InstallerCode result = parse_profiles(&ini, profiles);

	// Assert
	TEST_ASSERT_EQUAL_INT(PI_OK, result);
	TEST_ASSERT_EQUAL_INT(PreInstall | PostInstall, profiles[0].type);
}

void test_it_does_not_accept_invalid_types(void)
{
	// Arrange
	struct section sections[1];
	struct attribute attributes[2];

	struct slice header = (struct slice) {
		.start = "Profile",
		.size = 5
	};

	sections[0] = (struct section) {
		.header = header,
		.tag = slice_from("root"),
		.attribs_offset = 0,
		.attribs_size = 2
	};
	attributes[0] = attribute("Source", "/");
	attributes[1] = attribute("Type", "invalid");

	struct ini ini = {
		.sections = sections,
		.size_section = sizeof(sections) / sizeof(*sections),
		.attributes = attributes,
	};

	// Act
	struct file_profile profiles[ini.size_section];
	enum InstallerCode result = parse_profiles(&ini, profiles);

	// Assert
	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, result);
}

void test_it_requires_a_non_null_ini(void)
{
	// Arrange
	struct section sections[1];
	struct attribute attributes[2];

	struct slice header = (struct slice) {
		.start = "Profile",
		.size = 7
	};

	sections[0] = (struct section) {
		.header = header,
		.tag = slice_from("root"),
		.attribs_offset = 0,
		.attribs_size = 2
	};
	attributes[0] = attribute("Source", "/");
	attributes[1] = attribute("Type", "preinstall");

	// Act
	struct file_profile profiles[sizeof(sections) / sizeof(*sections)];
	enum InstallerCode result = parse_profiles(NULL, profiles);

	// Assert
	TEST_ASSERT_EQUAL_INT(PI_FAIL, result);
}

void test_it_requires_a_non_null_profiles(void)
{
	// Arrange
	struct section sections[1];
	struct attribute attributes[2];

	struct slice header = (struct slice) {
		.start = "Profile",
		.size = 7
	};

	sections[0] = (struct section) {
		.header = header,
		.tag = slice_from("root"),
		.attribs_offset = 0,
		.attribs_size = 2
	};
	attributes[0] = attribute("Source", "/");
	attributes[1] = attribute("Type", "preinstall");

	struct ini ini = {
		.sections = sections,
		.size_section = sizeof(sections) / sizeof(*sections),
		.attributes = attributes,
	};

	// Act
	enum InstallerCode result = parse_profiles(&ini, NULL);

	// Assert
	TEST_ASSERT_EQUAL_INT(PI_FAIL, result);
}

// ----------
// -- Main --
// ----------

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_it_parses_valid_profiles);
	RUN_TEST(test_it_requires_a_correct_header);
	RUN_TEST(test_it_requires_a_name);
	RUN_TEST(test_it_requires_a_source);
	RUN_TEST(test_it_parses_multi_part_types);
	RUN_TEST(test_it_does_not_accept_invalid_types);
	RUN_TEST(test_it_requires_a_non_null_ini);
	RUN_TEST(test_it_requires_a_non_null_profiles);

	return UNITY_END();
}
