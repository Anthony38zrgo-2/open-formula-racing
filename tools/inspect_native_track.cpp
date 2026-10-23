#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <tgf.hpp>
#include <itrackloader.h>
#include <robottools.h>

int main(int argument_count, char** argument_values)
{
    if (argument_count != 5)
        return EXIT_FAILURE;

    GfInit();
    GfInitInstallDir(argument_values[0]);
    GfSetLocalDir("./");
    GfSetDataDir(argument_values[1]);
    GfSetLibDir(argument_values[2]);
    GfModule* track_module = GfModule::load(std::string(GfLibDir()) + "modules/track/trackv1.dll");
    if (!track_module)
        return EXIT_FAILURE;

    ITrackLoader* track_loader = track_module->getInterface<ITrackLoader>();
    tTrack* track = track_loader ? track_loader->load(argument_values[3]) : nullptr;
    if (!track)
        return EXIT_FAILURE;

    std::ofstream output(argument_values[4]);
    output << std::setprecision(15);
    output << "{\"length\":" << track->length << ",\"width\":" << track->width << ",\"segments\":[";
    tTrackSeg* segment = track->seg->next;
    for (int segment_index = 0; segment_index < track->nseg; ++segment_index)
    {
        if (segment_index)
            output << ',';
        output << "{\"name\":\"" << segment->name << "\",\"distance\":" << segment->lgfromstart;
        output << ",\"length\":" << segment->length << ",\"angle\":" << segment->angle[TR_ZS];
        output << ",\"start_left\":[" << segment->vertex[TR_SL].x << ',' << segment->vertex[TR_SL].y << ',' << segment->vertex[TR_SL].z << ']';
        output << ",\"start_right\":[" << segment->vertex[TR_SR].x << ',' << segment->vertex[TR_SR].y << ',' << segment->vertex[TR_SR].z << ']';
        output << ",\"end_left\":[" << segment->vertex[TR_EL].x << ',' << segment->vertex[TR_EL].y << ',' << segment->vertex[TR_EL].z << ']';
        output << ",\"end_right\":[" << segment->vertex[TR_ER].x << ',' << segment->vertex[TR_ER].y << ',' << segment->vertex[TR_ER].z << ']';
        const tTrackSeg* left_border = segment->lside ? segment->lside : segment;
        const tTrackSeg* right_border = segment->rside ? segment->rside : segment;
        output << ",\"road_left\":[" << left_border->vertex[TR_SL].x << ',' << left_border->vertex[TR_SL].y << ',' << left_border->vertex[TR_SL].z << ']';
        output << ",\"road_right\":[" << right_border->vertex[TR_SR].x << ',' << right_border->vertex[TR_SR].y << ',' << right_border->vertex[TR_SR].z << "]}";
        segment = segment->next;
    }
    output << "],\"pits\":{\"type\":" << track->pits.type << ",\"max\":" << track->pits.nMaxPits
           << ",\"slots\":" << track->pits.nPitSeg << ",\"positions\":[";
    for (int index = 0; index < track->pits.nMaxPits; ++index)
    {
        auto position = track->pits.driversPits[index].pos;
        tdble x, y;
        RtTrackLocal2Global(&position, &x, &y, TR_TORIGHT);
        if (index) output << ',';
        output << "{\"x\":" << x << ",\"y\":" << y << ",\"distance\":" << position.seg->lgfromstart + (position.seg->type == TR_STR ? position.toStart : position.toStart * position.seg->radius)
               << ",\"to_right\":" << position.toRight << '}';
    }
    output << "]}}";
    track_loader->unload();
    GfModule::unload(track_module);
    return output.good() ? EXIT_SUCCESS : EXIT_FAILURE;
}
