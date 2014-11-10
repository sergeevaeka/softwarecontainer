/*
 *   Copyright (C) 2014 Pelagicore AB
 *   All rights reserved.
 */

#include <vector>

struct testData
{
    const char *title;
    const char *data;
    const std::vector<std::string> names;
    const std::vector<std::string> majors;
    const std::vector<std::string> minors;
    const std::vector<std::string> modes;
};

void PrintTo(const testData &d, ::std::ostream *os)
{
    *os << "Title: " << d.title;
    *os << "\nData: " << d.data;
}

const struct testData invalidConfigs[] = {
    {
        "Missing closing ]",
        "{\"devices\": ["
        "                  {"
        "                      \"name\":  \"tty0\","
        "                      \"major\": \"4\","
        "                      \"minor\": \"0\","
        "                      \"mode\":  \"666\""
        "                  }"
        "}"
    },
    {
        "Bad top level key",
        "{\"wrongKey\": ["
        "                  {"
        "                      \"name\":  \"tty0\","
        "                      \"major\": \"4\","
        "                      \"minor\": \"0\","
        "                      \"mode\":  \"666\""
        "                  }"
        "               ]"
        "}"
    },
    {
        "Missing name",
        "{\"devices\": ["
        "                  {"
        "                      \"major\": \"4\","
        "                      \"minor\": \"0\","
        "                      \"mode\":  \"666\""
        "                  }"
        "               ]"
        "}"
    },
    {
        "Missing major and name and minor",
        "{\"devices\": ["
        "                  {"
        "                      \"mode\":  \"666\""
        "                  }"
        "               ]"
        "}"
    },
    {
        "Missing all props",
        "{\"devices\": ["
        "                  {"
        "                  }"
        "               ]"
        "}"
    },
    {
        "'Devices' is a string?!",
        "{\"devices\": \"hej\" }"
    },
    {
        "Last device malformed",
        "{\"devices\": ["
        "                  {"
        "                      \"name\":  \"tty0\","
        "                      \"major\": \"4\","
        "                      \"minor\": \"0\","
        "                      \"mode\":  \"666\""
        "                  },"
        "                  {"
        "                      \"major\": \"4\","
        "                      \"minor\": \"0\","
        "                      \"mode\":  \"666\""
        "                  }"
        "              ]"
        "}"
    },
};

const struct testData validConfigs[] = {
    {
        "Correct 1",
        "{\"devices\": ["
        "                  {"
        "                      \"name\":  \"tty0\","
        "                      \"major\": \"4\","
        "                      \"minor\": \"0\","
        "                      \"mode\":  \"666\""
        "                  }"
        "              ]"
        "}",
        {"tty0"},
        {"4"},
        {"0"},
        {"666"}
    },
    {
        "Correct 2",
        "{\"devices\": [ ]}",
        {},
        {},
        {},
        {}
    },
    {
        "Correct 2",
        "{\"devices\": ["
        "                  {"
        "                      \"name\":  \"tty0\","
        "                      \"major\": \"4\","
        "                      \"minor\": \"0\","
        "                      \"mode\":  \"666\""
        "                  },"
        "                  {"
        "                      \"name\":  \"tty1\","
        "                      \"major\": \"4\","
        "                      \"minor\": \"0\","
        "                      \"mode\":  \"400\""
        "                  },"
        "                  {"
        "                      \"name\":  \"/dev/galcore\","
        "                      \"major\": \"199\","
        "                      \"minor\": \"0\","
        "                      \"mode\":  \"666\""
        "                  }"
        "              ]"
        "}",
        {"tty0", "tty1", "/dev/galcore"},
        {"4", "4", "199"},
        {"0", "0", "0"},
        {"666", "400", "666"}
    },
};
