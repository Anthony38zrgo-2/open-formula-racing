#ifndef MK_TRACKLIGHT_H
#define MK_TRACKLIGHT_H

typedef struct Situation tSituation;

namespace ssggraph {

void grTrackLightInit();
void grTrackLightUpdate( tSituation *s );
void grTrackLightShutdown();

} // namespace ssggraph

#endif //MK_TRACKLIGHT_H
