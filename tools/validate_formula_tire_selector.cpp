#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include <tgf.h>

#include "tire_model_configuration.h"

static int failures = 0;

static void Check(bool condition, const char* name)
{
    if (!condition)
    {
        std::printf("FAIL %s\n", name);
        failures += 1;
    }
    else
    {
        std::printf("PASS %s\n", name);
    }
}

static std::string PressureSection(const std::string& convention)
{
    return std::string("<section name=\"Formula Tires\">\n")
        + "<attnum name=\"schema version\" val=\"1\"/>\n"
        + "<attstr name=\"pressure convention\" val=\"" + convention + "\"/>\n"
        + "<attstr name=\"pressure unit\" val=\"Pa\"/>\n"
        + "<attnum name=\"ambient pressure\" val=\"101325\" unit=\"Pa\"/>\n"
        + "<attnum name=\"reference temperature\" val=\"298.15\" unit=\"K\"/>\n"
        + "<attnum name=\"cold pressure front right\" val=\"130000\" unit=\"Pa\"/>\n"
        + "<attnum name=\"cold pressure front left\" val=\"130000\" unit=\"Pa\"/>\n"
        + "<attnum name=\"cold pressure rear right\" val=\"130000\" unit=\"Pa\"/>\n"
        + "<attnum name=\"cold pressure rear left\" val=\"130000\" unit=\"Pa\"/>\n"
        + "<attnum name=\"reference hot pressure front right\" val=\"160000\" unit=\"Pa\"/>\n"
        + "<attnum name=\"reference hot pressure front left\" val=\"160000\" unit=\"Pa\"/>\n"
        + "<attnum name=\"reference hot pressure rear right\" val=\"150000\" unit=\"Pa\"/>\n"
        + "<attnum name=\"reference hot pressure rear left\" val=\"150000\" unit=\"Pa\"/>\n"
        + "</section>\n";
}

static std::string Document(const std::string& controls, const std::string& tires)
{
    return "<params name=\"selector test\">\n" + controls + tires + "</params>\n";
}

static bool ReadConfiguration(const char* path, TireModelConfiguration& configuration)
{
    void* handle = GfParmReadFile(path, GFPARM_RMODE_STD);
    if (handle == NULL)
    {
        return false;
    }
    double loads[4] = { 1300.0, 1300.0, 1700.0, 1700.0 };
    double radii[4] = { 0.312, 0.312, 0.322, 0.322 };
    double widths[4] = { 0.308, 0.308, 0.389, 0.389 };
    double masses[4] = { 13.0, 13.0, 16.0, 16.0 };
    bool valid = TireModelConfigurationRead(configuration, handle, loads, radii, widths, masses,
        101325.0, 298.15);
    GfParmReleaseHandle(handle);
    return valid;
}

static bool WriteDocument(const char* path, const std::string& text)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream)
    {
        return false;
    }
    stream << text;
    return stream.good();
}

int main()
{
    GfInit();
    TireModelConfiguration configuration;
    const char* paths[5] = {
        "validate_tire_selector_absent.xml",
        "validate_tire_selector_legacy.xml",
        "validate_tire_selector_coupled.xml",
        "validate_tire_selector_schema.xml",
        "validate_tire_selector_pressure.xml"
    };
    std::string absent = Document("", "");
    std::string legacy = Document(
        "<section name=\"Formula Controls\"><attstr name=\"tire model\" val=\"legacy\"/></section>\n",
        PressureSection("gauge"));
    std::string coupled = Document(
        "<section name=\"Formula Controls\"><attstr name=\"tire model\" val=\"transient coupled\"/></section>\n",
        PressureSection("gauge"));
    std::string bad_schema_tires = PressureSection("gauge");
    bad_schema_tires.replace(bad_schema_tires.find("val=\"1\""), 7, "val=\"2\"");
    std::string bad_schema = Document(
        "<section name=\"Formula Controls\"><attstr name=\"tire model\" val=\"transient coupled\"/></section>\n",
        bad_schema_tires);
    std::string bad_pressure = Document(
        "<section name=\"Formula Controls\"><attstr name=\"tire model\" val=\"transient coupled\"/></section>\n",
        "<section name=\"Formula Tires\"><attnum name=\"schema version\" val=\"1\"/></section>\n");
    bool written = WriteDocument(paths[0], absent) && WriteDocument(paths[1], legacy)
        && WriteDocument(paths[2], coupled) && WriteDocument(paths[3], bad_schema)
        && WriteDocument(paths[4], bad_pressure);
    Check(written, "selector fixtures written");

    bool valid = ReadConfiguration(paths[0], configuration);
    Check(valid && configuration.enabled == 0, "absent selector stays legacy");
    valid = ReadConfiguration(paths[1], configuration);
    Check(valid && configuration.enabled == 0, "legacy selector stays legacy");
    valid = ReadConfiguration(paths[2], configuration);
    Check(valid && configuration.enabled == 1, "coupled selector enables model");
    valid = ReadConfiguration(paths[3], configuration);
    Check(!valid && configuration.enabled == 0, "invalid schema disables model");
    valid = ReadConfiguration(paths[4], configuration);
    Check(!valid && configuration.enabled == 0, "missing pressure disables model");

    for (int index = 0; index < 5; ++index)
    {
        std::remove(paths[index]);
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
