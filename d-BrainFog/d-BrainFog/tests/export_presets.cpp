// Genera los presets de fabrica como archivos .dbfpreset (XML compatible con el estado del plugin).
#include "../Source/DSP/Presets.h"
#include <cstdio>
#include <string>
int main(int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : ".";
    for (int p = 0; p < bf::kNumFactoryPresets; ++p) {
        float v[bf::P_COUNT]; bf::factoryPresetValues(p, v);
        std::string name = bf::kFactoryPresetNames[p];
        const std::string path = dir + "/" + name + ".dbfpreset";
        FILE* f = std::fopen(path.c_str(), "w"); if (!f) return 1;
        std::fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<STATE presetName=\"%s\">\n", name.c_str());
        for (int i = 0; i < bf::P_COUNT; ++i) std::fprintf(f, "  <PARAM id=\"%s\" value=\"%.6g\"/>\n", bf::kParams[i].id, v[i]);
        std::fprintf(f, "</STATE>\n");
        std::fclose(f);
    }
    return 0;
}
